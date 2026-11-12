# LTC Decoder

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

从音频流中解码 LTC（线性时间码），输出时、分、秒、帧等时间信息，用于同步时间轴或显示。

同时是 **SYNC 体系的输入边界节点**：把外部 SMPTE 时间码转换为图内同步时钟（SyncData，TC→内部秒），供 Audio/Video Decoder、时间线等节点跟随——图内同步不绑定 SMPTE 语义，TC 只在边界转换。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 6 | 否 | 6 | 6 | SyncData | 末端口（5）为 SYNC 同步发布 |



### 输入

- **AUDIO**（AudioData）：含 LTC 信号的音频（通常为单声道线路）。

### 输出

- **FRAMES**（VariableData）：帧数。
- **SECONDS**（VariableData）：秒。
- **MINUTES**（VariableData）：分。
- **HOURS**（VariableData）：时。
- **TOTAL**（VariableData）：换算后的总帧数。
- **SYNC**（SyncData）：**图内同步时钟**——每帧把时间码换算为秒写入共享同步状态（位置=时+分+秒+帧/帧率，锚点=全局帧计数），供下游从机节点跟随；信号中断 >500ms 自动置为失效（watchdog），从机端按失联处理。

## 3. 界面与属性

时间码显示面板（时:分:秒:帧）+ 解码状态（Connected / Decoding / Disconnected）+ **偏移帧数**数值框（对解码结果整体加减偏移，`WidgetEmbeddable=false`，控件在右侧属性面板）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），偏移量只能在属性面板设置。

## 5. 行为细节

- **解码架构**：LTCDecoderWorker 运行于独立 QThread（构造即启动），从共享音频环形缓冲取 PCM 解码；解码结果经信号回主线程。
- **输入语义**：AUDIO 端口需接 AudioData 且已连接共享缓冲；非 AudioData 或未连缓冲按断开处理（状态 Disconnected，停止解码）。
- **偏移处理**：解码帧 + 界面偏移（`timecode_frame_add`）后写入输出；输出 0-3 在每帧解码后与输入变化时刷新。
- **TOTAL 口**：按时间码类型（帧率）将 时/分/秒/帧 换算为总帧数，读取时即时计算。
- **SYNC 发布（Out 5）**：TC→内部秒的输入边界换算——`秒 = 时×3600 + 分×60 + 秒 + 帧/帧率`，与界面偏移无关（发布的是解码原始位置）；200ms 看门狗检测信号中断，>500ms 无新帧将同步源置为失效（跟随端 watchdog 逻辑接管）。
- **持久化**：工程 JSON 顶层 `offset` 字段（偏移帧数；非 `values` 子对象）。

## 6. 使用说明与典型接法

1. 将调音台或播放器的 LTC 输出接到 Audio Device In。
2. 再接到本节点 AUDIO 端口。
3. 用各输出口驱动 TimeLine、字幕或逻辑节点；**SYNC 输出可接 TimeLineNode In 4、Audio/Video Decoder SYNC 输入**，外部时间码直接驱动图内节点同步。

## 7. 示例

![img.png](img.png)
播放机 LTC → 声卡输入 → LTC Decoder → TimeLine `/currentFrame`。

**SYNC 模式**：播放机 LTC → LTC Decoder Out(SYNC) → TimeLineNode In 4（SYNC）——外部时间码直接驱动时间线播放头（含启停跟随与失联保护）。

