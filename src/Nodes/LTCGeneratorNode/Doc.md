# LTC Generator 节点

## 1. 节点说明

生成 LTC 时间码音频信号并输出，用于同步外部录像机、灯光台等设备。可在界面或端口控制启停与归零。

## 2. 端口说明

### 输入

- **START/STOP**（VariableData）：切换运行（true 启动，false 停止）。
- **START**（VariableData）：为 true 时启动。
- **STOP**（VariableData）：为 true 时停止。
- **RESET**（VariableData）：为 true 时时间码归零。

### 输出

- **AUDIO**（AudioData）：LTC 音频波形（48 kHz）。

## 3. 界面说明

时间码显示、帧率/制式选择、音量、开始/停止/复位按钮。

## 4. 使用说明

1. 在界面设置制式与音量，点「开始」生成。
2. 将 AUDIO 接到 Audio Device Out 或录音链路。
3. 可用触发端口远程控制启停与复位。

## 5. 示例

LTC Generator → Audio Device Out → 调音台 LTC 输入，驱动全场时间同步。
