# WebSocket Client

## 1. 节点说明

WebSocket 客户端节点，连接到远程 WebSocket 服务并收发消息。设备层为独立线程 + QWebSocket（断开后每 **5 秒**自动重连）。适合对接云端 WS API、其他软件的 WebSocket 接口或本机 WebSocket 服务。

发送支持 TEXT/BINARY 两种消息类型 × HEX/UTF-8/ASCII 三种编码；接收打包为多键 VariableData（hex / utf-8 / ascii / 对端地址）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 末端口为 TRIGGER（发送触发） |
| 输出 | 3 | 否 | 3 | 3 | — | 末端口为 STRING（接收文本） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | URL | VariableData | 字符串：设置连接地址并立即连接（如 `ws://127.0.0.1:2003`） |
| 1 | VALUE | VariableData | 字符串：设置发送内容并**立即发送** |
| 2 | TRIGGER | VariableData | 收到任意数据即按当前内容发送（不检查真假） |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | RESULT | VariableData | 接收消息完整键值表（见「行为细节」） |
| 1 | URL | VariableData | 当前连接的 URL 字符串 |
| 2 | STRING | VariableData | 最近接收消息的内容 |

收到消息时三个输出端口同时推送更新。

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 说明 |
|------|------|
| URL 输入框 | WebSocket 地址，默认 `ws://127.0.0.1:2003`；编辑完成即连接 |
| Value 输入框 | 发送内容；编辑完成即发送 |
| Message Type 下拉框 | TEXT / BINARY |
| Format 下拉框 | HEX / UTF-8 / ASCII |
| 状态按钮 | 只读显示 Connected（绿）/ Disconnected（红） |
| Send 按钮 | 发送一次；仅已连接时可用 |

公开属性（Q_PROPERTY）：

| 属性 | 类型 | 说明 |
|------|------|------|
| `url` | string | 连接地址，变更即重连 |
| `value` | string | 发送内容，赋值即发送 |
| `connected` | bool | 连接状态（跟随设备层，界面只读） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/url` | 可写 | 设置连接地址并连接（payload 为字符串） |
| `/host` | 可写 | `/url` 的别名，行为相同 |
| `/value` | 可写 | 设置发送内容并**立即发送**（赋值即发） |
| `/send` | 可写 | 触发一次发送（任意 payload），随后反馈瞬时 true→false |
| `/connected` | 只读 | 连接状态反馈；未订阅 Command，外部写不生效 |

## 5. 行为细节

- **连接管理**：设备层独立线程持有 QWebSocket；URL 变更（端口 0、OSC、界面）会先断开当前连接再发起新连接；连接成功停重连定时器，断开后启动 **5 秒间隔**自动重连；未连接时发送请求被静默丢弃。
- **发送编码**：TEXT/BINARY（Message Type）× HEX（字符串按 hex 解码为字节）/ UTF-8 / ASCII（Latin1）组合发送；未连接不发送。
- **赋值即发**：`/value` 属性变化即触发发送（端口 1、OSC 写、界面编辑完成都一样）；TRIGGER 仅作触发不读取值。
- **发送反馈**：每次发送通过 `/send` 地址推送瞬时 true→false 脉冲（member 绑定无对应属性，仅作状态标识）。
- **接收打包**：收到消息输出 VariableData 键值表：`type`、`url`（对端地址）、`hex`（十六进制字符串）、`utf-8`、`ascii`（二进制消息才有）、`default`（UTF-8 文本）。**注意源码标签写反**：文本消息的 `type` 值为 `BinaryMessage`、二进制消息的 `type` 值为 `TextMessage`，勿依赖该键判断类型。
- **持久化**：`values` 子对象含 `URL`、`Value`、`Format`、`MessageType`（下拉索引）；加载时按保存的 URL 直接发起连接。
- **析构**：随设备层断开连接。

## 6. 使用说明与典型接法

1. 填写 **URL** 后自动连接；修改 URL 会重连。
2. 连接成功后接收数据从 **RESULT** / **STRING** 输出（用 Extract 取键）。
3. 通过 **Send** 按钮、**VALUE** 端口或 OSC `/value` / `/send` 发送；编码由界面 Message Type × Format 决定。
4. 工程会保存 URL、发送内容、格式与消息类型，重开自动重连。

## 7. 示例

- **订阅推送服务**：URL 填 `wss://api.example.com/live`，**STRING** 连解析节点处理推送 JSON。
- **与 Server 节点联调**：本机 WebSocket Server 监听 2003 端口，Client URL 填 `ws://127.0.0.1:2003` 做回环测试。
- **外部触发**：OSC 写 `/value` 为 `{"action":"play"}`（赋值即发），或先写 `/value` 再命令 `/send` 发到云端。
