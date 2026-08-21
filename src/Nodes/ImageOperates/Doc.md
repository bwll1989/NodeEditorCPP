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

### 2.2 TimestampGenerator 驱动

所有 GPU 算子订阅 `TimestampGenerator::frameCountUpdated`（`Qt::QueuedConnection`），在 tick 内调用 `requestProcess(frameCount)`：

1. 用 `normalizeTargetTimestamp(frameCount)` 对齐输入检索帧号；
2. `resolveInputGpuFrame` / `resolveDualInputGpuFrames` 取输入纹理（静态源会回退 `getLatestFrame`）；
3. 执行 `{Name}Gpu::run(...)`；
4. `pushGpuResult` 写入输出 buffer（打**当前系统帧号**，与输入时间戳解耦）。

**Display 节点**（Image Display / Window Display）同样由 tick 拉帧；上游 buffer 清空后，tick 内 `isEmpty()` 为真时应清屏（见 BuildInNodes 实现）。

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
| `m_lastRequestedFrame` | 同帧号去重，避免 QueuedConnection 重复处理 |
| `m_lastProcessedInputTimestamp` | 输入帧未变且参数未脏时跳过 GPU |
| `m_lastPushedTimestamp` | 输出 push 去重辅助 |
| `m_paramsDirty` | 输入/参数变化后置 true，处理完或 clear 后置 false |

---

## 5. 使用建议

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

## 6. 相关文件

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

- **输入**：`IMAGE`；`H`（水平翻转）；`V`（垂直翻转）
- **输出**：`IMAGE`
- **GPU**：`ImageGpuPass::resample`，UV 镜像

---

# Image Cross

- **输入**：`A`；`B`；`BLEND`（0→A，1→B，中间线性混合）
- **输出**：`IMAGE`
- B 尺寸与 A 不一致时先 `matchTextureSize`

---

# Image Add

- **输入**：`A`；`B`
- **输出**：`IMAGE`（尺寸同 A）
- **GPU**：`clamp(A + B, 0, 1)`，类似 TD Add TOP
- B 尺寸与 A 不一致时先 `matchTextureSize`

---

# Image Subtract

- **输入**：`A`；`B`
- **输出**：`IMAGE`（尺寸同 A）
- **GPU**：`clamp(A - B, 0, 1)`，类似 TD Subtract TOP
- B 尺寸与 A 不一致时先 `matchTextureSize`

---

# Image Difference

- **输入**：`A`；`B`；`GAIN`（≥1，默认 1）
- **输出**：`IMAGE`
- **GPU**：`|A - B| * gain`，clamp 到 [0,1]

---

# Image Blur

- **输入**：`IMAGE`；`RADIUS X`；`RADIUS Y`（0–32）
- **输出**：`IMAGE`
- **GPU**：可分离高斯；半径 0 透传；X/Y 各一次 pass

---

# Image Crop

- **输入**：`IMAGE`；`LEFT %` / `RIGHT %` / `TOP %` / `BOTTOM %`（0–100）；`CROP`（0–1 `[x, y, w, h]`，与 ROI 一致）
- **CROP**：解析 `VariableData.default`，不足补 0、超出截断；写入后换算成四边百分比
- **输出**：`IMAGE`（裁剪后尺寸）
- 四边合计导致宽或高 ≤0 时输出无效纹理

---

# Image Chroma Key

- **输入**：`IMAGE`；`HUE MIN` / `HUE MAX`（0–360）；`SOFT LOW` / `SOFT HIGH`
- **输出**：`IMAGE`（BGRA，alpha 为抠像结果）
- 输入原有 alpha 与抠像 alpha 相乘

---

# Image Under

- **输入**：`A`（遮罩）；`B`（图像）
- **输出**：`IMAGE`
- **语义**：`B over A`；遮罩通道值越高，B 越不可见（可选 B/G/R/A 通道）

---

# Image Over

- **输入**：`A`（Input1 / 前景）；`B`（Input2 / 背景）
- **输出**：`IMAGE`（尺寸同 A）
- **GPU**：Porter-Duff Over，`A` 叠在 `B` 上；`A.alpha` 决定 `B` 的可见区域
- B 尺寸与 A 不一致时先 `matchTextureSize`

---

# Image Monochrome

- **输入**：`IMAGE`；`MONO`（0–255）；`RGB`；`ALPHA`
- **输出**：`IMAGE`
- **MONO**：灰度混合量，0=保留原色，255=完全黑白（对齐 TD mono 0–1）
- **RGB / ALPHA 通道选择**（索引）：
  - `0` Luminance（BT.601）
  - `1` Red、`2` Green、`3` Blue、`4` Alpha
  - `5` RGB Average、`6` RGBA Average
- **GPU**：分别按 RGB/Alpha 菜单取样灰度值，`mix(原色, 灰度, mono/255)`

---

# Image Level

- **输入**：`IMAGE`；`BRIGHTNESS`；`GAMMA`；`BLACK`；`STEP`；`OPACITY`
- **输出**：`IMAGE`
- **BRIGHTNESS**：亮度偏移 -255–255（默认 0）
- **GAMMA**：伽马 ×100，10–400 表示 0.1–4.0（默认 100 = 1.0）
- **BLACK**：黑电平 0–255，低于此值的源通道像素压到 0
- **STEP**：量化/色阶 0=关闭，1–255 对应 TD stepSize（色带数 ≈ 255/STEP）
- **OPACITY**：不透明度 0–255（默认 255）
- **Source**（属性/OSC `/source`）：0 Luminance、1 R、2 G、3 B、4 A、5 RGB Avg、6 RGBA Avg
- **GPU**：Luma Level 风格 `out.rgb = in.rgb * lookup(src)/src`，保持色相

---

# Image Channel

- **输入**：`IMAGE`；`R GAIN`；`G GAIN`；`B GAIN`；`A GAIN`
- **输出**：`IMAGE`
- **增益**（端口或属性）：-400–400，×100 为倍率（100 = 1.0，默认直通）
- **偏移**（属性/OSC）：-255–255，各通道独立常数偏移
- **GPU**：`out.ch = clamp(in.ch * gain + offset, 0, 1)`，无通道交叉混合

---

# Image Scale

- **输入**：`IMAGE`；`SIZE`（`[w, h]`）；`Width`；`Height`（像素）
- **SIZE**：解析 `VariableData.default` 为 2 维向量，不足补 0、超出截断
- **输出**：`IMAGE`
- **GPU**：`ImageGpuPass::resize`

---

# Image Threshold

- **输入**：`IMAGE`；`THRESHOLD`（0–255）；`COMPARATOR`；`SOURCE`
- **输出**：`IMAGE`（保留原 RGB；满足条件 alpha=1，否则 alpha=0）
- **SOURCE**：`0` 亮度 LUM（BT.601）、`1` R、`2` G、`3` B
- **COMPARATOR**：`0` Less、`1` Greater、`2` LessOrEqual、`3` GreaterOrEqual、`4` Equal、`5` NotEqual（默认 GreaterOrEqual）
- **THRESHOLD**：0–255，内部归一化到 [0,1] 比较（TD 阈值 0.259 ≈ 66）
- **GPU**：仅按选定通道做阈值判断，输出 alpha 遮罩，颜色来自输入

---

# Image Switch

- **输入**：动态 `IMAGE 0…N-1` + 末口 `INDEX`
- **输出**：`IMAGE`（透传选中输入的 `ImageData`，**无 GPU、无 tick**）
- `PortEditable = true`，可增删输入口
- 索引变化或选中口换源时 `dataUpdated`

---
