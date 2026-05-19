# FaceDetectionNode 使用说明

## 用途
FaceDetectionNode 用于对输入图像进行人脸检测，并输出：
- 带检测框的图像
- 检测结果（数量、每个框的位置与置信度）

## 端口
- 输入（1）
  - IMAGE（端口 0）：ImageData
- 输出（2）
  - IMAGE 0（端口 0）：ImageData（已绘制检测框）
  - RESULT（端口 1）：VariableData（检测结果）

RESULT 常用字段：
- `count`：检测到的人脸数量（int）
- `detections`：列表，每项包含：
  - `score`：置信度（float）
  - `box`：`[x1, y1, x2, y2]`（int）
  - `classId`：类别 ID（int）
  - `className`：类别名称（固定为 `"face"`）
- `width` / `height`：原图尺寸

## 节点参数/界面
- conf：置信度阈值
- nms：NMS 阈值

## 使用步骤
1. 将图像源连接到 IMAGE。
2. 从 IMAGE 0 获取绘制框后的图像用于显示/录制。
3. 从 RESULT 读取 detections，用于做跟踪、裁剪或触发逻辑。

## 注意事项
- 默认模型文件路径为 `./plugins/Models/yolo11n-face-detection.onnx`，缺失时将无法检测。
- 输入为空图像时不会输出结果。
