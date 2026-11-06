# -*- coding: utf-8 -*-
"""flow-mcp：把 Flow / FlowRuntime 的图编辑能力暴露为 MCP 工具。

架构：MCP 客户端（任意模型） → 本服务（翻译层） → Flow HTTP :8992（能力层）。

环境变量：
  FLOW_HOST             Flow 主机，默认 127.0.0.1
  FLOW_PORT             HTTP 端口，默认 8992
  FLOW_KNOWLEDGE_PATH   ai-knowledge.json 路径（默认 <项目>/docs/ai-knowledge.json）
  FLOW_BACKUP_DIR       备份目录（默认 <项目>/mcp/backups）
"""

from __future__ import annotations

import json
import shutil
import time
from pathlib import Path
from typing import Any

from mcp.server.fastmcp import FastMCP

from .flow_client import FlowClient, FlowError
from .knowledge import Knowledge

PROJECT_ROOT = Path(__file__).resolve().parents[2]
BACKUP_DIR = Path(
    __import__("os").environ.get("FLOW_BACKUP_DIR", PROJECT_ROOT / "mcp" / "backups")
)
MINIMAL_FLOW = Path(__file__).resolve().with_name("minimal_flow.json")
FLOW_SPEC = Path(__file__).resolve().with_name("flow_spec.md")

mcp = FastMCP(
    "flow",
    instructions=(
        "驱动 Flow 节点图编辑器：用自然语言需求生成/修改节点图并应用到软件，"
        "运行时调参数。改图前先 get_current_graph，查节点用 search_nodes/get_node_doc。"
    ),
)

_client = FlowClient()
_knowledge: Knowledge | None = None


def _know() -> Knowledge:
    global _knowledge
    if _knowledge is None:
        try:
            _knowledge = Knowledge()
        except FileNotFoundError as exc:
            raise RuntimeError(str(exc)) from exc
    return _knowledge


def _j(obj: Any) -> str:
    import json

    return json.dumps(obj, ensure_ascii=False, indent=2)


# ----------------------------------------------------------------------
# 工具 0：格式规范
# ----------------------------------------------------------------------
@mcp.tool()
def get_flow_format_spec() -> str:
    """获取 .flow 节点图 JSON 格式规范（生成/修改图之前必读）。

    返回 DataFlow 结构、nodes/connections 字段表、internal-data 规则、
    intNodeId 拼写陷阱、标准搭图工作流程。第一次为用户改图前先调用本工具。
    """
    return FLOW_SPEC.read_text(encoding="utf-8")


# ----------------------------------------------------------------------
# 工具 1：探活
# ----------------------------------------------------------------------
@mcp.tool()
def get_app_info() -> str:
    """查询 Flow 软件名称与版本，兼做在线探活。

    返回 {"ok":true,"name":"Flow","version":"1.6.14"}。
    连接失败会给出明确错误（Flow 未运行 / 地址不对）。
    """
    try:
        return _j(_client.get_app_info())
    except FlowError as exc:
        return _j({"ok": False, "error": str(exc)})


# ----------------------------------------------------------------------
# 工具 2：读当前图
# ----------------------------------------------------------------------
@mcp.tool()
def get_current_graph(full: bool = False) -> str:
    """读取 Flow 当前加载的节点图 JSON。

    返回的 graph 为 DataFlow 内容对象 {"nodes":[...], "connections":[...], "groups":[...]}
    （工具已自动处理 .flow 中 DataFlow→scene 的包装层，模型无需感知）。
    full=false（默认）：精简版——剥离 internal-data（内部状态长文本），适合增量改图。
    full=true：完整版——含 internal-data，仅在需要查看/修改节点内部配置时使用。
    Flow 无工程时返回错误说明。
    """
    data = _client.download_current_flow()
    if data is None:
        return _j(
            {
                "ok": False,
                "error": "Flow 当前没有打开的工程。请让用户在 Flow 中新建并保存一个工程，"
                "或直接用 apply_graph 从空图开始（将自动以最小模板打底）。",
            }
        )
    try:
        root = json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        return _j({"ok": False, "error": f"当前工程不是合法 JSON：{exc}"})
    scene = _unwrap_scene(root.get("DataFlow", {}))
    if not full:
        for node in scene.get("nodes", []):
            node.pop("internal-data", None)
    return _j(
        {
            "ok": True,
            "graph": {"nodes": scene.get("nodes", []),
                      "connections": scene.get("connections", []),
                      "groups": scene.get("groups", [])},
        }
    )


# ----------------------------------------------------------------------
# 工具 3：备份
# ----------------------------------------------------------------------
@mcp.tool()
def backup_current_flow() -> str:
    """把 Flow 当前工程备份到 mcp/backups/<时间戳>.flow，返回备份路径。

    建议在每次 apply_graph 之前调用（apply_graph 自身也会自动备份一次）。
    """
    data = _client.download_current_flow()
    if data is None:
        return _j({"ok": False, "error": "Flow 当前没有打开的工程，无需备份。"})
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    name = time.strftime("backup-%Y%m%d-%H%M%S.flow")
    dest = BACKUP_DIR / name
    dest.write_bytes(data)
    return _j({"ok": True, "backup": str(dest)})


# ----------------------------------------------------------------------
# 校验器（MCP 侧软校验；服务端硬校验在 M2）
# ----------------------------------------------------------------------
def _validate_dataflow(df: dict, old_df: dict | None) -> tuple[list, list, dict]:
    """返回 (errors, warnings, normalized_df)。"""
    errors: list[str] = []
    warnings: list[str] = []
    nodes = df.get("nodes")
    if not isinstance(nodes, list):
        return ["nodes 必须是数组"], warnings, df

    old_nodes = {n.get("id"): n for n in (old_df or {}).get("nodes", []) if isinstance(n, dict)}
    seen_ids: set[int] = set()
    node_index: dict[int, dict] = {}

    for node in nodes:
        if not isinstance(node, dict):
            errors.append(f"nodes 中存在非对象元素: {node!r}")
            continue
        nid = node.get("id")
        ntype = node.get("type")
        if not isinstance(nid, int):
            errors.append(f"节点缺少数字 id: {node}")
            continue
        if nid in seen_ids:
            errors.append(f"节点 id 重复: {nid}")
        seen_ids.add(nid)
        if not isinstance(ntype, str) or not ntype:
            errors.append(f"节点 {nid} 缺少 type")
            continue

        # 别名归一化
        norm = _knowledge_normalize(ntype)
        if norm is None:
            warnings.append(
                f"节点 {nid} 的 type '{ntype}' 不在节点知识库中——"
                "请用 search_nodes 确认规范注册名，否则 Flow 可能拒绝创建。"
            )
        elif norm != ntype:
            warnings.append(f"节点 {nid} 的 type '{ntype}' 已归一化为 '{norm}'。")
            node["type"] = norm

        # 位置兜底：AI 常忘写 position，自动排布避免全部叠原点
        pos = node.get("position")
        if not isinstance(pos, dict) or "x" not in pos or "y" not in pos:
            idx = len(node_index)
            node["position"] = {"x": 80 + (idx % 5) * 260, "y": 60 + (idx // 5) * 220}
            warnings.append(f"节点 {nid} 缺少 position，已自动排布为 {node['position']}。")

        # internal-data 白名单提示
        if nid in old_nodes:
            old_data = old_nodes[nid].get("internal-data")
            new_data = node.get("internal-data", {})
            if old_data and new_data and new_data != old_data:
                warnings.append(
                    f"节点 {nid}（{node['type']}）的 internal-data 与原图不同——"
                    "如果你并非有意修改其内部状态，请从 full 版原图原样复制。"
                )
        else:
            if node.get("internal-data"):
                warnings.append(
                    f"新建节点 {nid}（{node['type']}）携带了 internal-data。"
                    "除非该内容来自 get_node_doc/用户明确要求，否则应留空对象。"
                )
        node_index[nid] = node

    # 连接校验
    conns = df.get("connections", [])
    if not isinstance(conns, list):
        errors.append("connections 必须是数组")
        conns = []
    for i, conn in enumerate(conns):
        if not isinstance(conn, dict):
            errors.append(f"connections[{i}] 不是对象")
            continue
        out_id, in_id = conn.get("outNodeId"), conn.get("intNodeId")
        if out_id not in node_index:
            errors.append(f"connections[{i}] 的 outNodeId={out_id} 不存在于 nodes")
            continue
        if in_id not in node_index:
            errors.append(f"connections[{i}] 的 intNodeId={in_id} 不存在于 nodes（注意拼写是 intNodeId 不是 inNodeId）")
            continue
        out_node, in_node = node_index[out_id], node_index[in_id]
        out_cnt = out_node.get("output-count", 1)
        in_cnt = in_node.get("input-count", 1)
        if not (0 <= conn.get("outPortIndex", 0) < out_cnt):
            errors.append(
                f"connections[{i}] 越界：节点 {out_id}（{out_node.get('type')}）"
                f"output-count={out_cnt}，outPortIndex={conn.get('outPortIndex')}"
            )
        if not (0 <= conn.get("inPortIndex", 0) < in_cnt):
            errors.append(
                f"connections[{i}] 越界：节点 {in_id}（{in_node.get('type')}）"
                f"input-count={in_cnt}，inPortIndex={conn.get('inPortIndex')}"
            )
    df["connections"] = conns
    return errors, warnings, df


def _knowledge_normalize(name: str) -> str | None:
    try:
        return _know().normalize(name)
    except FileNotFoundError:
        return name  # 知识库缺失时不阻塞，只跳过归一化


def _unwrap_scene(dataflow: Any) -> dict:
    """兼容两种 DataFlow 存储形态，统一拆出 {nodes, connections, groups} 层。

    当前版本 Flow 的工程格式：DataFlow = {"scene": {nodes, connections, groups}}
    （DataflowViewsManger::save 包一层 scene，load 时找不到 scene 则静默返回空图）。
    """
    if isinstance(dataflow, dict) and isinstance(dataflow.get("scene"), dict):
        return dataflow["scene"]
    return dataflow if isinstance(dataflow, dict) else {}


def _wrap_scene(scene: dict) -> dict:
    """把 {nodes, connections, groups} 包装为当前 Flow 工程格式（scene 层）。"""
    if "scene" in scene and "nodes" not in scene:
        return scene  # 已是包装形态
    return {"scene": scene}


def _load_base_project() -> tuple[dict, str]:
    """取打底工程 JSON：优先当前工程，失败用最小模板。"""
    data = _client.download_current_flow()
    if data is not None:
        return json.loads(data.decode("utf-8")), "current_flow"
    return json.loads(MINIMAL_FLOW.read_text(encoding="utf-8")), "minimal_template"


# ----------------------------------------------------------------------
# 工具 4：应用图
# ----------------------------------------------------------------------
@mcp.tool()
def apply_graph(dataflow_json: str, filename: str = "") -> str:
    """校验并把新的 DataFlow 节点图应用到 Flow（上传后软件就地加载生效）。

    dataflow_json：DataFlow 内容对象字符串，即 {"nodes":[...],"connections":[...],"groups":[...]}。
    生成前先 get_flow_format_spec 读规范；改图基于 get_current_graph 增量修改。
    filename：可选，保存的文件名（默认 ai-edit-<时间戳>.flow）。
    返回校验错误 / 警告 / 加载结果。自动先备份当前工程。
    """
    try:
        new_df = json.loads(dataflow_json)
    except json.JSONDecodeError as exc:
        return _j({"ok": False, "error": f"dataflow_json 不是合法 JSON：{exc}"})

    # 兼容三种输入：DataFlow 内容对象 / 完整工程 / 已带 scene 包装
    if isinstance(new_df, dict) and "DataFlow" in new_df and "nodes" not in new_df:
        new_df = new_df["DataFlow"]
    new_df = _unwrap_scene(new_df)
    if not isinstance(new_df, dict) or "nodes" not in new_df:
        return _j({"ok": False, "error": "dataflow_json 应为 DataFlow 内容对象（含 nodes 数组）"})

    base_root, base_src = _load_base_project()
    old_df = _unwrap_scene(base_root.get("DataFlow", {}))

    errors, warnings, new_df = _validate_dataflow(new_df, old_df)
    if errors:
        return _j(
            {
                "ok": False,
                "stage": "validate",
                "errors": errors,
                "warnings": warnings,
                "hint": "修正以上错误后重新调用 apply_graph。",
            }
        )

    # 自动备份
    backup = ""
    try:
        current = _client.download_current_flow()
        if current is not None:
            BACKUP_DIR.mkdir(parents=True, exist_ok=True)
            backup = str(BACKUP_DIR / time.strftime("auto-%Y%m%d-%H%M%S.flow"))
            Path(backup).write_bytes(current)
    except Exception:  # 备份失败不阻塞应用，但记录
        warnings.append("自动备份失败（继续应用）。")

    # 合并并上传（统一包装为当前版本的 scene 形态）
    base_root["DataFlow"] = _wrap_scene(new_df)
    fname = filename.strip() or time.strftime("ai-edit-%Y%m%d-%H%M%S.flow")
    if not fname.lower().endswith(".flow"):
        fname += ".flow"
    payload = json.dumps(base_root, ensure_ascii=False, indent=2).encode("utf-8")
    try:
        upload_result = _client.upload_flow(fname, payload)
    except FlowError as exc:
        return _j({"ok": False, "stage": "upload", "error": str(exc), "warnings": warnings})

    return _j(
        {
            "ok": True,
            "applied": True,
            "loaded_from": upload_result.get("path", ""),
            "base": base_src,
            "backup": backup,
            "node_count": len(new_df.get("nodes", [])),
            "connection_count": len(new_df.get("connections", [])),
            "warnings": warnings,
            "hint": "图已加载。运行时调参数用 send_command；"
            "若要把改动固化到用户原工程文件，请在 Flow 中 Ctrl+S 保存。",
        }
    )


# ----------------------------------------------------------------------
# 工具 5：运行时控制
# ----------------------------------------------------------------------
@mcp.tool()
def send_command(address: str, value: str) -> str:
    """向运行中的节点图发送控制指令（等价网页控制台/OSC）。

    address：OSC 地址，形如 /dataflow/{别名}/{节点ID}/{参数}，如 /dataflow/DataFlow/3/int。
    value：字符串；数字/布尔/null 会自动转换类型（如 "60"→60，"true"→true，"off"→"off"）。
    """
    try:
        parsed: Any = json.loads(value)
    except json.JSONDecodeError:
        parsed = value  # 纯文本按字符串发送
    try:
        return _j(_client.send_command(address, parsed))
    except FlowError as exc:
        return _j({"ok": False, "error": str(exc)})


# ----------------------------------------------------------------------
# 工具 6/7：节点知识检索
# ----------------------------------------------------------------------
@mcp.tool()
def search_nodes(keyword: str, limit: int = 8) -> str:
    """按关键词搜索节点库（AI 搭图前查节点能力的主要入口）。

    匹配节点注册名/别名/分类/功能摘要/端口名/属性名，如 "audio" "artnet" "亮度" "选择"。
    返回每个节点的 type（规范注册名）、tag、summary、ports（输入输出端口及类型说明）、props（属性说明）。
    生成节点前必须用这里的 type 原文作为节点 type 字段。
    """
    try:
        results = _know().search(keyword, max(1, min(int(limit), 20)))
    except (FileNotFoundError, RuntimeError) as exc:
        return _j({"ok": False, "error": f"知识库不可用：{exc}"})
    return _j({"ok": True, "count": len(results), "nodes": results})


@mcp.tool()
def get_node_doc(node_type: str) -> str:
    """获取单个节点的完整知识（精确匹配，支持别名归一化）。

    返回 type、tag、summary、ports、props；未知类型返回近似建议。
    """
    try:
        k = _know()
    except (FileNotFoundError, RuntimeError) as exc:
        return _j({"ok": False, "error": f"知识库不可用：{exc}"})
    doc = k.get(node_type)
    if doc is None:
        near = k.search(node_type, limit=5)
        return _j(
            {
                "ok": False,
                "error": f"知识库中无 '{node_type}'。近似结果见 suggestions（type 用其原文）。",
                "suggestions": near,
            }
        )
    return _j({"ok": True, **doc})


@mcp.tool()
def list_all_nodes() -> str:
    """列出全部节点规范注册名与分类（需要总览节点库时用）。"""
    try:
        names = _know().all_names()
    except (FileNotFoundError, RuntimeError) as exc:
        return _j({"ok": False, "error": f"知识库不可用：{exc}"})
    return _j({"ok": True, "count": len(names), "nodes": names})


def main() -> None:
    mcp.run()


if __name__ == "__main__":
    main()
