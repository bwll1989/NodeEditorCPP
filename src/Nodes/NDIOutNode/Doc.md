# NDIOutNode 使用说明

## 用途
NDIOutNode 用于把流程中的图像（ImageData）通过 NDI 协议发送到局域网，让其他支持 NDI 的软件/设备接收（例如 OBS、vMix 等）。

## 端口
- 输入（2）
  - IMAGE（端口 0）：ImageData
  - ENABLE（端口 1）：VariableData（bool），true 开始发送，false 停止
- 输出：无

## 节点参数
- senderName：NDI 发送器名称（默认 `NodeEditor NDI Output`）
- enable：是否发送

## 外部控制（可选）
- `/senderName`：设置发送器名称（string）
- `/enable`：启动/停止发送（bool）

## 使用步骤
1. 将上游图像连接到 IMAGE。
2. 设置 senderName（可选）。
3. 把 ENABLE 设为 true（或点击 Start/Stop）。
4. 在其他 NDI 软件中选择该 senderName 作为输入源。

## 注意事项
- 需要安装并可用 NDI Runtime。
- 未启用 ENABLE 时，即使有图像输入也不会发送。
