# MTC Decoder

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

从指定 MIDI 输入设备接收 MTC（MIDI Time Code），解码为时间码输出，用于同步时间轴或显示。

同时是 **SYNC 体系的输入边界节点**（与 LTC Decoder 镜像）：把外部 MTC 时间码转换为图内同步时钟（SyncData，TC→内部秒），供 Audio/Video Decoder、时间线等节点跟随——图内同步不绑定 SMPTE 语义，TC 只在边界转换。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 0 | 否 | 0 | 0 | — | 无（数据来自 MIDI 设备） |
| 输出 | 6 | 否 | 6 | 6 | SyncData | 末端口（5）为 SYNC 同步发布 |

### 输入

无（MTC 来自系统 MIDI 输入设备，在面板下拉框中选择）。

### 输出

- **FRAMES**（VariableData）：帧数。
- **SECONDS**（VariableData）：秒。
- **MINUTES**（VariableData）：分。
- **HOURS**（VariableData）：时。
- **TOTAL**（VariableData）：按识别出的帧率换算的总帧数。
- **SYNC**（SyncData）：**图内同步时钟**——每帧把时间码换算为秒写入共享同步状态（位置=时+分+秒+帧/帧率，锚点=全局帧计数），供下游从机节点跟随；信号中断 >500ms 自动置为失效（watchdog），从机端按失联处理。

## 3. 界面与属性

时间码显示面板（时:分:秒.帧）+ 解码状态（Connected / Listening / Decoding · 帧率 / Disconnected）+ **MIDI 输入**下拉 + **偏移帧数**数值框（对解码结果整体加减偏移，-100~100）。`WidgetEmbeddable=false`，控件在右侧属性面板。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 类型 | 说明 |
|------|------|------|
| `/device` | string | 切换 MIDI 输入设备（`"索引: 名称"`） |
| `/offset` | int | 显示偏移（帧） |

## 5. 行为细节

- **解码架构**：MtcReceiver 封装 RtMidiIn，回调线程收到的原始字节经 Qt 队列信号投递到主线程 MtcDecoder 状态机；解码结果信号回节点。两种消息形态均支持：
  - **Quarter-Frame 流**（0xF1）：8 个 QF 打包 2 帧时间，前向收到 piece 3 即组帧（本组帧/秒 + 上一组分/时），反向（seek）收到 piece 7 组帧。
  - **Full-Frame SysEx**（`F0 7F 7F 01 01 hh mm ss ff F7`）：一次携带完整时间，用于定位/跳转。
- **帧率自动识别**：从 QF piece 7 高 2 位 / Full-Frame 小时字节高 2 位识别 24 / 25 / 30DF / 30 fps，识别结果显示在状态栏（如 `Decoding · 25fps`）；TOTAL 输出与 SYNC 秒换算均按识别出的帧率计算（未识别前回退 PAL 25）。
- **RtMidi 关键配置**：`ignoreTypes(false, false, true)`——midiTime 标志必须关闭，否则 MTC Quarter-Frame（0xF1）会被 RtMidi 当作时钟消息静默丢弃。
- **偏移处理**：解码帧 + 界面偏移（`timecode_frame_add`）后写入输出 0-3；SYNC 发布同样含偏移。
- **SYNC 发布（Out 5）**：TC→内部秒的输入边界换算——`秒 = 时×3600 + 分×60 + 秒 + 帧/帧率`；200ms 看门狗检测信号中断，>500ms 无新帧将同步源置为失效（跟随端 watchdog 逻辑接管），UI 状态回落 Listening。
- **状态语义**：Connected（设备已开）→ Listening（有设备但未解到 MTC）→ Decoding · 帧率（正常解码）→ Listening/Idle（信号丢失/停止）。
- **换源清理**：切换设备时解码器状态自动复位，避免上一设备的 QF 半包错组时间码。
- **持久化**：工程 JSON 顶层 `offset / device` 字段（非 `values` 子对象）。

## 6. 使用说明与典型接法

1. 用 loopMIDI（或声卡 MIDI 输入）建立虚拟 MIDI 端口；对端（DAW/播放器）向该端口发送 MTC。
2. 本节点 **MIDI 输入** 选择该端口；解码成功后状态显示 `Decoding · 帧率`。
3. 用各输出口驱动 TimeLine、字幕或逻辑节点；**SYNC 输出可接 TimeLineNode In 4、Audio/Video Decoder SYNC 输入**，外部 MTC 直接驱动图内节点同步。

## 7. 示例

**硬件/DAW 时间码跟随**：Reaper MTC 输出 → loopMIDI Port → MTC Decoder → TimeLine `/currentFrame`。

**SYNC 模式**：DAW MTC → MTC Decoder Out(SYNC) → TimeLineNode In 4（SYNC）——外部走带直接驱动时间线播放头（含启停跟随与 >500ms 失联保护）。

**Generator↔Decoder 环回自测**：MTC Generator 与 MTC Decoder 选同一 loopMIDI 端口，Generator 运行时 Decoder 即跟随显示（可用于测试与演示）。
