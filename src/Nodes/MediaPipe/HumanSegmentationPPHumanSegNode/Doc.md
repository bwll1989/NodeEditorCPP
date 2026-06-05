# PPHumanSeg 人物分割节点

## 1. 节点说明

基于 [OpenCV Zoo PPHumanSeg](https://github.com/opencv/opencv_zoo/tree/main/models/human_segmentation_pphumanseg) 实现的人物分割节点。使用 `human_segmentation_pphumanseg_2023mar.onnx` 对输入画面进行语义分割，将人物与背景分离。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入画面。
- **ENABLE**（VariableData）：启停分割。

### 输出

- **FOREGROUND**（ImageData）：前景图，保留人物区域像素，非人物区域为黑色。
- **BACKGROUND**（ImageData）：背景图，保留背景区域像素，人物区域为黑色。

## 3. 界面说明

- 最大帧率（1–30 FPS）
- **使用 GPU (OpenCV CUDA DNN)**：默认开启；需 OpenCV 编译时启用 CUDA/cuDNN，否则自动回退 CPU
- 启停按钮

## 4. 模型文件

将以下 ONNX 模型放入 `./plugins/Models/`：

- `human_segmentation_pphumanseg_2023mar.onnx`

下载地址：[human_segmentation_pphumanseg](https://github.com/opencv/opencv_zoo/tree/main/models/human_segmentation_pphumanseg)

## 5. 使用说明

1. 连接视频或图像到 IMAGE，并通过 ENABLE 或面板启用分割。
2. 将 **FOREGROUND** 接到后续合成、抠像或特效节点。
3. 将 **BACKGROUND** 用于背景替换、模糊等处理。
4. 支持 OSC：`/enable`。

## 6. 示例

Camera → PPHumanSeg → Spout Out（前景），用于实时人物抠像与背景分离。
