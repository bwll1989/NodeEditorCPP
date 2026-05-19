# ColorNode 使用说明

## 用途
ColorNode 用于在流程中生成/编辑一个颜色，并把颜色以及各通道值输出给下游节点使用。

## 输入端口（VariableData）
- R：红色（0~255）
- G：绿色（0~255）
- B：蓝色（0~255）
- A：透明度（0~255）

## 输出端口（VariableData）
- COLOR：完整颜色（QColor）。
- RED / GREEN / BLUE / ALPHA：分通道输出。

## 界面操作
- 颜色编辑器支持 RGBA 与 HSV 两种方式调整。
- 你可以直接在界面调色，也可以通过输入端口控制 RGBA。

## 外部控制（OSC）
- `/red` `/green` `/blue` `/alpha`：设置 RGBA。
- `/hue` `/saturation` `/value`：设置 HSV。

## 快速上手
1. 放入 ColorNode。
2. 在界面调色，或连接输入端口控制 RGBA。
3. 将 COLOR（或单通道输出）连接到下游节点。

## 注意事项
- RGBA 输入会与界面保持同步；如果颜色更新频繁，建议用单通道输入或直接发送 COLOR。