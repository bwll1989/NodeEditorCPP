# Flow AI 助手使用指南（flow-mcp）

> 让 AI 通过对话直接搭建 / 修改 Flow 节点图，还能在演出运行中实时调参数。
> 你说需求，AI 生成节点图并应用到 Flow——不需要你自己拖节点、拉连线。

---

## 1. 一分钟了解原理

```
你（对话） ──► WorkBuddy（大模型，可换任意模型）
                │ 通过 MCP 协议调用工具
                ▼
         flow-mcp（本目录的 Python 服务）
                │ 通过 HTTP 访问 127.0.0.1:8992
                ▼
         Flow.exe / FlowRuntime.exe（正在运行的软件）
```

- AI 拿到的是**节点图数据**（不是模拟点击界面），所以可校验、可备份、可回滚
- GUI 版和 Headless 版（FlowRuntime）都能被驱动

---

## 2. 安装（一次性，约 5 分钟）

### 第 1 步：确认 Flow 能被 AI 看见

启动 **Flow.exe 或 FlowRuntime.exe**（HTTP 服务在 8992 端口）。没开软件时 AI 会报"连接被拒绝"。

### 第 2 步：安装 Python 依赖

本机 Python ≥ 3.10，执行（**版本必须 <2**，2.x 接口不兼容）：

```bash
pip install "mcp<2"
```

### 第 3 步：注册到 AI 客户端

编辑 WorkBuddy 配置 `C:\Users\<你的用户名>\.workbuddy\mcp.json`（没有就新建）：

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

> ⚠️ 不要写成 `"args": ["-m", "flow_mcp.server"]` + `"cwd": ...` 的形式——
> 部分 MCP 客户端不支持 `cwd` 字段，会导致启动失败。请始终使用上面的绝对路径启动器写法。

连接 Flow 在另一台电脑时，在 `env` 里加一行：

```json
"FLOW_HOST": "192.168.x.x"
```

### 第 4 步：信任并重启

1. WorkBuddy「连接器管理」页对 flow 点一次「信任」
2. **完全退出并重启 WorkBuddy**（修改配置后必须重启才生效）

### 接入其他 MCP 客户端（Claude Desktop 等）

写法相同：`command` + 绝对路径 `run_server.py` + `env` 三件套，见 `README.md`。

---

## 2.5 分发给别人用

`mcp/` 文件夹是**自包含**的：`flow_mcp/data/` 里内置了节点知识库副本，
把整个 `mcp/` 文件夹拷给任何人、放到任何位置都能直接跑（无需连 Flow 源码一起拷）。

给使用者的三步说明：

1. **拿到 `mcp/` 文件夹**（如 `D:\Tools\flow-mcp\`），**Flow.exe 要能访问**（同一台电脑或配 `FLOW_HOST` 指向 Flow 所在机器）
2. 本机装 Python ≥3.10 并执行 `pip install "mcp<2"`
3. 按**第 3 步**写 AI 客户端的配置，把路径换成实际的 `mcp/` 位置即可

> 知识库随分发的 `mcp/` 文件夹走；开发机上的源码树会优先读取 `docs/ai-knowledge.json`（文档流水线的最新版），
> 单独分发的文件夹则使用内置副本。Flow 升级节点后重新分发一次 `mcp/` 即可同步知识库。

---

---

## 3. 日常使用

**直接在对话里说人话即可**。自定义 MCP 的工具会自动按需加载，
**不会出现在"+"菜单的连接器搜索列表里——那是正常现象**，不要去那里找。

### 典型说法

| 你说 | AI 做的事 |
|---|---|
| "连一下 Flow" | 探活，返回软件版本 |
| "看看当前工程里有什么节点" | 读当前图，列出节点与连接 |
| "搭一个 Web Render 接画面显示的图，加个开关控制启用" | 查节点知识 → 生成图 → 校验 → 备份 → 应用加载 |
| "在现有图里加一个 Bool Source 控制窗口显示" | 读当前图 → 增量修改 → 应用 |
| "把 3 号节点的启用开关关掉" | 运行时指令（不等同于改图，立即生效不用重新加载） |
| "有哪些音频相关的节点？" | 检索节点知识库，列出能力说明 |

### 建议的对话习惯

1. **改图前让 AI 先看图**："基于当前工程……"——AI 会先 `get_current_graph` 再增量修改，而不是推倒重画
2. **说清楚节点和连接**："加一个 Audio Matrix，把 Audio In 的左右声道都接进去"比"弄个音频处理"效果好
3. **应用后不满意直接说**："把刚才那个节点删掉"——AI 基于当前图继续改，每次应用前都会自动备份

---

## 4. AI 拥有的 9 个能力（工具速查）

| 工具 | 作用 | 你需要关心吗 |
|---|---|---|
| `get_app_info` | 探活：确认 Flow 在线 | 排障时看 |
| `get_flow_format_spec` | 读 .flow 格式规范 | 否（AI 自动用） |
| `get_current_graph` | 读当前节点图 | AI 每次改图前会调 |
| `backup_current_flow` | 手动备份当前工程 | 重要操作前可主动要求 |
| `apply_graph` | **校验+备份+应用新图**（就地加载生效） | 核心 |
| `send_command` | 运行时调参数（OSC 地址） | 微调用 |
| `search_nodes` | 按关键词搜节点能力 | 自动 |
| `get_node_doc` | 查单节点端口/属性/边界语义 | 自动 |
| `list_all_nodes` | 列出全部节点类型 | 自动 |

## 5. 安全机制（你该知道的）

- **自动备份**：每次 `apply_graph` 前自动把当前工程存到 `mcp/backups/auto-<时间戳>.flow`。恢复方法：把备份文件复制到任意位置，在 Flow 里正常打开即可
- **不会动你的原文件**：应用后加载的是上传副本，你磁盘上的原 `.flow` 不被修改；想固化改动在 Flow 里 **Ctrl+S**
- **三层校验**：节点类型必须真实存在（别名自动纠正，如 `AudioOut`→`Audio Out`）、连接端口索引越界拦截、节点内部数据（internal-data）被 AI 改动时给出警告
- **Headless 同样可用**：FlowRuntime.exe 开着时整套能力照常工作

## 6. 故障排查

| 症状 | 原因 | 处理 |
|---|---|---|
| 报"目标计算机积极拒绝 (10061)" | Flow 没开 | 启动 Flow.exe / FlowRuntime.exe |
| 报 502 Bad Gateway | 系统代理劫持了 127.0.0.1 | 已内置直连修复；若仍出现说明会话里的还是旧进程，重启 WorkBuddy |
| 对话里 AI 说"知识库不可用" | `docs/ai-knowledge.json` 被移动 | 设置环境变量 `FLOW_KNOWLEDGE_PATH` 指向新位置 |
| 新会话里 AI 没有这些工具 | 改过 mcp.json 后没重启 | 完全退出 WorkBuddy 再启动 |
| 生效的是旧行为（如打开图空白） | 会话里的 MCP 进程是旧代码 | 重启 WorkBuddy 重新拉起 |
| 打开旧生成的 .flow 是空白的 | 早期版本生成的文件缺 `scene` 包装层 | 用同名 `.bak` 文件恢复，或让 AI 重新生成（新版已修复） |
| 8992 端口被其他软件占用 | Flow 配置了别的端口 | `mcp.json` 的 `env` 里加 `"FLOW_PORT": "<实际端口>"` |

## 7. 文件说明

```
mcp/
├── run_server.py            # 启动器（MCP 配置指向它，别动）
├── README.md                # 开发者文档：架构与接入
├── USER-GUIDE.md            # 本文档
├── pyproject.toml           # 依赖声明（mcp>=1.2,<2）
├── backups/                 # 自动备份（时间戳命名）
└── flow_mcp/
    ├── server.py            # MCP 服务器：9 个工具 + 校验器
    ├── flow_client.py       # Flow HTTP API 封装（已禁用系统代理）
    ├── knowledge.py         # 节点知识库检索（173 节点，含别名归一化）
    ├── flow_spec.md         # 喂给 AI 的 .flow 格式规范
    └── minimal_flow.json    # 空工程模板
```

## 8. 两个重要的行为说明

1. **"应用"≠"保存"**：AI 应用新图后，Flow 立即加载显示，但**你磁盘上的原工程文件不变**——满意后按 Ctrl+S 保存，不满意直接关掉 Flow 里加载的副本即可。
2. **改图基于当前工程**：对话里说"加一个 XX 节点"时，AI 会先读当前图再做增量修改；如果你想从零开始，明确说"清空画布重新搭"。
