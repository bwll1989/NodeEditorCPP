# Face Detection 节点

## 1. 节点说明

使用 YOLO11n 人脸检测模型定位画面中的人脸，输出标注图与检测数据。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入图像。

### 输出

- **IMAGE 0**（ImageData）：标出人脸框的图像。
- **RESULT**（VariableData）：人脸检测结果。

## 3. 界面说明

无专用面板；模型 `./plugins/Models/yolo11n-face-detection.onnx`。

## 4. 使用说明

连接图像源即可自动推理；用 RESULT 统计人数或裁剪人脸区域。

## 5. 示例

Camera → Face Detection → Image Switch，多人时切换特写逻辑。
