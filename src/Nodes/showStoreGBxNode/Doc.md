# showStoreGBxNode 使用说明

## 用途
通过 TCP 连接 showStoreGBx 设备，触发 4 路播放（Channel1~4）以及停止播放，并输出设备返回的日志文本。

## 端口
### 输入（VariableData）
- SHOW 0：触发通道 1（value 为 true 时触发一次）
- SHOW 1：触发通道 2（value 为 true 时触发一次）
- SHOW 2：触发通道 3（value 为 true 时触发一次）
- SHOW 3：触发通道 4（value 为 true 时触发一次）

### 输出（VariableData）
- LOG 0：日志/返回信息（RESULT.default 为文本）

## 参数/界面
- Host：设备 IP（默认 127.0.0.1）
- Port：设备端口（默认 23）
- Channel 1~4：播放按钮（触发一次播放）
- Stop：停止按钮
- Connected：连接状态显示

## 外部控制（可选）
### 属性（写入）
- /host（string）：设置主机并重连
- /port（int）：设置端口并重连

### 命令（触发）
- /channel1 ~ /channel4（bool 或任意值）：触发对应通道一次
- /stop（bool 或任意值）：触发停止一次

### 反馈（只读）
- /connected：连接状态

## 输出字段（LOG 0）
- default：设备返回的文本（QString）

## 使用步骤
1. 设置 Host/Port，确认已连接（/connected 或界面状态）。
2. 用 SHOW 0~3 输入端口触发通道播放，或直接点界面按钮。
3. 需要停止时发送 /stop 或点 Stop。
4. 从 LOG 0 查看设备返回信息。 

