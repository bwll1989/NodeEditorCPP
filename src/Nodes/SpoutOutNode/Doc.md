# SpoutOutNode 使用说明

## 用途
把输入图像通过 Spout 发送给其他软件（例如 TouchDesigner、Resolume 等），作为一个 Spout Sender。

## 端口
### 输入
- IMAGE（ImageData）：要发送的图像
- ENABLE（VariableData / bool）：启用/停止发送

### 输出
无

## 参数/界面
- Sender Name：发送器名称
- Start：开始发送
- Stop：停止发送
- Status：状态显示（是否发送中）

## 使用步骤
1. 设置 Sender Name（建议唯一）。
2. 将图像连接到 IMAGE。
3. 将 ENABLE 置为 true（或点击 Start）开始发送。
4. 在接收端软件中选择该 Sender Name 作为输入源。

## 注意事项
- ENABLE 为 false 时不会发送图像。
- 更改 Sender Name 会切换到新的发送器名称；接收端需要重新选择/刷新。 
