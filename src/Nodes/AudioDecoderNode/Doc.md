# Audio Decoder 节点

## 1. 节点说明

播放媒体库中的音频文件，按声道分离输出。通道数随文件自动调整。

## 2. 端口说明

### 输入

- **PLAY**（VariableData）：播放/停止（布尔）。
- **LOOP**（VariableData）：是否循环。
- **GAIN**（VariableData）：音量（dB）。

### 输出

- **CH 0**、**CH 1**…（AudioData）：各声道，数量动态变化。

## 3. 界面说明

文件选择、播放按钮、循环、音量滑块；`/progress` 反馈播放进度。

## 4. 使用说明

1. 选择音频文件并播放。
2. 将各 CH 接到矩阵、混音或 Audio Device Out。
3. 外部：`/file`、`/volume`、`/loop`、`/play`。

## 5. 示例

Audio Decoder → Audio Matrix → Audio Device Out，多轨预览。
