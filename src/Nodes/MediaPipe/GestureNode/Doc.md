# Gesture（手势识别）

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

加载一个 `.gesture` 动作库文件（由独立工具 **GestureRecorder.exe** 录制），对画面中的手部做实时模板匹配，输出匹配到的手势标签。

原理：MediaPipe 手部 21 关键点（米制 `world_landmarks`）拼成 64 维特征向量（**左手自动镜像成右手等效，库只需录一只手，左右手通用**），与库内每个手势的全部样本计算余弦相似度，取最高分且过阈值者为当前手势。**零训练、零模型文件**，换手势 = 换库文件或在录制工具里补样本。

模型文件（与 HandPose 节点共用，无需额外部署）：

- `./plugins/Models/palm_detection_mediapipe_2023feb.onnx`
- `./plugins/Models/handpose_estimation_mediapipe_2023feb.onnx`

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 末端口为 ENABLE（启停） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

### 输入

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE | ImageData | 实时画面。长边超过 1280 会先缩小 |
| ENABLE | VariableData | 布尔启停。`true` 开始识别，`false` 停止。与「开始识别」按钮、OSC `/enable` 同步 |

### 输出

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE 0 | ImageData | 叠画手部骨架与当前手势标签的画面。关闭「绘制手势标签」后不刷新这一路 |
| RESULT | VariableData | 当前匹配结果。见下文 |

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板）：

- **动作库**：从媒体库 **Gestures** 分类选择 `.gesture` 文件（GestureRecorder 录制后导入媒体库即可），「重新加载」立即重读；运行时按文件名在媒体库反查全路径（兜底 `Documents/Flow/Medias`）
- **库内标签**：列表显示每个标签的样本帧数与文件阈值
- **相似度阈值（最低）**：全局叠加下限，默认 `0`（不干预，此时完全使用文件内各手势自己的阈值）
- **确认帧数**：同一标签连续匹配多少帧才输出 `confirmed=true`，默认 `3`，范围 1–30
- **最大帧率 (FPS)**：识别频率，默认 `15`，范围 1–30
- **绘制手势标签**：关闭后只更新 RESULT
- **开始识别 / 停止识别**：工程会保存该状态，下次打开时恢复

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/enable` | 读写 | 开始或停止识别（bool） |

> 相似度阈值无 OSC 绑定，只能在面板或 `.flow` 工程中设置。

## 5. RESULT 数据结构

```json
{
  "default": {
    "gesture": "fist",
    "confidence": 0.9412,
    "present": true,
    "confirmed": true,
    "threshold": 0.9
  }
}
```

| 字段 | 说明 |
|------|------|
| `gesture` | 匹配到的标签；低于阈值时为空字符串 |
| `confidence` | 与库内样本的最高余弦相似度，0–1 |
| `present` | 本帧匹配到某手势且过阈值时为 `true` |
| `confirmed` | 连续「确认帧数」都匹配同一标签时为 `true`（防抖，触发用它） |
| `threshold` | 本次生效的阈值（文件阈值与全局下限取大） |

没有检测到手时 `present=false`、`gesture` 为空。

## 6. 行为细节

- **阈值语义**：每个手势的文件内 `threshold` 优先；节点上的「相似度阈值（最低）」是全局下限，最终阈值 = max(文件阈值, 全局下限)。设为 0 即完全交给文件。录制工具预览里会实时显示「最接近的手势和相似度」，用它来定位合适的阈值：同手势相似度通常 >95%，与其它手势的混淆分值就是下限参考。
- **热重载**：外部修改库文件（例如在录制工具里另存覆盖）约 1 秒内自动生效，无需重启工程。
- **单手语义**：多只手同框时取置信度最高的一只做匹配。
- **调度**：订阅 TimestampGenerator 帧时钟，按 `maxFps` 节流；推理进行中时合并新帧，不积压。推理走 CPU。
- **停止清理**：停止识别或 IMAGE 断开时清空输出图像缓冲，RESULT 复位。
- **持久化**：`save()` 将 `libraryName`、`threshold`、`stableFrames`、`enabled`、`maxFps`、`drawOverlay` 写入 `values.*`；加载时兼容旧字段 `libraryPath`（自动取文件名走媒体库反查）。

## 7. 典型接法

1. 用 **GestureRecorder.exe**（在 Flow 的 bin 目录下）录制手势并保存为 `gestures.gesture`。
2. Camera → **Gesture**（选择 .gesture 文件）→ Condition（`confirmed` 为真且 `gesture` 等于某标签）→ 触发场景。

## 8. .gesture 文件格式

```json
{
  "format": "flow-gesture",
  "version": 1,
  "type": "hand",
  "featureDim": 64,
  "gestures": [
    { "label": "fist", "threshold": 0.9, "samples": [[64 个浮点数]] }
  ]
}
```

特征 64 维 = 21 点米制坐标展平（63）+ 左右手（1）。文件内是数值向量，不建议手改；改手势请走录制工具。

> ⚠️ **特征版本**：文件含 `featureVersion` 字段。v2（当前）已做左右手镜像归一，旧 v1 库（无此字段）加载时节点会提示重录——v1 的相似度被手性维度主导，精度不可信。
