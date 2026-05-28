# Mqtt Client 节点

## 1. 节点说明

MQTT 客户端节点，连接 Broker 后订阅主题、接收消息，并可向主题发布内容。适合物联网设备、智能家居、分布式状态同步等场景。

## 2. 端口说明

### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | HOST | 设置 Broker 地址 |
| 1 | PORT | 设置 Broker 端口 |
| 2 | TOPIC | 设置主题（并重新订阅） |
| 3 | VALUE | 设置发布内容并立即发布 |
| 4 | TRIGGER | 收到数据即按当前 **payload** 发布 |

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | RESULT | 完整消息（含 topic、default、hex 等） |
| 1 | TOPIC | 收到消息的主题 |
| 2 | VALUE | 收到消息的文本内容 |

## 3. 界面说明

- **Host / Port**：Broker 地址与端口，默认 `127.0.0.1:1883`（支持 `mqtt://` 等前缀，会自动剥离）。
- **Username / Password**：可选认证信息；修改后会延迟约 400ms 自动重连。
- **Topic**：订阅与发布使用的主题。
- **Payload**：要发布的消息体。
- **QoS**：服务质量 0、1 或 2（影响订阅与发布）。
- **Connected / Disconnect**：连接状态（只读，绿色为已连接）。
- **Publish**：手动发布。

## 4. 使用说明

1. 填写 Broker **Host**、**Port** 及可选账号密码；节点会自动连接。
2. 设置 **Topic** 后，连接成功即订阅该主题；收到消息从 **RESULT** 等端口输出。
3. 填写 **Payload** 后点 **Publish**，或从 **VALUE** / **TRIGGER** 触发发布。
4. 修改 Host、Port、用户名或密码会触发防抖重连；工程保存全部连接参数与 QoS。

**外部控制路径**（完整地址：`/dataflow/{父级别名}/{节点ID}{相对路径}`）：

| 相对路径 | 作用 |
|----------|------|
| `/host` | Broker 地址 |
| `/port` | Broker 端口 |
| `/topic` | 主题 |
| `/payload` | 发布内容 |
| `/publish` | 触发发布（命令） |
| `/connect` | 连接状态（只读反馈，`true`/`false`） |

`/host`、`/port`、`/topic`、`/payload`、`/publish` 支持 OSC / 全局事件总线；`/connect` 在连接变化时自动反馈。

## 5. 示例

- **传感器上报**：主题 `home/temp`，上游数值连 **VALUE**，定时 **TRIGGER** 发布 JSON。
- **灯控订阅**：订阅 `building/floor1/light/#`，**TOPIC** 连路由节点，**VALUE** 解析开关状态。
- **远程改 Broker**：OSC 更新 `/host` 为 `mqtt.example.com`，`/port` 为 `8883`，节点自动重连并重新订阅。
