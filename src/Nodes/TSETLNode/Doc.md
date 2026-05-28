# TSETLNode

## 1. 节点说明

作为 **TSETL** 触发事件系统的 TCP 客户端：连接服务器、解析二进制帧中的 JSON，在收到 `SignalID` 消息时输出信号 ID 与完整 JSON；并每 5 秒发送心跳维持连接。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| HOST | VariableData | 服务器 IP 或主机名 |
| PORT | VariableData | 端口号（默认 11001） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| SIGNAL_ID | VariableData | 解析出的 SignalID 字符串 |
| JSON_DATA | VariableData | 完整 JSON 消息（键值表） |
| CONNECTION | VariableData | TCP 是否已连接（布尔） |

## 3. 界面说明

- **主机 / 端口**：连接地址，修改后自动重连。
- **连接状态**：是否在线。
- **最近信号**：显示最后一次 SignalID 与时间。

外部控制：`/host`、`/port`、`/connect`（状态）。

## 4. 使用说明

1. 填写 TSETL 服务器地址与端口。
2. 连接成功后等待 `SignalID` 类消息。
3. SIGNAL_ID 可驱动场景切换；JSON_DATA 可接解析或记录。
4. CONNECTION 可用于界面或联锁（未连接时不执行）。

## 5. 示例

TSETL 服务器推送 SignalID `Scene_A` → SIGNAL_ID 接 Switch 选路 → JSON_DATA 接日志节点存档。
