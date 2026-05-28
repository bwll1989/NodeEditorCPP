# Noise Generator 节点

## 1. 节点说明

生成测试用噪声音频（单声道），可切换噪声类型并调节音量，用于线路检测或占位。

## 2. 端口说明

### 输入

- **GAIN**（VariableData）：音量（dB）。
- **TYPE**（VariableData）：噪声类型索引。
- **START/STOP**（VariableData）：开始/停止生成（布尔）。

### 输出

- **Out**（AudioData）：噪声音频流。

## 3. 界面说明

- 启动/停止按钮。
- 噪声类型下拉框。
- 音量滑块。

## 4. 使用说明

点「启动」或通过端口/外部地址 `/generating` 开始；`/volume`、`/noiseType` 可远程调节。

## 5. 示例

Noise Generator → Audio Analysis → 检测声卡通道是否正常。
