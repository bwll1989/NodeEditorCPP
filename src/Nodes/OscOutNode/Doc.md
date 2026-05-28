# OSC Output 节点

## 1. 节点说明

OSC 发送节点（界面标题为「OSC Output」），向指定主机和端口发送 Open Sound Control 消息。适合控制外部媒体服务器、灯光台或任何支持 OSC 的软件。

## 2. 端口说明

### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | HOST | 设置目标 IP |
| 1 | PORT | 设置目标端口 |
| 2 | ADDRESS | 设置 OSC 地址并发送 |
| 3 | VALUE | 设置参数值并发送（改值即发送） |
| 4 | TRIGGER | 输入为 `true` 时触发发送 |

### 输出

本节点无输出端口。

## 3. 界面说明

- **host**：目标 IP，默认 `127.0.0.1`。
- **port**：目标端口，默认 8000。
- **address**：OSC 路径，默认 `/test`。
- **type**：参数类型 Int、Float 或 String。
- **value**：参数值（界面修改会立即尝试发送）。
- **Send**：手动发送一次。

## 4. 使用说明

1. 填写 **host**、**port**、**address**、**type**、**value**。
2. 点 **Send**，或从对应输入端口写入数据；**VALUE** 端口更新时会自动发送。
3. **TRIGGER** 端口收到 `true` 时执行一次发送。
4. 工程会保存主机、端口、地址、类型与数值。

**外部控制路径**（完整地址：`/dataflow/{父级别名}/{节点ID}{相对路径}`）：

| 相对路径 | 作用 |
|----------|------|
| `/host` | 目标 IP |
| `/port` | 目标端口 |
| `/address` | OSC 地址路径 |
| `/value` | 参数值 |
| `/type` | 类型索引：0=Int，1=Float，2=String |
| `/send` | 触发发送（命令；无 payload 或 `true` 时执行） |

支持 OSC / 全局事件总线命令。

## 5. 示例

- **控制 Resolume**：host 填控台 IP，address `/composition/columns/1/connect`，type String，value `1`。
- **节拍闪灯**：LFO 输出连 **TRIGGER**，address `/light/strobe`，value 交替 0/1。
- **批量改目标**：Switch 节点选择不同 **HOST** 输入，同一套逻辑控制多台设备。
