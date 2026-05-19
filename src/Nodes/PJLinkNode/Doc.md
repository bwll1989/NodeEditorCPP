# PJLinkNode 使用说明

## 用途
通过 PJLink（TCP）控制投影仪的电源/静音，并支持发送自定义 PJLink 指令。

## 端口
### 输入（VariableData）
- POWER：电源控制（bool）
  - true：开机
  - false：关机
- MUTE：静音控制（bool）
  - true：静音开
  - false：静音关
- CUSTOM：发送自定义命令（bool）
  - true：发送一次（命令内容来自界面里的 Custom Command）

### 输出（VariableData）
- STATUS：状态/返回信息（可能输出 bool 或 string）
  - 连接变化时：通常输出 true/false
  - 收到投影仪返回时：可能输出原始响应文本

## 参数/界面
- Host：投影仪 IP/域名（默认 192.168.0.10）
- Port：端口（PJLink 常见为 4352，实际以设备为准）
- Password：密码（设备未启用密码时可留空）
- Custom Command：自定义命令字符串（例如 `POWR ?`、`INPT 11`）
- Power On / Power Off：电源按钮
- Mute On / Mute Off：静音按钮
- Send：发送自定义命令按钮

## 外部控制（可选）
### 属性（写入）
- /host（string）
- /port（int）
- /password（string）
- /customCommand（string）

### 命令（触发）
- /powerOn（bool 或任意值）：触发一次开机
- /powerOff（bool 或任意值）：触发一次关机
- /muteOn（bool 或任意值）：触发一次静音开
- /muteOff（bool 或任意值）：触发一次静音关
- /custom：
  - 传 string：把该字符串当作命令直接发送（例如 `POWR 1`）
  - 传 bool/任意值：触发发送一次（使用当前 /customCommand 或界面内容）

### 反馈（只读）
- /connect：连接状态（用于显示/反馈）

## 使用步骤
1. 设置 Host/Port/Password，等待连接成功（STATUS 或 /connect 有反馈）。
2. 通过 POWER/MUTE 输入端口或界面按钮控制电源/静音。
3. 需要发送自定义指令时，在 Custom Command 填写后点击 Send，或向 CUSTOM 输入 true。 
