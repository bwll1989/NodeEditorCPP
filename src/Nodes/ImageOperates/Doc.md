**ImageOperates 节点包**

## 1. 概述

`ImageOperates` 是图像处理节点集合插件，提供类似 TouchDesigner TOP 系列的基础算子。除 **Image Switch** 为透传路由外，其余 GPU 算子均在 **OpenGL 纹理** 上执行，通过 `ImageTimestampRingQueue` 与上下游交换帧。

| 节点 | 类型 | 说明 |
|------|------|------|
| Image Flip | GPU | 水平/垂直翻转 |
| Image Cross | GPU | 双路线性混合 |
| Image Add | GPU | 双路像素相加 |
| Image Subtract | GPU | 双路像素相减 |
| Image Difference | GPU | 双路绝对差 |
| Image Blur | GPU | 可分离高斯模糊 |
| Image Crop | GPU | 按百分比裁剪 |
| Image Chroma Key | GPU | 色相范围抠像 |
| Image Under | GPU | 遮罩叠加（Under TOP） |
| Image Over | GPU | Alpha 前景叠底（Over TOP） |
| Image Monochrome | GPU | 彩色转灰度（Monochrome TOP） |
| Image Level | GPU | 亮度/伽马/黑电平/量化/不透明度（Luma Level TOP） |
| Image Channel | GPU | 逐通道增益/偏移（Channel Mix 精简版） |
| Image Scale | GPU | 缩放到指定分辨率 |
| Image Threshold | GPU | 阈值抠像（LUM/RGB） |
| Image Switch | 透传 | 多路输入按索引选一路输出 |

---

## 2. 架构与数据流

### 2.1 共享句柄 + 环形缓冲

```
上游 ImageData ──shared_ptr──► 算子.m_inImage
                                算子.m_outImageData ──同一对象──► 下游 Display / 下一算子
                                └─ ImageTimestampRingQueue（8 槽，按帧号索引）
```

- **ImageData** 不持有像素，只是 `ImageTimestampRingQueue` 的稳定句柄。
- **ImageFrame** 主路径仅填 `GpuTextureHandle texture` + `qint64 timestamp`；`cv::Mat image` 供 legacy CPU 节点读回。
- 下游与上游连的是**同一个** `ImageData` 指针；内容变化发生在 ring buffer 内部。

### 2.2 环形队列唤醒（对齐 AudioGateWorker）

GPU 算子由 `DirtyTickCook`（`ImageOperateCookDriver.hpp`）驱动，**不**订阅全局 `frameCountUpdated`：

1. 上游 `ImageTimestampRingQueue::pushFrame` → `registerFrameWaiter` / `notifyFrameWaiters`（`AudioTickWaiter`）；
2. cook 线程 `wait` 被唤醒；多路输入注册同一 waiter，`pending_` 合并多次 notify；
3. `QueuedConnection` 回到 GUI 调用 `requestProcess`（`ImageGpuUpload` 须 GUI 线程）；
4. `resolveInputGpuFrameLatest` 取最新输入纹理 → `{Name}Gpu::run` → `pushGpuResult`（输出 stamp 沿用输入 timestamp）。

参数变更置 `m_paramsDirty`；cook 线程 20ms 超时轮询或下次 push 时结算。

**Display 节点**仍由 tick 按 `clock` `getFrameByTimestamp` 拉帧（音画对齐）；未命中回退 `getLatestFrame`。

### 2.3 GPU 基础设施

| 组件 | 路径 | 职责 |
|------|------|------|
| `ImageGpuPass` | `Common/DataTypes/ImageGpuPass.*` | FBO、`runFragmentPass`、`resize`、`resample` |
| `{Name}Gpu` | 各节点 `.hpp` 内 | 本节点 shader 源码 + `run()` |
| `ImageOperateCommon.hpp` | 本目录 | 取帧、双输入对齐、`pushGpuResult` 等 |

Shader 使用 **GLSL 1.x**（Compatibility Profile）：`texture2D`、`varying`、`gl_FragColor`；**不支持** `abs(int)` 等仅 float 的内建重载。

---

## 3. 节点开发约定（GPU 算子模板）

以 `FlipImageOperateModel.hpp` 为参考实现。

### 3.1 setInData

| 端口 | 行为 |
|------|------|
| IMAGE（0） | 保存 `m_inImage`；`ensureSharedOutput`；`m_lastProcessedInputTimestamp = -1`；`m_paramsDirty = true`。**不在此 clearOutput / requestProcess**。 |
| 参数口 | 调用对应 setter（setter 内只设 `m_paramsDirty`）。 |

断开上游时图编辑器会 `setInData(nullptr)`，清空在**下一 tick** 的 `requestProcess` 中通过 `clearOutput()` 完成。

### 3.2 参数 setter

仅当值变化时：`m_paramsDirty = true` + 发 property 信号。**不**主动跑 GPU。

### 3.3 tick → requestProcess

```text
ensureSharedOutput
├─ 无输入 / 输入 buffer 空     → clearOutput()
├─ resolveInputGpuFrame 失败
│   ├─ hasInputImage == false   → clearOutput()   // 真正无源
│   └─ 否则                     → 保留现有输出      // 重连瞬间防误清
├─ !m_paramsDirty && 输入帧未变 → return          // 静态图跳过
└─ {Name}Gpu::run → pushGpuResult → 更新去重状态
```

### 3.4 clearOutput

- `m_outBuffer->clear()`（发出 `bufferCleared`）
- 重置 `m_lastPushedTimestamp`、`m_lastProcessedInputTimestamp`（双输入还有 B）
- `m_paramsDirty = false`

Display 侧应在 tick 发现 `isEmpty()` 后直接 `clearTexture()`，不依赖 `dataUpdated`。

### 3.5 成员状态

| 成员 | 含义 |
|------|------|
| `m_cook` | `DirtyTickCook`：ring push / `markDirty` 唤醒 → GUI `requestProcess` |
| `m_lastProcessedInputTimestamp` | 输入帧未变且参数未脏时跳过 GPU |
| `m_lastPushedTimestamp` | 记录上次输出 stamp（参数变更同 stamp 仍会再 push） |
| `m_paramsDirty` | 输入/参数变化后置 true，处理完或 clear 后置 false；变更须同时 `m_cook->markDirty()` |

---

## 4. 使用建议

- **Flip**：镜像、纹理方向修正
- **Cross**：A/B 淡入淡出
- **Add**：亮度叠加、光效合成
- **Subtract**：背景扣除、参考减除
- **Difference**：帧差、参考图对比
- **Blur**：柔化、背景模糊（半径 ≤32）
- **Crop**：安全区、局部取景
- **Chroma Key**：绿幕/蓝幕
- **Under**：遮罩贴图、alpha 合成
- **Over**：前景叠底、带 alpha 图层合成
- **Monochrome**：去色、黑白化
- **Level**：亮度/对比/伽马/色阶调整
- **Channel**：单通道增益/偏移（去红、提 Alpha 等）
- **Scale**：统一分辨率后再进双输入算子
- **Threshold**：亮度/通道阈值抠像，输出 alpha 遮罩
- **Switch**：多机位/多素材切换

### 典型链路

```text
Image Loader / Video Decoder → Image Flip → Image Display
Image Loader (背景) ──┐
Image Loader (前景) ──┴→ Image Cross → Window Display
```

静态源（Image Loader）帧时间戳可能落后于当前 tick；算子通过 `resolveInputFrameForOperate` 回退最新帧，避免合成被误清空。

---

## 5. 相关文件

| 文件 | 说明 |
|------|------|
| `ImageOperateCommon.hpp` | 插件内公共辅助 API |
| `PluginDefinition.cpp` | 节点注册表 |
| `FlipImageOperateModel.hpp` | GPU 算子实现模板（首选阅读） |
| `Common/DataTypes/ImageGpuPass.*` | GPU pass 基础设施 |
| `Common/DataTypes/ImageTimestampRingQueue.*` | 环形帧队列 |
| `Nodes/BuildInNodes/ImageShowModel.cpp` | 嵌入式显示 tick 刷新 |

---

# Image Flip

## 1. 节点说明

水平/垂直镜像翻转（类似 TD Flip TOP）。典型用途：镜像、纹理方向修正、舞台投影反装补偿。

GPU 算子的通用行为（本插件所有算子一致）：图像口断开或无帧时输出被清空；参数口断开时参数重置为默认值；输出帧时间戳沿用输入；输入帧与参数均未变化时跳过 GPU；参数/OSC 地址前缀为 `/dataflow/{别名}/{节点ID}/…`。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| H | VariableData | 否 | 水平翻转 bool；断开时：置 false |
| V | VariableData | 否 | 垂直翻转 bool；断开时：置 false |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 翻转结果，尺寸不变，随输入帧连续更新 |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| horizontal | bool | true | true/false | 水平镜像，对应 OSC `/horizontal` |
| vertical | bool | false | true/false | 垂直镜像，对应 OSC `/vertical` |

无内嵌界面控件。两个属性均保存到工程并随 load 恢复。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/horizontal` | 可写 | 设置水平镜像（bool，非 0 为真） |
| `/vertical` | 可写 | 设置垂直镜像（bool，非 0 为真） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行翻转 pass；输出帧时间戳沿用输入帧。
- **参数生效**：属性/OSC 写入仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **断开行为**：图像口断开或无帧时输出被清空；H/V 参数口断开时对应镜像重置为 false。
- **持久化**：`values` 子对象含 `horizontal`、`vertical`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Cross

## 1. 节点说明

双路线性混合（类似 TD Cross TOP）：按混合系数在 A、B 之间淡入淡出。典型用途：场景过渡、双层素材叠化。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A | ImageData | 是 | 混合系数 0 时的图像；断开时：输出被清空 |
| B | ImageData | 是 | 混合系数 1 时的图像；尺寸与 A 不一致时自动缩放对齐到 A |
| BLEND | VariableData | 否 | 混合系数 float 0–1（0→A，1→B，中间线性混合）；断开时：重置为 0（输出 A） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 混合结果（尺寸同 A），随输入或参数变化更新 |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| blend | double | 0 | 0–1（越界截断） | 混合系数，对应 OSC `/blend` |

无内嵌界面控件。blend 保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/blend` | 可写 | 设置混合系数（0–1，越界截断） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行混合 pass；输出帧时间戳沿用 A 输入帧。
- **参数生效**：属性/OSC 写入仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **双输入对齐**：B 与 A 尺寸不一致时自动缩放到 A 再混合；B 断开时输出被清空。
- **断开行为**：BLEND 参数口断开时 blend 重置为 0（输出 A）。
- **持久化**：`values` 子对象含 `blend`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Add

## 1. 节点说明

双路像素相加（类似 TD Add TOP）：`clamp(A + B, 0, 1)`。典型用途：亮度叠加、光效合成。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A | ImageData | 是 | 底图；断开时：输出被清空 |
| B | ImageData | 是 | 叠加图；尺寸与 A 不一致时自动缩放对齐到 A；断开时：输出被清空 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 逐像素相加并 clamp 到 [0,1] 的结果（尺寸同 A） |

## 3. 界面与属性

本节点无可配置属性，无内嵌界面控件，无外部控制地址。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），不能通过 OSC/网页等方式远程控制；无可调参数。

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行相加 pass；输出帧时间戳沿用 A 输入帧。
- **双输入对齐**：B 与 A 尺寸不一致时自动缩放到 A 再相加；A 或 B 任一断开时输出被清空。
- **静态跳过**：输入帧与参数均未变化时跳过 GPU（本节点无参数，仅帧去重）。
- **持久化**：无参数可保存（save/load 为空实现）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Subtract

## 1. 节点说明

双路像素相减（类似 TD Subtract TOP）：`clamp(A - B, 0, 1)`。典型用途：背景扣除、参考减除。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A | ImageData | 是 | 被减图；断开时：输出被清空 |
| B | ImageData | 是 | 减图；尺寸与 A 不一致时自动缩放对齐到 A；断开时：输出被清空 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 逐像素相减并 clamp 到 [0,1] 的结果（尺寸同 A） |

## 3. 界面与属性

本节点无可配置属性，无内嵌界面控件，无外部控制地址。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），不能通过 OSC/网页等方式远程控制；无可调参数。

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行相减 pass；输出帧时间戳沿用 A 输入帧。
- **双输入对齐**：B 与 A 尺寸不一致时自动缩放到 A 再相减；A 或 B 任一断开时输出被清空。
- **静态跳过**：输入帧与参数均未变化时跳过 GPU（本节点无参数，仅帧去重）。
- **持久化**：无参数可保存（save/load 为空实现）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Difference

## 1. 节点说明

双路绝对差（类似 TD Difference TOP）：`|A - B| × gain` 后 clamp 到 [0,1]。典型用途：帧差检测、参考图对比、变化区域提取。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A | ImageData | 是 | 参考图；断开时：输出被清空 |
| B | ImageData | 是 | 当前图；尺寸与 A 不一致时自动缩放对齐到 A |
| GAIN | VariableData | 否 | 差值放大倍数 int ≥1，默认 1；断开时：重置为 1 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 绝对差放大结果（尺寸同 A） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| gain | int | 1 | ≥1 | 差值增益，对应 OSC `/gain` |

无内嵌界面控件。gain 保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/gain` | 可写 | 设置差值放大倍数（int，≥1） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行绝对差 pass；输出帧时间戳沿用 A 输入帧。
- **参数生效**：属性/OSC 写入仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **双输入对齐**：B 与 A 尺寸不一致时自动缩放到 A；B 断开时输出被清空。
- **断开行为**：GAIN 参数口断开时 gain 重置为 1。
- **持久化**：`values` 子对象含 `gain`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Blur

## 1. 节点说明

可分离高斯模糊（X/Y 各一次 pass），半径 0 时透传。典型用途：柔化、背景虚化、辉光预处理。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| RADIUS X | VariableData | 否 | 水平模糊半径 int 0–32；断开时：重置为 8 |
| RADIUS Y | VariableData | 否 | 垂直模糊半径 int 0–32；断开时：重置为 8 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 模糊结果（尺寸不变） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| radiusX | int | 8 | 0–32 | 水平半径，对应 OSC `/radiusX` |
| radiusY | int | 8 | 0–32 | 垂直半径，对应 OSC `/radiusY` |

无内嵌界面控件。半径 0 透传；两属性均保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/radiusX` | 可写 | 设置水平模糊半径（0–32，越界截断） |
| `/radiusY` | 可写 | 设置垂直模糊半径（0–32，越界截断） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行模糊 pass；输出帧时间戳沿用输入帧。
- **两趟 pass**：X / Y 方向各执行一次可分离高斯卷积；两半径均为 0 时透传（跳过 GPU）。
- **参数生效**：属性/OSC 写入仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **断开行为**：图像口断开时输出被清空；RADIUS X/Y 参数口断开时对应半径重置为 8。
- **持久化**：`values` 子对象含 `radiusX`、`radiusY`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Crop

## 1. 节点说明

按四边百分比裁剪（GPU 重采样取子区）。典型用途：安全区裁剪、局部取景、多屏切边。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 6 | 否 | 6 | 6 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| LEFT % | VariableData | 否 | 左边裁剪百分比 0–100；断开时：重置为 0 |
| RIGHT % | VariableData | 否 | 右边裁剪百分比 0–100；断开时：重置为 0 |
| TOP % | VariableData | 否 | 顶边裁剪百分比 0–100；断开时：重置为 0 |
| BOTTOM % | VariableData | 否 | 底边裁剪百分比 0–100；断开时：重置为 0 |
| CROP | VariableData | 否 | 一次写入四边 `[x, y, w, h]`（与 ROI 节点一致）；解析 VariableData，不足补 0、超出截断；写入后换算为四边百分比 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 裁剪后图像；四边合计导致宽或高 ≤0 时输出无效纹理 |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| leftPercent / rightPercent / topPercent / bottomPercent | double | 0 | 0–100 | 四边裁剪百分比，对应 OSC `/left` `/right` `/top` `/bottom` |

无内嵌界面控件。四边百分比均保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/left` | 可写 | 设置左边裁剪百分比（0–100） |
| `/right` | 可写 | 设置右边裁剪百分比（0–100） |
| `/top` | 可写 | 设置顶边裁剪百分比（0–100） |
| `/bottom` | 可写 | 设置底边裁剪百分比（0–100，注意地址无 Percent 后缀） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行 GPU 子区重采样；输出帧时间戳沿用输入帧。
- **CROP 复合口**：一次写入 `[x, y, w, h]`（像素，与 ROI 节点一致），解析 VariableData 后换算为四边百分比（不足补 0、超出截断）。
- **边界行为**：四边合计使宽或高 ≤ 0 时输出无效纹理；参数口断开时对应边重置为 0。
- **参数生效**：属性/OSC 写入仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **持久化**：`values` 子对象含 `leftPercent`、`rightPercent`、`topPercent`、`bottomPercent`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Chroma Key

## 1. 节点说明

色相范围抠像（绿幕/蓝幕）：对输入做 HSV 色相区间判断，输出 alpha 为抠像结果（BGRA）。输入原有 alpha 与抠像 alpha 相乘。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| HUE MIN | VariableData | 否 | 抠像色相下限 0–360；断开时：重置为 80 |
| HUE MAX | VariableData | 否 | 抠像色相上限 0–360；断开时：重置为 160 |
| SOFT LOW | VariableData | 否 | 色相下缘柔化宽度；断开时：重置为 10 |
| SOFT HIGH | VariableData | 否 | 色相上缘柔化宽度；断开时：重置为 10 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | BGRA 结果，alpha 为抠像遮罩（色相区间内→透明） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| hueMin | double | 80 | 0–360 | 色相下限，对应 OSC `/huemin` |
| hueMax | double | 160 | 0–360 | 色相上限，对应 OSC `/huemax` |
| hueSoftLow | double | 10 | 0–360 | 下缘柔化，对应 OSC `/hsoftlow` |
| hueSoftHigh | double | 10 | 0–360 | 上缘柔化，对应 OSC `/hsofthigh` |

无内嵌界面控件。四个属性均保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/huemin` | 可写 | 设置抠像色相下限（0–360） |
| `/huemax` | 可写 | 设置抠像色相上限（0–360） |
| `/hsoftlow` | 可写 | 设置色相下缘柔化宽度 |
| `/hsofthigh` | 可写 | 设置色相上缘柔化宽度 |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行 HSV 抠像 pass；输出帧时间戳沿用输入帧。
- **抠像语义**：色相落在 [hueMin, hueMax] 区间内的像素 alpha → 0（透明），区间外 → 1；上下缘柔化宽度内线性过渡。
- **alpha 相乘**：输出 alpha = 输入原 alpha × 抠像 alpha。
- **断开行为**：图像口断开时输出被清空；四个参数口断开时分别重置为 80 / 160 / 10 / 10。
- **持久化**：`values` 子对象含 `hueMin`、`hueMax`、`hueSoftLow`、`hueSoftHigh`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Under

## 1. 节点说明

遮罩叠加（Under TOP 语义：`B over A`）：遮罩通道值越高，B 越不可见。典型用途：遮罩贴图、alpha 合成。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A | ImageData | 是 | 遮罩图（按所选通道取样）；断开时：输出被清空 |
| B | ImageData | 是 | 被叠加图像；尺寸与 A 不一致时自动缩放对齐；断开时：输出被清空 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 合成结果（尺寸同 A） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| maskChannel | 枚举 | Alpha | B/G/R/A | 取样 A 的哪个通道作遮罩，对应 OSC `/maskChannel` |

无内嵌界面控件。maskChannel 保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/maskChannel` | 可写 | 设置遮罩取样通道（B/G/R/A 枚举） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行 Under 合成 pass；输出帧时间戳沿用 A 输入帧。
- **合成语义**：按所选通道取样 A 作遮罩——通道值越高，B 越不可见（Under TOP 的 `B over A` 语义）；B 与 A 尺寸不一致时自动缩放对齐。
- **枚举桥接**：`maskChannelIndex` 为内部桥接辅助属性（DESIGNABLE false），无独立 OSC 地址。
- **断开行为**：A 或 B 断开时输出被清空；maskChannel 参数口断开时重置为 Alpha。
- **持久化**：`values` 子对象含 `maskChannel`（int 枚举）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Over

## 1. 节点说明

Alpha 前景叠底（Porter-Duff Over，Over TOP 语义）：A 叠在 B 上，`A.alpha` 决定 B 的可见区域。典型用途：带 alpha 的前景图层合成、字幕/台标叠加。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| A | ImageData | 是 | 前景（Input1）；断开时：输出被清空 |
| B | ImageData | 是 | 背景（Input2）；尺寸与 A 不一致时自动缩放对齐到 A |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | Over 合成结果（尺寸同 A） |

## 3. 界面与属性

本节点无可配置属性，无内嵌界面控件，无外部控制地址。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），不能通过 OSC/网页等方式远程控制；无可调参数。

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行 Over 合成 pass；输出帧时间戳沿用 A 输入帧。
- **合成语义**：Porter-Duff Over——结果 = A.rgb × A.alpha + B.rgb × (1 − A.alpha)；A 的 alpha 决定 B 的透出程度。
- **双输入对齐**：B 与 A 尺寸不一致时自动缩放到 A；A 或 B 任一断开时输出被清空。
- **持久化**：无参数可保存（save/load 为空实现）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Monochrome

## 1. 节点说明

彩色转灰度（Monochrome TOP）：按选定通道取样灰度值，与原色按 mono 量混合。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 4 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| MONO | VariableData | 否 | 灰度混合量 int 0–255（0=保留原色，255=完全黑白，对齐 TD mono 0–1）；断开时：重置为 255 |
| RGB | VariableData | 否 | RGB 灰度取样源索引（0=Luminance BT.601、1=R、2=G、3=B、4=A、5=RGB Average、6=RGBA Average）；断开时：重置为 0 |
| ALPHA | VariableData | 否 | Alpha 灰度取样源索引（同上）；断开时：重置为 0 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | `mix(原色, 灰度, mono/255)` 结果（尺寸不变） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| mono | int | 255 | 0–255 | 灰度混合量，对应 OSC `/mono` |
| rgbSource | 枚举 | Luminance | Luminance/R/G/B/A/RGB Avg/RGBA Avg | RGB 取样源，对应 OSC `/rgbSource` |
| alphaSource | 枚举 | Luminance | 同上 | Alpha 取样源，对应 OSC `/alphaSource` |

无内嵌界面控件。三个属性均保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/mono` | 可写 | 设置灰度混合量（0–255，0=保留原色，255=完全黑白） |
| `/rgbSource` | 可写 | 设置 RGB 灰度取样源（0 Luminance … 6 RGBA Avg） |
| `/alphaSource` | 可写 | 设置 Alpha 灰度取样源（同上） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行灰度混合 pass；输出帧时间戳沿用输入帧。
- **混合语义**：`mix(原色, 灰度, mono/255)`——mono=0 完全保留原色，255 完全黑白；取样源支持 Luminance(BT.601)/R/G/B/A/RGB Avg/RGBA Avg 共 7 种。
- **枚举桥接**：`rgbSourceIndex` / `alphaSourceIndex` 为内部桥接辅助属性（DESIGNABLE false），无独立 OSC 地址。
- **断开行为**：MONO 断开重置 255，RGB/ALPHA 断开重置 0（Luminance）。
- **持久化**：`values` 子对象含 `mono`、`rgbSource`、`alphaSource`（int 枚举）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Level

## 1. 节点说明

亮度/伽马/黑电平/量化/不透明度调整（Luma Level TOP 风格）：`out.rgb = in.rgb * lookup(src)/src`，保持色相。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 6 | 否 | 6 | 6 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| BRIGHTNESS | VariableData | 否 | 亮度偏移 int -255–255；断开时：重置为 0 |
| GAMMA | VariableData | 否 | 伽马 ×100（10–400 表示 0.1–4.0，100 = 1.0）；断开时：重置为 100 |
| BLACK | VariableData | 否 | 黑电平 int 0–255，低于此值的源通道像素压到 0；断开时：重置为 0 |
| STEP | VariableData | 否 | 量化/色阶 int，0=关闭，1–255 对应 TD stepSize（色带数 ≈ 255/STEP）；断开时：重置为 0 |
| OPACITY | VariableData | 否 | 不透明度 int 0–255；断开时：重置为 255 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 调整结果（尺寸不变） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| brightness | int | 0 | -255–255 | 亮度偏移，对应 OSC `/brightness` |
| gamma | int | 100 | 10–400 | 伽马 ×100，对应 OSC `/gamma` |
| blackLevel | int | 0 | 0–255 | 黑电平，对应 OSC `/blackLevel` |
| stepSize | int | 0 | 0–255 | 量化步长（0=关闭），对应 OSC `/stepSize` |
| opacity | int | 255 | 0–255 | 不透明度，对应 OSC `/opacity` |
| source | 枚举 | Luminance | 0 Luminance、1 R、2 G、3 B、4 A、5 RGB Avg、6 RGBA Avg | 亮度取样源，对应 OSC `/source` |

无内嵌界面控件。全部属性保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/brightness` | 可写 | 设置亮度偏移（-255–255） |
| `/gamma` | 可写 | 设置伽马 ×100（10–400，100 = 1.0） |
| `/blackLevel` | 可写 | 设置黑电平（0–255） |
| `/stepSize` | 可写 | 设置量化步长（0=关闭，1–255） |
| `/opacity` | 可写 | 设置不透明度（0–255） |
| `/source` | 可写 | 设置亮度取样源（0 Luminance … 6 RGBA Avg） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行 Luma Level pass；输出帧时间戳沿用输入帧。
- **计算语义**：`out.rgb = in.rgb × lookup(src)/src`——按取样源构建亮度查找表缩放 RGB，保持色相；黑电平以下的源像素压到 0；stepSize 量化产生色带（色带数 ≈ 255/stepSize）。
- **枚举桥接**：`sourceIndex` 为内部桥接辅助属性（DESIGNABLE false），无独立 OSC 地址。
- **断开行为**：图像口断开时输出被清空；各参数口断开时重置为默认值（0 / 100 / 0 / 0 / 255 / Luminance）。
- **持久化**：`values` 子对象含 `blackLevel`、`brightness`、`gamma`、`stepSize`、`opacity`、`source`（int 枚举）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Channel

## 1. 节点说明

逐通道增益/偏移（Channel Mix 精简版）：`out.ch = clamp(in.ch × gain + offset, 0, 1)`，无通道交叉混合。典型用途：去红、提 Alpha、单通道调色。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| R GAIN | VariableData | 否 | 红通道增益 int -400–400（×100 为倍率，100 = 1.0）；断开时：重置为 100 |
| G GAIN | VariableData | 否 | 绿通道增益；同上；断开时：重置为 100 |
| B GAIN | VariableData | 否 | 蓝通道增益；同上；断开时：重置为 100 |
| A GAIN | VariableData | 否 | Alpha 通道增益；同上；断开时：重置为 100 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 逐通道调整结果（尺寸不变） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| redGain / greenGain / blueGain / alphaGain | int | 100 | -400–400 | 四通道增益，对应 OSC `/redGain` `/greenGain` `/blueGain` `/alphaGain` |
| redOffset / greenOffset / blueOffset / alphaOffset | int | 0 | -255–255 | 四通道常数偏移，对应 OSC `/redOffset` `/greenOffset` `/blueOffset` `/alphaOffset` |

无内嵌界面控件。八个属性均保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/redGain` | 可写 | 设置红通道增益（-400–400，×100 为倍率） |
| `/greenGain` | 可写 | 设置绿通道增益（同上） |
| `/blueGain` | 可写 | 设置蓝通道增益（同上） |
| `/alphaGain` | 可写 | 设置 Alpha 通道增益（同上） |
| `/redOffset` | 可写 | 设置红通道偏移（-255–255） |
| `/greenOffset` | 可写 | 设置绿通道偏移（同上） |
| `/blueOffset` | 可写 | 设置蓝通道偏移（同上） |
| `/alphaOffset` | 可写 | 设置 Alpha 通道偏移（同上） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行逐通道调整 pass；输出帧时间戳沿用输入帧。
- **计算公式**：`out.ch = clamp(in.ch × gain/100 + offset/255, 0, 1)`，无通道交叉混合。
- **参数生效**：属性/OSC 写入经 setter（范围截断、值不变不触发）仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **断开行为**：图像口断开时输出被清空；四个 GAIN 口断开时重置为 100（×1.0）。
- **持久化**：`values` 子对象含 `redGain`、`greenGain`、`blueGain`、`alphaGain`、`redOffset`、`greenOffset`、`blueOffset`、`alphaOffset`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Scale

## 1. 节点说明

缩放到指定分辨率（GPU `ImageGpuPass::resize`）。典型用途：统一分辨率后再进双输入算子、输出规格化。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 4 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| SIZE | VariableData | 否 | 一次写入目标尺寸 `[w, h]`；解析 VariableData 为 2 维向量，不足补 0、超出截断 |
| Width | VariableData | 否 | 目标宽（像素）；断开时：重置为 0 |
| Height | VariableData | 否 | 目标高（像素）；断开时：重置为 0 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 缩放结果；宽或高为 0 时输出无效纹理 |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| width | int | 0 | ≥0 | 目标宽，对应 OSC `/width` |
| height | int | 0 | ≥0 | 目标高，对应 OSC `/height` |

无内嵌界面控件。两属性保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/width` | 可写 | 设置目标宽（像素，≥0） |
| `/height` | 可写 | 设置目标高（像素，≥0） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行 `ImageGpuPass::resize`；输出帧时间戳沿用输入帧。
- **SIZE 复合口**：一次写入 `[w, h]`（解析 VariableData 为 2 维向量，不足补 0、超出截断），等效于同时写 Width/Height。
- **边界行为**：宽或高为 0 时输出无效纹理；参数口断开时重置为 0。
- **参数生效**：属性/OSC 写入仅置脏标记，与下一次输入帧一起生效；输入帧与参数均未变化时跳过 GPU。
- **持久化**：特殊——保存为顶层 `size` 子对象 `{width, height}`（非 `values` 包裹）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Threshold

## 1. 节点说明

阈值抠像（LUM/RGB）：仅按选定通道做阈值判断，输出 alpha 遮罩（满足条件 alpha=1，否则 0），颜色保留输入原值。典型用途：亮度抠像、生成遮罩。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 4 | — | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE | ImageData | 是 | 源图像；断开时：输出被清空 |
| THRESHOLD | VariableData | 否 | 阈值 int 0–255（内部归一化到 [0,1] 比较，TD 阈值 0.259 ≈ 66）；断开时：重置为 128 |
| COMPARATOR | VariableData | 否 | 比较方式索引：0 Less、1 Greater、2 LessOrEqual、3 GreaterOrEqual、4 Equal、5 NotEqual；断开时：重置为 3（GreaterOrEqual） |
| SOURCE | VariableData | 否 | 取样通道索引：0 亮度 LUM（BT.601）、1 R、2 G、3 B；断开时：重置为 0 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | alpha 遮罩结果（RGB 保留输入，alpha 0/1） |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| thresh | int | 128 | 0–255 | 阈值，对应 OSC `/thresh` |
| comparator | 枚举 | GreaterOrEqual | Less/Greater/LessOrEqual/GreaterOrEqual/Equal/NotEqual | 比较方式，对应 OSC `/comparator` |
| source | 枚举 | Luminance | Luminance/R/G/B | 取样通道，对应 OSC `/source` |

无内嵌界面控件。全部属性保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/thresh` | 可写 | 设置阈值（0–255） |
| `/comparator` | 可写 | 设置比较方式（0 Less、1 Greater、2 LessOrEqual、3 GreaterOrEqual、4 Equal、5 NotEqual） |
| `/source` | 可写 | 设置取样通道（0 Luminance、1 R、2 G、3 B） |

## 5. 行为细节

- **处理链路**：cook 线程由输入环形队列帧推送唤醒（20 ms 超时兜底），Queued 调用回 GUI 线程执行阈值 pass；输出帧时间戳沿用输入帧。
- **抠像语义**：仅按选定通道（亮度 LUM BT.601 / R / G / B）取值与阈值比较（内部归一化到 [0,1]，TD 阈值 0.259 ≈ 66）；满足条件 alpha=1，否则 0；RGB 保留输入原值。
- **枚举桥接**：`comparatorIndex` / `sourceIndex` 为内部桥接辅助属性（DESIGNABLE false），无独立 OSC 地址。
- **断开行为**：图像口断开时输出被清空；THRESHOLD/COMPARATOR/SOURCE 断开时分别重置为 128 / GreaterOrEqual / Luminance。
- **持久化**：`values` 子对象含 `thresh`、`comparator`、`source`（int 枚举）。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。

# Image Switch

## 1. 节点说明

多路输入按索引选一路透传输出（**无 GPU、无 tick**，选中即透传 `ImageData`）。典型用途：多机位/多素材切换。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 是 | 1 | 不限 | ImageData | 末端口为 INDEX（选择索引，VariableData，始终位于最后） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 必接 | 说明 |
|------|------|------|------|
| IMAGE 0…N-1 | ImageData | 条件 | 候选图像；至少接一路；端口可编辑（可增删） |
| INDEX | VariableData | 否 | 选中索引 int（末端口）；越界时：输出被清空；断开时：重置为 0 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IMAGE | ImageData | 选中输入的原 `ImageData`（透传，不重采样）；索引变化或选中口换源时触发 dataUpdated |

## 3. 界面与属性

| 属性 | 类型 | 默认值 | 取值范围 | 说明 |
|------|------|--------|----------|------|
| index | int | 0 | 0–输入数-1 | 选中索引，对应 OSC `/index` |

无内嵌界面控件。index 保存到工程。

---

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/index` | 可写 | 设置选中输入索引（int，越界时输出被清空） |

## 5. 行为细节

- **透传语义**：本节点**无 GPU、无 tick、无 cook**——选中即透传原 `ImageData` 句柄，不重采样、不改时间戳；索引变化或选中口换源时触发 `dataUpdated`。
- **端口布局**：图像端口可编辑（1…任意，走端口插入/删除事件），INDEX 恒为末端口。
- **越界行为**：index 越界时输出被清空；INDEX 口断开时重置为 0。
- **参数生效**：属性/OSC 写入经 setter 同步生效（无脏标记、无帧缓存）。
- **持久化**：`values` 子对象含 `index`。

## 6. 使用说明与典型接法

基本用法：将上游节点输出接入本节点输入端口，本节点输出端口连接下游节点的对应输入。
