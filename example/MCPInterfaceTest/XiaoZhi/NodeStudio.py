# server.py
from mcp.server.fastmcp import FastMCP
import asyncio
import websockets
import json
import os
import re
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple, Union
# Create an MCP server
mcp = FastMCP("NodeStudio")

_CONFIG_CACHE: Optional[Dict[str, Any]] = None
_REGISTERED_TOOL_NAMES: set[str] = set()

def _get_app_dir() -> Path:
    """获取应用目录（脚本运行取脚本所在目录，打包运行取 exe 所在目录）。"""
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parent

def _get_commands_path() -> Path:
    """获取 commands.json 的路径。"""
    return _get_app_dir() / "commands.json"

def _load_commands_json() -> Dict[str, Any]:
    """读取并解析 commands.json，返回完整 JSON 对象。"""
    path = _get_commands_path()
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)

def _get_commands_root(cfg: Dict[str, Any]) -> Dict[str, Any]:
    """从配置中提取 commands 字典（兼容旧版根对象即 commands 的格式）。"""
    commands = cfg.get("commands")
    if isinstance(commands, dict):
        return commands
    return cfg

def _get_nodestudio_endpoint(cfg: Dict[str, Any]) -> Tuple[str, int]:
    """从配置或环境变量中获取 NodeStudio 的 host/port。"""
    env_host = os.getenv("NODESTUDIO_HOST")
    env_port = os.getenv("NODESTUDIO_PORT")

    nodestudio_cfg = cfg.get("nodestudio") if isinstance(cfg, dict) else None
    if not isinstance(nodestudio_cfg, dict):
        nodestudio_cfg = {}

    host = env_host or str(nodestudio_cfg.get("host") or "127.0.0.1")
    port_raw = env_port or nodestudio_cfg.get("port") or 8992
    try:
        port = int(port_raw)
    except Exception:
        port = 8992
    return host, port

def _get_config() -> Dict[str, Any]:
    """获取配置（带缓存），避免每次工具调用都读磁盘。"""
    global _CONFIG_CACHE
    if _CONFIG_CACHE is None:
        _CONFIG_CACHE = _load_commands_json()
    return _CONFIG_CACHE

def _to_identifier(name: str) -> str:
    """将任意名称转为可用的 Python 标识符（尽量保留可读性）。"""
    s = re.sub(r"[^0-9a-zA-Z_]+", "_", str(name or "")).strip("_")
    if not s:
        return ""
    if s[0].isdigit():
        s = f"cmd_{s}"
    return s

def _safe_doc(text: str) -> str:
    """生成安全的 docstring 内容（避免破坏三引号）。"""
    return (text or "").replace('"""', '\\"\\"\\"')

def _extract_args_spec(command_spec: Dict[str, Any]) -> List[Dict[str, str]]:
    """从命令规格中提取参数定义列表：[{name,type,description}]。"""
    args_raw = command_spec.get("args")
    if isinstance(args_raw, dict) and args_raw:
        out: List[Dict[str, str]] = []
        for k, v in args_raw.items():
            if not isinstance(k, str):
                continue
            if not _to_identifier(k):
                continue
            if isinstance(v, dict):
                t = str(v.get("type") or "string")
                d = str(v.get("description") or "")
            else:
                t = "string"
                d = ""
            out.append({"name": k, "type": t, "description": d})
        return out

    value_spec = command_spec.get("value")
    if isinstance(value_spec, dict) and "$arg" in value_spec:
        arg_name = str(value_spec.get("$arg") or "")
        if _to_identifier(arg_name):
            return [{"name": arg_name, "type": "number", "description": ""}]

    return []

def _python_type_annotation(type_name: str) -> str:
    """把配置里的类型名映射为 Python 注解字符串。"""
    t = str(type_name or "").lower()
    if t in {"int", "integer"}:
        return "int"
    if t in {"float", "number", "double"}:
        return "float"
    if t in {"bool", "boolean"}:
        return "bool"
    return "str"

def _resolve_command_spec(command_name: str, commands: Dict[str, Any]) -> Optional[Dict[str, Any]]:
    """根据命令名或别名在 commands 中定位命令规格。"""
    spec = commands.get(command_name)
    if isinstance(spec, dict):
        return spec

    for _, v in commands.items():
        if not isinstance(v, dict):
            continue
        aliases = v.get("aliases")
        if isinstance(aliases, list) and command_name in aliases:
            return v
    return None

def _resolve_value_spec(value_spec: Any, tool_args: Dict[str, Any]) -> Any:
    """根据 value 规格与工具参数计算最终 value。"""
    if isinstance(value_spec, dict) and "$arg" in value_spec:
        arg_name = str(value_spec.get("$arg"))
        if arg_name not in tool_args:
            raise ValueError(f"缺少参数：{arg_name}")
        v = tool_args.get(arg_name)
        if "scale" in value_spec:
            v = float(v) * float(value_spec.get("scale"))
        if "offset" in value_spec:
            v = float(v) + float(value_spec.get("offset"))
        clamp = value_spec.get("clamp")
        if isinstance(clamp, list) and len(clamp) == 2:
            lo, hi = float(clamp[0]), float(clamp[1])
            v = max(lo, min(hi, float(v)))
        if value_spec.get("type") == "int":
            v = int(round(float(v)))
        elif value_spec.get("type") == "float":
            v = float(v)
        return v

    if isinstance(value_spec, str):
        try:
            return value_spec.format_map(tool_args)
        except Exception:
            return value_spec

    return value_spec

def _build_payload(command_spec: Dict[str, Any], tool_args: Dict[str, Any]) -> Tuple[Dict[str, Any], int]:
    """把 command_spec + tool_args 转成将要发送的 WebSocket payload，并返回预期回复条数。"""
    if "query" in command_spec:
        query = command_spec.get("query")
        if not isinstance(query, list):
            raise ValueError("query 必须是数组")
        return {"query": query}, len(query)

    if "address" not in command_spec:
        raise ValueError("command 必须包含 address 或 query")

    payload: Dict[str, Any] = {"address": command_spec.get("address")}
    if "value" in command_spec:
        payload["value"] = _resolve_value_spec(command_spec.get("value"), tool_args)
    return payload, 0

async def _send_ws(payload: Dict[str, Any], expect_replies: int = 0) -> Union[Dict[str, Any], List[Dict[str, Any]], None]:
    """向 NodeStudio 发送 payload；如 expect_replies>0，则按条数接收并返回。"""
    cfg = _get_config()
    host, port = _get_nodestudio_endpoint(cfg)
    ws_url = f"ws://{host}:{port}"

    async with websockets.connect(ws_url) as ws:
        await ws.send(json.dumps(payload, ensure_ascii=False))

        if expect_replies <= 0:
            return None

        results: List[Dict[str, Any]] = []
        for _ in range(expect_replies):
            raw = await asyncio.wait_for(ws.recv(), timeout=2)
            try:
                obj = json.loads(raw)
                if isinstance(obj, dict):
                    results.append(obj)
            except Exception:
                continue
        return results

async def _run_configured_command(command_name: str, tool_args: Optional[Dict[str, Any]] = None) -> Any:
    """执行 commands.json 里配置的命令，并返回执行结果或查询结果。"""
    tool_args = tool_args or {}
    cfg = _get_config()
    commands = _get_commands_root(cfg)
    spec = _resolve_command_spec(command_name, commands)
    if not isinstance(spec, dict):
        raise ValueError(f"未找到命令：{command_name}")

    payload, expect_replies = _build_payload(spec, tool_args)
    result = await _send_ws(payload, expect_replies=expect_replies)

    if expect_replies > 0:
        if isinstance(result, list) and len(result) == 1:
            return result[0]
        return result

    return {"success": True, "result": "OK"}

@mcp.tool()
async def reload_commands() -> Dict[str, Any]:
    """重新加载 commands.json，用于部署时热更新命令配置。"""
    global _CONFIG_CACHE
    _CONFIG_CACHE = _load_commands_json()
    registered = _register_tools_from_config()
    return {"success": True, "path": str(_get_commands_path()), "registeredTools": registered}

@mcp.tool()
async def run_command(command: str, args_json: str = "") -> Any:
    """运行 commands.json 中的指定命令（args_json 为 JSON 字符串，可选）。"""
    tool_args: Dict[str, Any] = {}
    if args_json:
        obj = json.loads(args_json)
        if not isinstance(obj, dict):
            raise ValueError("args_json 必须是 JSON 对象字符串")
        tool_args = obj
    return await _run_configured_command(command, tool_args=tool_args)

@mcp.tool()
async def list_commands() -> Dict[str, Any]:
    """列出 commands.json 中可用的命令与工具名。"""
    cfg = _get_config()
    commands = _get_commands_root(cfg)
    items: List[Dict[str, Any]] = []
    for k, v in commands.items():
        if not isinstance(v, dict):
            continue
        tool_name = _to_identifier(str(v.get("toolName") or k))
        if not tool_name:
            tool_name = "cmd"
        items.append({
            "command": k,
            "toolName": tool_name,
            "description": v.get("description") or "",
            "aliases": v.get("aliases") if isinstance(v.get("aliases"), list) else [],
        })
    return {"count": len(items), "items": items}

def _register_tools_from_config() -> int:
    """根据 commands.json 动态注册所有命令为 MCP tools。"""
    cfg = _get_config()
    commands = _get_commands_root(cfg)

    registered = 0
    for command_name, spec in commands.items():
        if not isinstance(spec, dict):
            continue

        tool_name_raw = str(spec.get("toolName") or command_name)
        tool_name = _to_identifier(tool_name_raw)
        if not tool_name:
            continue
        if tool_name in _REGISTERED_TOOL_NAMES:
            continue

        args_spec = _extract_args_spec(spec)
        args_spec = [a for a in args_spec if _to_identifier(a.get("name", ""))]
        if len(args_spec) > 8:
            args_spec = []

        doc = str(spec.get("description") or f"运行命令：{command_name}")
        doc = _safe_doc(doc)

        if args_spec:
            params = []
            for a in args_spec:
                py_name = _to_identifier(a["name"])
                ann = _python_type_annotation(a.get("type") or "string")
                params.append(f"{py_name}: {ann}")
            params_str = ", ".join(params)
            tool_args_str = ", ".join([f"{json.dumps(a['name'])}: { _to_identifier(a['name']) }" for a in args_spec])
            func_src = (
                f"async def {tool_name}({params_str}):\n"
                f"    \"\"\"{doc}\"\"\"\n"
                f"    return await _run_configured_command({json.dumps(command_name)}, tool_args={{ {tool_args_str} }})\n"
            )
        else:
            func_src = (
                f"async def {tool_name}():\n"
                f"    \"\"\"{doc}\"\"\"\n"
                f"    return await _run_configured_command({json.dumps(command_name)})\n"
            )

        ns: Dict[str, Any] = {"_run_configured_command": _run_configured_command}
        exec(func_src, ns, ns)
        fn = ns.get(tool_name)
        if not callable(fn):
            continue

        mcp.tool()(fn)
        _REGISTERED_TOOL_NAMES.add(tool_name)
        registered += 1

    return registered

_register_tools_from_config()
# Start the server
if __name__ == "__main__":
    mcp.run(transport="stdio")
