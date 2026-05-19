# DAWControllerNode 使用说明

## 用途
DAWControllerNode 用于连接 DAW 控制服务并发送字符串命令。它既支持在节点界面里手动发送，也支持通过输入端口触发发送；同时输出连接状态与最近一次返回信息，方便在流程里联动。

## 端口
- 输入（2）
  - Command：要发送的命令字符串（VariableData）
  - Trigger：触发发送（VariableData；为 true 时发送一次）
- 输出（1）
  - STATUS：状态输出（VariableData）

STATUS 常用字段：
- `connected`：是否连接（bool）
- `last_message`：最近一次收到的消息（string）
- `timestamp`：时间戳（ms，int）

## 节点参数/界面
- host：服务器地址
- command：命令内容
- send：发送按钮（脉冲）
- connected：连接状态显示

## 使用步骤
1. 设置 host，等待 connected 变为 true（或观察 STATUS 输出）。
2. 填写 command（或从输入端口 Command 传入）。
3. 点击 send，或给 Trigger 输入 true 触发发送。
4. 从 STATUS 查看连接状态与返回信息。

## 外部控制（可选）
- `/host`：设置 host（string）
- `/command`：设置 command（string）
- `/send`：发送一次（bool/脉冲，true 触发）
- `/connected`：连接状态反馈（bool，只读）

## 常见问题
- connected 一直为 false：确认 host/端口正确、服务端已启动、防火墙未拦截。
- 点击发送无响应：先看 STATUS 的 last_message 是否更新；必要时换一条简单命令验证链路。
