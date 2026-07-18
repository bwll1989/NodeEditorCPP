# Face Detection

## 1. 节点说明

使用 YOLO11n 人脸检测模型定位画面中的人脸，输出标注图与检测数据。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入图像。
- **ENABLE**（VariableData）：启停检测（也可在面板点击「启动检测」）。

### 输出

- **IMAGE 0**（ImageData）：标出人脸框的图像（可关闭绘制以降低 CPU）。
- **RESULT**（VariableData）：人脸检测结果（`count`、`detections`、`width`、`height`）。

## 3. 界面说明

- 置信度阈值、NMS 阈值
- 最大帧率（1–30 FPS，推理进行中合并为最新帧）
- 绘制检测框（关闭后仅输出 RESULT）
- 启停按钮

模型路径：`./plugins/Models/yolo11n-face-detection.onnx`

## 4. 使用说明

连接图像源后需启用检测（ENABLE 端口或面板按钮）；用 RESULT 统计人数或裁剪人脸区域。支持 OSC：`/confidence`、`/nms`、`/enable`。

## 5. 示例

Camera → Face Detection → Image Switch，多人时切换特写逻辑。
