# StyleTransfer 节点

## 1. 节点说明

使用 ONNX 模型对图像做风格迁移（默认 AnimeGAN 模型）。输入一帧即推理一帧，输出风格化图像及结果变量。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：原始图像。

### 输出

- **IMAGE 0**（ImageData）：风格化后的图像。
- **RESULT**（VariableData）：推理相关结果数据。

## 3. 界面说明

无复杂面板；可通过 `/enable` 控制是否处理。模型文件位于 `plugins/Models/`（如 `AnimeGANv3_Hayao_36.onnx`）。

## 4. 使用说明

1. 确认 ONNX 模型已部署。
2. 连接图像源到 IMAGE。
3. 使用 IMAGE 0 显示或继续处理；RESULT 可供调试或逻辑判断。

## 5. 示例

摄像头 → StyleTransfer → NDI Out，实时漫画风直播画面。
