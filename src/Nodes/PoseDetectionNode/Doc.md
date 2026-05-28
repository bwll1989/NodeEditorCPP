# Pose Detection 节点

## 1. 节点说明

基于 YOLO11n Pose ONNX 模型检测人体关键点，在图像上绘制骨架并输出检测结果。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入画面。

### 输出

- **IMAGE 0**（ImageData）：带关键点标注的图像。
- **RESULT**（VariableData）：检测数据（框、关键点等）。

## 3. 界面说明

无额外控件；模型路径 `./plugins/Models/yolo11n-pose.onnx`。

## 4. 使用说明

1. 连接视频或图像到 IMAGE。
2. 用 IMAGE 0 预览；用 RESULT 驱动统计、触发或网络发送。

## 5. 示例

NDI In → Pose Detection → Spout Out，互动装置骨骼可视化。
