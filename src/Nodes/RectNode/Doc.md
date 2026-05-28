# Rect 节点

## 1. 节点说明

定义或接收一个矩形区域，并输出矩形本身及其派生几何量，供裁剪、布局等节点使用。

## 2. 端口说明

### 输入

- **POS_X**（VariableData）：左上角 X。
- **POS_Y**（VariableData）：左上角 Y。
- **WIDTH**（VariableData）：宽度。
- **HEIGHT**（VariableData）：高度。

### 输出

- **Rect**（VariableData）：完整矩形（QRectF）。
- **SIZE**（VariableData）：宽高（QSizeF）。
- **CENTER**（VariableData）：中心点。
- **BOTTOM_RIGHT**（VariableData）：右下角坐标。

## 3. 界面说明

无内嵌面板；通过属性 `x`、`y`、`width`、`height` 或 `/x`、`/y`、`/width`、`/height` 外部控制。

## 4. 使用说明

1. 在属性中设置默认矩形，或用输入端口动态更新。
2. 将 **Rect** 接到 CutImage 的 RECT，或将 **SIZE** 接到 Scale Image。

## 5. 示例

触摸坐标 → POS_X/POS_Y → Rect → CutImage，实现可移动裁剪框。
