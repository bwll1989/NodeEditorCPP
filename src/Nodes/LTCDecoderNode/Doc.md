# LTC Decoder 节点

## 1. 节点说明

从音频流中解码 LTC（线性时间码），输出时、分、秒、帧等时间信息，用于同步时间轴或显示。

## 2. 端口说明

### 输入

- **AUDIO**（AudioData）：含 LTC 信号的音频（通常为单声道线路）。

### 输出

- **FRAMES**（VariableData）：帧数。
- **SECONDS**（VariableData）：秒。
- **MINUTES**（VariableData）：分。
- **HOURS**（VariableData）：时。
- **TOTAL**（VariableData）：换算后的总帧数。

## 3. 界面说明

时间码显示面板（时:分:秒:帧）及解码状态。

## 4. 使用说明

1. 将调音台或播放器的 LTC 输出接到 Audio Device In。
2. 再接到本节点 AUDIO 端口。
3. 用各输出口驱动 TimeLine、字幕或逻辑节点。

## 5. 示例

播放机 LTC → 声卡输入 → LTC Decoder → TimeLine `/currentFrame`。
