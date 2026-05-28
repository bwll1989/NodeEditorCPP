# PJLinkNode

## 1. 节点说明

通过 **PJLink** 协议（TCP）控制网络投影仪：开关机、静音/取消静音、自定义命令；支持密码认证与连接状态查询。连接后约每 5 秒查询电源状态作为心跳。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| POWER | VariableData | `true` 开机，`false` 关机 |
| MUTE | VariableData | `true` 静音，`false` 取消静音 |
| CUSTOM | VariableData | `true` 时发送界面中的自定义命令 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| STATUS | VariableData | 连接状态或设备返回的文本响应 |

## 3. 界面说明

- **主机 / 端口 / 密码**：投影仪 PJLink 地址（默认端口常见为 4352，以设备为准）。
- **开机 / 关机 / 静音开 / 静音关**：快捷按钮。
- **自定义命令**：如 `POWR 1` 或 `AVMT 30`（指令与参数用空格分隔）。
- **连接状态**：是否已连接并完成认证。

外部控制：`/host`、`/port`、`/password`、`/powerOn`、`/powerOff`、`/muteOn`、`/muteOff`、`/custom`、`/customCommand`。

## 4. 使用说明

1. 填写投影仪 IP、端口与 PJLink 密码（若需要）。
2. 等待连接与认证成功后再发控制命令。
3. 自定义命令格式：`指令 参数`，例如 `INPT HDMI1`。
4. 输入端口适合接场景触发或定时节点。

## 5. 示例

场景「会议开始」→ POWER 输入 `true` 开机；场景「结束」→ POWER `false`；STATUS 接日志记录投影仪应答。
