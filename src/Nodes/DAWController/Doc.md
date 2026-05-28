# DAWController（Curtain Controller）

## 1. 节点说明

通过 TCP 控制 **电动幕帘**：发送开、关、复位指令（协议帧 `C5` + 地址 + 命令 + `5C`），地址取自主机 IP 末段。连接后每 3 秒发送 `HeartBeat` 心跳；收到运行反馈时输出运行状态。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| OPEN | VariableData | `true` 时打开（命令 `01`） |
| CLOSE | VariableData | `true` 时关闭（命令 `02`） |
| RESET | VariableData | `true` 时复位（命令 `FF`） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| RUNNING | VariableData | 幕帘是否在运行（布尔，收到执行状态 `0x11` 时为 true） |

## 3. 界面说明

- **主机 / 端口**：幕帘控制器地址（默认端口 10001）。
- **打开 / 关闭 / 复位**：与输入端口等效。
- **连接状态**：TCP 是否连接（`/status`）。

外部控制：`/host`、`/port`、`/open`、`/close`、`/reset`。

## 4. 使用说明

1. 主机 IP 末段须与幕帘编址一致（指令中嵌入该字节）。
2. 连接成功后可用按钮或 OPEN/CLOSE/RESET 输入触发。
3. RUNNING 可接指示灯或互锁逻辑，避免重复动作。
4. 修改主机或端口后会自动重连。

## 5. 示例

场景「演出开始」→ OPEN 输入 `true`；「结束」→ CLOSE；RUNNING 接 Hold 节点，运行中禁止再次打开。
