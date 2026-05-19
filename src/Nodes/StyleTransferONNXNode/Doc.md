# StyleTransferONNXNode 使用说明

## 用途
对输入图像进行 ONNX 风格迁移，并输出迁移后的图像与处理状态信息。

## 端口
### 输入
- IMAGE（ImageData）：输入图像

### 输出
- IMAGE 0（ImageData）：风格迁移后的图像
- RESULT（VariableData）：处理状态/信息

## 参数
- enable（bool）：启用/禁用风格迁移

## 外部控制（可选）
- /enable（bool）：启用/禁用

## 输出字段（RESULT）
RESULT 为一个键值表，常用字段：
- status：success / error
- input_size：原图尺寸（例如 1920x1080）
- output_size：输出尺寸（例如 1920x1080）
- model_input_size：模型输入尺寸（例如 720x720）
- timestamp：时间戳（毫秒）
- error_message：错误信息（仅 status=error 时）

## 使用步骤
1. 将图像源连接到 IMAGE。
2. 设置 enable=true（或通过 /enable 启用）。
3. 从 IMAGE 0 获取风格化图像；从 RESULT 获取状态与尺寸信息。

## 注意事项
- enable=false 或无输入时，RESULT 会输出空/默认值（不做风格迁移）。 
