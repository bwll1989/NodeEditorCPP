# ImageInfoNode 使用说明

## 用途
ImageInfoNode 用于查看输入图像的基础信息，并把这些信息以 VariableData 的形式输出，方便你在流程中做调试与条件判断。

## 端口
- 输入（1）
  - INPUT（端口 0）：ImageData
- 输出（2）
  - OUTPUT 0（端口 0）：ImageData（透传）
  - OUTPUT 1（端口 1）：VariableData（图像属性）

OUTPUT 1 常用字段：
- `width`：宽度（int）
- `height`：高度（int）
- `channels`：通道数（int）
- `isNull`：是否为空（bool）

## 使用步骤
1. 将图像源连接到 INPUT。
2. 通过节点界面查看属性树。
3. 若需要做逻辑判断，把 OUTPUT 1 接到 Extract/Condition/Switch 等节点读取字段。

## 注意事项
- 输入为空时，OUTPUT 1 只会包含 `isNull=true`。
