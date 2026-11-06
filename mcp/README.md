# flow-mcp — Flow 节点图 MCP 服务器

把 Flow / FlowRuntime 的节点图能力通过 MCP（Model Context Protocol）暴露给 AI 客户端
（WorkBuddy、Claude Desktop 等），实现"对话搭节点图"。

> **只是想用？看 [USER-GUIDE.md](USER-GUIDE.md)**（安装步骤、对话用法、故障排查）。
> 本 README 面向开发者：架构与实现。

架构：`MCP 客户端（任意模型） → flow-mcp（本服务） → Flow HTTP :8992`

## 工具清单

| 工具 | 作用 |
|---|---|
| `get_flow_format_spec` | 读 .flow JSON 格式规范（改图前必读） |
| `get_app_info` | 探活：确认 Flow 在线 |
| `get_current_graph` | 读当前节点图（默认剥离 internal-data） |
| `backup_current_flow` | 备份当前工程到 `mcp/backups/` |
| `apply_graph` | 校验 + 自动备份 + 应用新图（上传后 Flow 就地加载生效） |
| `send_command` | 运行时调参数（OSC 地址树 `/dataflow/{别名}/{节点ID}/{参数}`） |
| `search_nodes` | 按关键词检索节点知识库（173 节点：端口/属性/边界语义） |
| `get_node_doc` | 取单节点完整知识（支持别名归一化） |
| `list_all_nodes` | 列出全部规范注册名 |

## 环境变量

| 变量 | 默认 | 说明 |
|---|---|---|
| `FLOW_HOST` | 127.0.0.1 | Flow 所在主机 |
| `FLOW_PORT` | 8992 | Flow HTTP 端口 |
| `FLOW_KNOWLEDGE_PATH` | `<项目>/docs/ai-knowledge.json` | 节点知识库路径 |
| `FLOW_BACKUP_DIR` | `<项目>/mcp/backups` | 备份目录 |

## 运行

```bash
# 依赖：Python 3.10+，pip install mcp
cd mcp
python -m flow_mcp.server        # stdio 模式，供 MCP 客户端拉起
```

## 接入 WorkBuddy（`~/.workbuddy/mcp.json`）

**不要用 `-m` + `cwd` 写法**（部分客户端不支持 `cwd` 字段，会导致启动失败、连接器不可见）。
用自包含启动器 `run_server.py`（绝对路径 + PYTHONPATH，与工作目录无关）：

```json
{
  "mcpServers": {
    "flow": {
      "command": "python",
      "args": ["D:/19_NodeEditorCPP/NodeEditorCPP/mcp/run_server.py"],
      "env": {
        "PYTHONPATH": "D:/19_NodeEditorCPP/NodeEditorCPP/mcp",
        "PYTHONUTF8": "1"
      }
    }
  }
}
```

写入后需在 WorkBuddy「连接器管理」页对 flow 服务点一次「信任」才会启用；
**修改 mcp.json 后必须重启 WorkBuddy（或重开会话）才会重新拉起连接器**。

## 接入 Claude Desktop（`claude_desktop_config.json`）

```json
{
  "mcpServers": {
    "flow": {
      "command": "python",
      "args": ["D:/19_NodeEditorCPP/NodeEditorCPP/mcp/run_server.py"],
      "env": {
        "PYTHONPATH": "D:/19_NodeEditorCPP/NodeEditorCPP/mcp",
        "PYTHONUTF8": "1"
      }
    }
  }
}
```

## 使用前提与注意

- **Flow.exe 或 FlowRuntime.exe 必须正在运行**（8992 端口可达）。Headless 同样支持全部工具。
- `apply_graph` 的落地方式：合并新 `DataFlow` 后经 `/api/upload/flow` 上传，Flow **就地加载**（无需重启）；上传副本存放在 Flow 媒体库 flow 目录，**用户原工程文件不会自动更新**——需要固化时在 Flow 里 Ctrl+S。
- `apply_graph` 自带：JSON 结构校验、节点 type 别名归一化、id 重复检查、连接索引越界检查、internal-data 捏造警告、当前工程自动备份。
- M2 计划把校验/备份下沉到 Flow 侧（`/api/ai/*` 端点），本服务的工具签名保持不变。
