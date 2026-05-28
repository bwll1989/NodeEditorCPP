# Video Decoder 节点

## 1. 节点说明

播放媒体库中的视频文件，输出视频帧和多路音频。输出音频通道数随文件自动增减。

## 2. 端口说明

### 输入

- **PLAY/STOP**（VariableData）：播放/暂停（布尔）。
- **STOP**（VariableData）：为 true 时停止。
- **LOOP**（VariableData）：是否循环。
- **GAIN**（VariableData）：音量（dB）。

### 输出

- **Image**（ImageData）：当前视频帧。
- **CH 1**、**CH 2**…（AudioData）：各音频声道，数量随文件变化。

## 3. 界面说明

- 文件选择框（媒体库路径）。
- 播放/停止、循环、音量滑块。

## 4. 使用说明

1. 选择视频文件。
2. 点击播放或通过 PLAY/STOP 端口控制。
3. Image 接显示或处理；CH n 接 Audio Device Out 等。

外部地址：`/fileName`、`/loop`、`/volume`、`/play`。

## 5. 示例

Video Decoder → Image + CH1/CH2 → NDI Out + 声卡输出，音视频同步播放。
