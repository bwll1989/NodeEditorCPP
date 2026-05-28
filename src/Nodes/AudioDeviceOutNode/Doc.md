# Audio Device Out 节点

## 1. 节点说明

将多路 AudioData 混音并输出到本机音频设备（声卡）。按全局时间戳从环形缓冲取帧，连接后自动开始播放。

## 2. 端口说明

### 输入

- **输入 0**、**输入 1**…（AudioData）：待输出的声道，默认 2 路，可增删。

### 输出

无。

## 3. 界面说明

- **驱动类型**：PortAudio 主机 API 筛选。
- **设备选择**：输出设备列表。

## 4. 使用说明

1. 选择驱动与输出设备。
2. 将解码器、麦克风等 AudioData 接到各输入端口。
3. 连接有效音频后自动出声。

## 5. 示例

Video Decoder CH1/CH2 + Audio Device In → Audio CrossFader → Audio Device Out。
