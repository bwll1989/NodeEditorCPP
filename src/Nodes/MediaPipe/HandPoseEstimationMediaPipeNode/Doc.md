# MediaPipe Hand Pose Estimation 节点

## 1. 节点说明

基于 [OpenCV Zoo MediaPipe Hand Pose](https://github.com/opencv/opencv_zoo/tree/main/models/handpose_estimation_mediapipe) 实现的手部关节点预测节点。采用两阶段推理：

1. **手掌检测**（`palm_detection_mediapipe_2023feb.onnx`）
2. **手部姿态估计**（`handpose_estimation_mediapipe_2023feb.onnx`）

每个检测到的手输出 **21 个 MediaPipe Hand 关键点**（屏幕坐标 + 3D 世界坐标），以及左右手分类。

模型文件请放置于 `./plugins/Models/` 目录。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入画面。
- **ENABLE**（VariableData）：启停检测。

### 输出

- **IMAGE 0**（ImageData）：带手部骨架标注的图像（可在面板关闭「绘制手部骨架」以节省 CPU）。
- **RESULT**（VariableData）：检测数据，结构见下文。

## 3. 面板参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 手掌检测阈值 | 0.6 | PalmDetector 置信度阈值 |
| NMS 阈值 | 0.3 | 手掌框非极大值抑制 |
| 手部置信度 | 0.9 | HandPoseEstimator 输出置信度过滤 |
| 最大帧率 | 15 | 推理频率上限 |
| 使用 GPU | 开 | OpenCV CUDA DNN；不可用时自动回退 CPU |
| 绘制手部骨架 | 开 | 是否在输出图像上绘制骨架 |

## 4. RESULT 数据结构

```json
{
  "count": 1,
  "width": 1920,
  "height": 1080,
  "detections": [
    {
      "confidence": 0.95,
      "handedness": 0.3,
      "handedness_label": "Left",
      "bbox": { "x1": 100, "y1": 50, "x2": 500, "y2": 900 },
      "keypoints": [ { "x": 320, "y": 240, "z": -0.01 }, ... ],
      "world_landmarks": [ { "x": 0.02, "y": -0.01, "z": 0.05 }, ... ]
    }
  ]
}
```

- `count`：检测到的手数（经 NMS 后）。
- `detections[]`：每只手一条记录。
- `confidence`：手部姿态估计置信度。
- `handedness`：左右手概率，**≤ 0.5 为左手，> 0.5 为右手**（与 OpenCV Zoo demo 一致）。
- `handedness_label`：`"Left"` 或 `"Right"`。
- `bbox`：手部包围框（像素坐标，左上 / 右下）。
- `keypoints`：屏幕空间关键点，共 **21** 个，见第 6 节。
- `world_landmarks`：世界空间关键点，共 **21** 个，见第 7 节。

## 5. keypoints 与 world_landmarks 的区别

| | **keypoints**（屏幕关键点） | **world_landmarks**（世界关键点） |
|---|---|---|
| **坐标系** | 2D 图像像素坐标 + 相对深度 | 以手腕为参考的 3D 坐标 |
| **用途** | 在画面上叠加、手势区域判断 | 3D 手势分析、手指角度计算 |
| **x, y** | 输入图像中的像素位置 | 左右 / 上下方向（模型输出尺度） |
| **z** | 相对深度，用于前后关系 | 相对手腕的深度 |
| **额外字段** | 无 | 无 |

**简要理解：**

- **`keypoints`**：点在**画面上的位置**，适合画骨架、做屏幕交互（例如「食指是否进入某区域」）。
- **`world_landmarks`**：点在**三维空间中的相对位置**，适合算指节角度、判断握拳 / 张开等姿态。

## 6. 21 个关键点索引

与 [MediaPipe Hands](https://developers.google.com/mediapipe/solutions/vision/hand_landmarker) 一致：

| 索引 | 名称 | 索引 | 名称 |
|------|------|------|------|
| 0 | WRIST（手腕） | 11 | MIDDLE_FINGER_DIP |
| 1 | THUMB_CMC | 12 | MIDDLE_FINGER_TIP |
| 2 | THUMB_MCP | 13 | RING_FINGER_MCP |
| 3 | THUMB_IP | 14 | RING_FINGER_PIP |
| 4 | THUMB_TIP | 15 | RING_FINGER_DIP |
| 5 | INDEX_FINGER_MCP | 16 | RING_FINGER_TIP |
| 6 | INDEX_FINGER_PIP | 17 | PINKY_MCP |
| 7 | INDEX_FINGER_DIP | 18 | PINKY_PIP |
| 8 | INDEX_FINGER_TIP | 19 | PINKY_DIP |
| 9 | MIDDLE_FINGER_MCP | 20 | PINKY_TIP |
| 10 | MIDDLE_FINGER_PIP | | |
![hand_keypoints.png](hand_keypoints.png)

骨架连接（与节点 overlay 一致）：

- 拇指：0→1→2→3→4
- 食指：0→5→6→7→8
- 中指：0→9→10→11→12
- 无名指：0→13→14→15→16
- 小指：0→17→18→19→20

## 7. keypoints 字段说明

每个 `keypoints[i]` 包含：

| 字段 | 含义 |
|------|------|
| `x`, `y` | 关键点在输入图像中的像素坐标 |
| `z` | 相对深度；不是米制绝对深度，表示前后相对关系 |

## 8. world_landmarks 字段说明

每个 `world_landmarks[i]` 包含：

| 字段 | 含义 |
|------|------|
| `x`, `y`, `z` | 模型估计的 3D 相对坐标（与 OpenCV Zoo `mp_handpose.py` 输出一致） |

索引与 `keypoints` 一一对应（同一索引表示同一解剖位置）。

## 9. 参考

- [OpenCV Zoo handpose_estimation_mediapipe](https://github.com/opencv/opencv_zoo/tree/main/models/handpose_estimation_mediapipe)
- [MediaPipe Hand Landmarker](https://developers.google.com/mediapipe/solutions/vision/hand_landmarker)
- [MediaPipe Hands 模型说明](https://github.com/google/mediapipe/blob/master/docs/solutions/models.md#hands)
