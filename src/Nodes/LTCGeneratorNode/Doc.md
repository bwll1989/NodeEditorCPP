# LTCGeneratorNode 帮助文档

## 概述
LTCGeneratorNode 是一个 LTC（Linear Time Code）时间码生成节点，负责生成 48kHz/float32 的 LTC 音频并通过输出端口提供给下游节点，同时在界面显示当前时间码。

## 功能特性
- **LTC时间码生成**：基于 libltc 的 LTCEncoder 实时编码
- **统一时钟同步**：跟随 TimestampGenerator 的统一帧计数（默认约 23.4375fps）
- **时间码偏移**：支持时间码偏移调整功能
- **多格式支持**：支持PAL、NTSC等不同时间码格式
- **状态监控**：实时显示生成状态与当前时间码

## 输入端口
- 端口0：START/STOP（Toggle，VariableData<bool>，true 启动，false 停止）
- 端口1：START（Trigger，VariableData<bool>，true 启动，忽略 false）
- 端口2：STOP（Trigger，VariableData<bool>，true 停止，忽略 false）
- 端口3：RESET（Trigger，VariableData<bool>，true 归零，忽略 false）

## 输出端口
- **端口数量**：1个
- **端口类型**：AudioData（共享 AudioTimestampRingQueue）
- **音频格式**：单声道、float32、采样率 48000 Hz

## 界面组件
- **时间码显示**：实时显示生成的时间码（HH:MM:SS:FF格式）
- **时间码类型标签**：显示当前时间码格式（PAL/NTSC等）
- **状态标签**：显示当前生成状态
  - "Idle"：未生成
  - "Processing"：正在生成
- **偏移调整**：SpinBox控件用于调整时间码偏移值

## 技术实现
- **编码库**：libltc（Linear Timecode library）
- **帧驱动**：TimestampGenerator::frameCountUpdated 信号驱动每帧生成
- **缓冲管理**：基于 AudioTimestampRingQueue 的音频缓冲区管理
- **采样构成**：每到一帧，按 SAMPLE_RATE/帧率 生成样本块并写入缓冲

## 使用说明
1. 将节点放入图中，不需要连接输入
2. 将输出端口连接到 AudioDeviceOutNode 或其他音频处理节点
3. 通过界面实时查看生成的时间码
4. 如需修正显示时间码，可使用偏移调整功能

## 配置保存
- **偏移值**：时间码偏移设置会自动保存到项目文件中
- **时间码格式**：自动根据帧率推断（25、30、29.97、24/23.976）

## 注意事项
- 输出为 float32 单声道，采样率固定为 48000 Hz
- 帧率跟随统一时钟（默认约 23.4375fps），可在全局设置调整
- 下游播放设备需支持 48kHz/float32

## 故障排除
- **无声或噪声**：检查下游设备是否按 float32/48kHz 播放
- **时间码显示异常**：确认全局帧率设置与期望一致
- **卡顿**：检查图中是否存在耗时节点阻塞主线程或音频线程
