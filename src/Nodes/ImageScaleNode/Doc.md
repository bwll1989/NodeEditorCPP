# ImageScaleNode 使用说明

## 用途
ImageScaleNode 用于把输入图像缩放到指定的目标宽高（直接缩放到目标尺寸，不做纵横比保持）。

## 端口
- 输入（4）
  - Image（端口 0）：ImageData
  - SIZE（端口 1）：VariableData（QSize），一次性设置宽高
  - Width（端口 2）：VariableData（int）
  - Height（端口 3）：VariableData（int）
- 输出（1）
  - Image（端口 0）：ImageData（缩放后的图像）

说明：
- 你可以只用 SIZE，也可以分别用 Width/Height。
- 当 Image、Width、Height 任意变化时，会重新缩放并更新输出。

## 外部控制（可选）
- `/width`：设置 width（int）
- `/height`：设置 height（int）

兼容：
- 某些旧用法也会发送 `/Width`、`/Height`（同样会生效）。

## 使用步骤
1. 将图像源连接到 Image 输入端口。
2. 设置目标尺寸（SIZE 或 Width/Height）。
3. 将输出 Image 连接到下游图像处理或显示节点。

## 注意事项
- width/height 小于等于 0 时，输出会为空。
