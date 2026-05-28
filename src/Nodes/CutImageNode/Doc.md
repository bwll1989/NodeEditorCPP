# CutImage 节点

## 1. 节点说明

从输入图像中按矩形区域裁剪出一块子图。裁剪区域可通过 RECT 一次性传入，或分别用坐标与宽高端口设置。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：源图像。
- **RECT**（VariableData）：裁剪矩形（QRect），可覆盖下方各分量。
- **POS_X**（VariableData）：左上角 X。
- **POS_Y**（VariableData）：左上角 Y。
- **WIDTH**（VariableData）：裁剪宽度。
- **HEIGHT**（VariableData）：裁剪高度。

### 输出

- **IMAGE**（ImageData）：裁剪结果。

## 3. 界面说明

无内嵌控件；通过属性 `topLeftX`、`topLeftY`、`width`、`height` 或外部绑定 `/topLeftX`、`/topLeftY`、`/width`、`/height` 设置。

## 4. 使用说明

1. 连接 IMAGE。
2. 设置裁剪矩形（属性、RECT 或 POS_X/Y + WIDTH/HEIGHT）。
3. 将输出接到后续图像处理或显示节点。

## 5. 示例

全画面 → CutImage（RECT 来自 Rect 节点）→ Object Detection，只对感兴趣区域做检测。
