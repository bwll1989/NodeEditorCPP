# Pose Detection 节点

## 1. 节点说明

基于 **YOLO11n Pose** ONNX 模型检测人体，输出 **COCO 17 关键点**骨架，支持在图像上绘制姿态，并同时输出像素坐标与归一化坐标。

模型路径：`./plugins/Models/yolo11n-pose.onnx`

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入画面。
- **ENABLE**（VariableData）：启停检测（也可在面板点击「启动检测」）。

### 输出

- **IMAGE 0**（ImageData）：带关键点与骨架标注的图像（可关闭绘制以降低 CPU）。
- **RESULT**（VariableData）：检测数据，结构见下文。

## 3. RESULT 数据结构

```json
{
  "count": 1,
  "width": 1920,
  "height": 1080,
  "detections": [
    {
      "score": 0.92,
      "confidence": 0.92,
      "box": [100, 50, 500, 900],
      "bbox": { "x1": 100, "y1": 50, "x2": 500, "y2": 900 },
      "keypoints": [ ... ],
      "world_landmarks": [ ... ]
    }
  ]
}
```

| 字段 | 说明 |
|------|------|
| `count` | 检测到的人数（NMS 后） |
| `width` / `height` | 输入图像尺寸 |
| `score` / `confidence` | 人体检测置信度（二者相同，保留 `score` 兼容旧逻辑） |
| `box` | 包围框 `[x1, y1, x2, y2]` 像素坐标（兼容旧格式） |
| `bbox` | 包围框对象 `{x1, y1, x2, y2}` |
| `keypoints` | 屏幕空间 17 关键点，见第 5 节 |
| `world_landmarks` | 以髋部为原点的归一化坐标，见第 6 节 |

## 4. keypoints 与 world_landmarks 的区别

| | **keypoints**（屏幕关键点） | **world_landmarks**（归一化关键点） |
|---|---|---|
| **坐标系** | 输入图像像素坐标 | 以双髋中点为原点、人体框尺度归一化 |
| **用途** | 画面叠加、热区判断、屏幕交互 | 姿态角度计算、跨分辨率/位置比较 |
| **x, y** | 像素位置（无效时为 `-1`） | 相对髋部的无量纲偏移（约 ±0.5 量级） |
| **z** | 无（2D 模型） | 固定为 `0`（2D 模型无深度） |
| **额外字段** | `conf`、`nx`、`ny` | 仅 `x, y, z` |
| **是否受透视/分辨率影响** | 是 | 否（同一姿态在不同分辨率下数值接近） |

**简要理解：**

- **`keypoints`**：关节在**画面上的位置**，含 `nx`/`ny`（相对图像宽高归一化到 0–1）。
- **`world_landmarks`**：关节在**以髋部为中心、人体高度/宽度为尺度**的局部坐标系中的位置，类似 MediaPipe 节点的 `world_landmarks` 用法，便于算角度和做姿态匹配。

> YOLO Pose 为 2D 模型，`world_landmarks.z` 恒为 `0`；若需真实 3D 米制坐标，请使用 MediaPipe Pose 节点。

## 5. 17 个 COCO 关键点索引

| 索引 | 名称 | 索引 | 名称 |
|------|------|------|------|
| 0 | 鼻子 (nose) | 9 | 左腕 (left wrist) |
| 1 | 左眼 (left eye) | 10 | 右腕 (right wrist) |
| 2 | 右眼 (right eye) | 11 | 左髋 (left hip) |
| 3 | 左耳 (left ear) | 12 | 右髋 (right hip) |
| 4 | 右耳 (right ear) | 13 | 左膝 (left knee) |
| 5 | 左肩 (left shoulder) | 14 | 右膝 (right knee) |
| 6 | 右肩 (right shoulder) | 15 | 左踝 (left ankle) |
| 7 | 左肘 (left elbow) | 16 | 右踝 (right ankle) |
| 8 | 右肘 (right elbow) | | |

**常用索引：** 左/右腕 9/10，左/右肩 5/6，左/右髋 11/12（`world_landmarks` 原点参考），左/右踝 15/16。

骨架连线与节点绘制逻辑一致，遵循 Ultralytics YOLO11 Pose / COCO 17 点定义。

每个 `keypoints[i]` 包含：

| 字段 | 含义 |
|------|------|
| `x`, `y` | 像素坐标；置信度不足时为 `-1` |
| `conf` | 该关键点置信度（0–1） |
| `nx`, `ny` | 相对图像宽高的归一化坐标（0–1），无效时为 `0` |

### world_landmarks 字段说明

每个 `world_landmarks[i]` 包含：

| 字段 | 含义 |
|------|------|
| `x` | 相对髋部原点的水平偏移（除以 `max(框宽, 框高)`） |
| `y` | 相对髋部原点的垂直偏移（同上） |
| `z` | 恒为 `0`（2D 模型） |

**原点计算：** 优先取左髋 (11) 与右髋 (12) 的中点；若髋部不可见，则退化为检测框中心。

**尺度：** `scale = max(框宽, 框高)`，保证不同远近下同一姿态的归一化坐标相对稳定。

## 6. 界面说明

- 置信度阈值、NMS 阈值
- 最大帧率（1–30 FPS，推理进行中合并为最新帧）
- 绘制姿态（关闭后仅输出 RESULT，可降低 CPU）
- 启停按钮

## 7. 使用说明

1. 连接视频或图像到 IMAGE，并通过 ENABLE 或面板启用检测。
2. 用 **IMAGE 0** 预览骨架；用 **RESULT** 驱动后续逻辑。
3. 屏幕交互、热区 → 读 `keypoints`（或 `nx`/`ny`）；角度 / 姿态匹配 → 读 `world_landmarks`。
4. 支持 OSC：`/confidence`、`/nms`、`/enable`。

## 8. 示例

NDI In → Pose Detection → Spout Out，互动装置骨骼可视化。
