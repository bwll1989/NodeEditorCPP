# WebSocket Client 节点

## 1. 节点说明

WebSocket 客户端节点，连接到远程 WebSocket 服务并收发消息。适合对接云端 WS API、其他软件的 WebSocket 接口或本机 WebSocket 服务。

## 2. 端口说明

### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | URL | 设置连接地址（如 `ws://127.0.0.1:2003`） |
| 1 | VALUE | 设置发送内容并立即发送 |
| 2 | TRIGGER | 收到数据即按当前内容发送 |

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | RESULT | 完整接收消息 |
| 1 | URL | 当前连接的 URL |
| 2 | STRING | 接收到的文本内容 |

## 3. 界面说明

- **URL**：WebSocket 地址，默认 `ws://127.0.0.1:2003`。
- **Value**：发送内容。
- **Message Type**：TEXT 或 BINARY。
- **Format**：HEX / UTF-8 / ASCII。
- **Connected / Disconnected**：连接状态（只读）。
- **Send**：发送；仅已连接时可点。

## 4. 使用说明

1. 填写 **URL** 后自动连接；修改 URL 会重连。
2. 连接成功后接收数据从 **RESULT**、**STRING** 输出。
3. 通过 **Send** 或 **VALUE** / **TRIGGER** 发送；发送时使用界面上的 **Message Type** 与 **Format**。
4. 工程会保存 URL、格式与消息类型。

**外部控制路径**（完整地址：`/dataflow/{父级别名}/{节点ID}{相对路径}`）：

| 相对路径 | 作用 |
|----------|------|
| `/url` | 设置 WebSocket 地址 |
| `/host` | 与 `/url` 相同（别名，兼容旧地址） |
| `/value` | 设置发送内容 |
| `/connected` | 连接状态（绑定状态按钮） |
| `/send` | 触发发送（命令） |

支持 OSC / 全局事件总线；`/host` 与 `/url` 均可写 URL 字符串。

## 5. 示例

- **订阅推送服务**：URL 填 `wss://api.example.com/live`，**STRING** 连解析节点处理推送 JSON。
- **与 Server 节点联调**：本机 WebSocket Server 在 2003 端口，Client URL 填 `ws://127.0.0.1:2003` 做回环测试。
- **外部触发**：OSC 写 `/value` 为 `{"action":"play"}`，再命令 `/send` 发到云端。
