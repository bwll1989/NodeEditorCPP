# AudioCrossFaderNode 帮助文档

## 概述
AudioCrossFaderNode 用于对两路输入音频进行交叉淡化过渡（A↔B）。支持手动混合比例控制和按时长自动淡入淡出，采用等功率曲线实现恒定响度的听感。

## 功能特性
- 双输入交叉淡化：两路音频 A、B 之间平滑过渡
- 等功率曲线：使用 cos/sin 权重保持响度一致
- 自动淡入淡出：按毫秒时长执行 A→B 或 B→A
- 手动混合控制：`mix` 范围 0.0~1.0（0=A，1=B）
- Reset Mix：重新发送当前 `mixSpin` 数值到处理线程
- 时间戳同步：基于 `TimestampGenerator` 的帧级同步
- 线程安全：UI 与工作线程通过信号/槽隔离
- 单路回退：仅有一侧输入时自动直通

## 输入端口
- 端口类型：`AudioData`
- 端口数量：2（`In0`: A，`In1`: B）
- 数据格式：`AudioTimestampRingQueue` 共享缓冲区
- 位深度：支持 16 位、有符号整数；或 32 位 float
- 采样率：通常 48kHz（随上游）
- 帧对齐：按 `TimestampGenerator` 的帧计数对齐

## 输出端口
- 端口类型：`AudioData`
- 端口数量：1（混合输出）
- 数据格式：内部混合为 `float32`，时间戳随帧推进
- 处理延迟：极低（含小幅时间戳补偿）

## 界面组件
- `Mix (0=A, 1=B)`：混合比例数值框（`QDoubleSpinBox`）
- `Fade Time (ms)`：自动淡入淡出时长（毫秒）
- `Start A -> B`：启动 A→B 的自动淡入淡出
- `Start B -> A`：启动 B→A 的自动淡入淡出
- `Reset Mix`：重新发送当前 `mixSpin` 数值至工作线程

## 技术实现
- 核心类：`AudioCrossFaderWorker`（工作线程）、`AudioCrossFaderDataModel`（数据模型）、`AudioCrossFaderInterface`（界面）
- 等功率权重：`wA = cos(π·mix/2)`，`wB = sin(π·mix/2)`
- 数据路径：输入缓冲区按时间戳取帧 → 转换为 `float32` → 加权混合 → 推送到输出缓冲区
- 自动淡入淡出：基于当前帧计数与时长换算目标帧区间，线性推进 `mix`
- 线程通信：通过 `QMetaObject::invokeMethod` 使用队列连接

## 使用说明
1. 将两路音频源分别接到输入端口 A、B
2. 调整 `Mix` 数值实现手动混合；或设置 `Fade Time (ms)` 后点击 `Start A -> B` / `Start B -> A`
3. 点击 `Reset Mix` 会把当前 `Mix` 数值再次发送到工作线程（等效于重新触发 `mixSpin.valueChanged`）
4. 从输出端口获取混合后的音频

## 故障排除
- 无输出：确认至少有一侧输入处于活动状态且时间戳对齐
- 失真/响度异常：检查上游位深与采样率；避免重复归一化
- 过渡不平滑：确认 `TimestampGenerator` 正常推进帧计数

## 版本信息
- 兼容系统：Windows 10 及以上
- 依赖：Qt6、QtNodes、项目内 `TimestampGenerator`
- 建议采样率：48kHz
- 备注：项目音频解码链路使用 FFmpeg 7.1（若涉及外部解码）

