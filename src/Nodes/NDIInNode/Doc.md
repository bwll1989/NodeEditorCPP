# NDIInNode 使用说明

## 用途
NDIInNode 用于从网络上的 NDI 发送端接收视频，并输出为 ImageData，供下游图像处理/显示/录制节点使用。

## 端口
- 输入（2）
  - SOURCE（端口 0）：VariableData（string），NDI 源名称
  - ENABLE（端口 1）：VariableData（bool），true 开始接收，false 停止
- 输出（1）
  - IMAGE（端口 0）：ImageData（接收到的视频帧）

## 节点界面
- 发送器列表：选择/输入 NDI 源名称
- Refresh：刷新发送器列表
- Start/Stop：启动/停止接收
- 状态显示：连接状态

## 外部控制（可选）
推荐使用以下地址：
- `/source`：设置 SOURCE（string）
- `/enable`：设置 ENABLE（bool）
- `/refresh`：刷新发送器列表（true/任意值触发）

兼容：
- 部分版本也会使用 `/sourceName` 写入源名称（等同于设置 SOURCE）。

## 使用步骤
1. 点击 Refresh 刷新发送器列表。
2. 选择一个 NDI 源，或手动输入完整名称。
3. 启动 ENABLE（或点击 Start/Stop）。
4. 将 IMAGE 输出连接到下游节点。

## 注意事项
- 必须安装并可用 NDI Runtime，且发送端与本机网络互通。
- 切换源名称后需要确保 ENABLE 为 true 才会继续输出画面。

