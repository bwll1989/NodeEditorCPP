# LTCDecoderNode 使用说明

## 用途
LTCDecoderNode 用于从音频中解析 LTC 时间码，并输出帧/秒/分/小时等数值，方便驱动时间轴、同步播放或做自动化控制。

## 端口
- 输入（1）
  - AUDIO（端口 0）：AudioData（需要连接到共享音频缓冲区的音频源）
- 输出（5）
  - FRAMES（端口 0）：帧（int）
  - SECONDS（端口 1）：秒（int）
  - MINUTES（端口 2）：分（int）
  - HOURS（端口 3）：时（int）
  - TOTAL（端口 4）：累计帧数（int）

## 界面
- 时间码显示：HH:MM:SS:FF
- 状态：Disconnected / Connected / Decoding / Processing / Idle
- Offset：时间码偏移（用于校正显示/输出）

## 使用步骤
1. 将包含 LTC 的音频源连接到 AUDIO 输入端口。
2. 观察状态变为 Connected/Decoding。
3. 将输出端口连接到需要同步的节点（例如脚本/逻辑/显示）。
4. 若时间码有偏差，用 Offset 调整。

## 注意事项
- 输入音频必须包含有效的 LTC 信号，否则输出会保持不更新或为 0。
