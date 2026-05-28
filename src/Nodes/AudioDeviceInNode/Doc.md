# Audio Device In 节点

## 1. 节点说明

从系统音频输入设备（麦克风、线路输入等）采集音频，按声道输出 AudioData。输出端口数量可编辑（默认立体声 2 路）。

## 2. 端口说明

### 输入

无。

### 输出

- **CH 0**、**CH 1**…（AudioData）：各输入声道，默认 2 路，可增删端口。

## 3. 界面说明

- **设备选择**：PortAudio 输入设备列表。
- **增益**：音量（dB）滑块。

## 4. 使用说明

1. 选择输入设备，调节增益。
2. 将各声道接到处理链或 Audio Device Out。
3. 外部：`/deviceId`、`/gain`。

选择设备后自动开始采集。

## 5. 示例

Audio Device In → LTCDecoder → TimeLine，用线路输入 LTC 驱动时间轴。
