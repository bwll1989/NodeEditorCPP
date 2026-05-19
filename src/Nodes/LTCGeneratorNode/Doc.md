# LTCGeneratorNode 使用说明

## 用途
LTCGeneratorNode 用于生成 LTC 时间码音频，并从输出端口输出给下游音频播放/录制节点；同时在节点界面显示当前时间码与状态。

## 端口
- 输入（4）
  - START/STOP（端口 0）：Toggle；true 启动，false 停止
  - START（端口 1）：Trigger；true 启动（忽略 false）
  - STOP（端口 2）：Trigger；true 停止（忽略 false）
  - RESET（端口 3）：Trigger；true 归零（忽略 false）
- 输出（1）
  - AUDIO（端口 0）：AudioData（共享缓冲区）

音频输出格式：
- 单声道、float32、48000 Hz

## 界面
- 时间码显示：HH:MM:SS:FF
- 状态：Idle / Processing / Generating
- Offset：时间码偏移（用于校正显示/输出）
- Type：时间码制式（例如 PAL/NTSC）
- Volume：输出音量

## 使用步骤
1. 将 AUDIO 输出连接到 AudioDeviceOutNode 或其他音频输出/录制节点。
2. 在节点界面点击 Start（或用输入端口触发 START/START/STOP）。
3. 需要对齐时使用 Offset 调整；需要切换制式时修改 Type。

## 注意事项
- 下游播放设备需要支持 48kHz/float32。
