# CutImageNode 使用说明

## 用途
CutImageNode 用于从输入图像中裁剪一个矩形区域（ROI），输出裁剪后的图像。适合做局部放大、目标区域截取、后续识别/对比前的预处理等。

## 端口
- 输入（6）
  - IMAGE（端口 0）：ImageData，源图像
  - RECT（端口 1）：VariableData，矩形区域（QRect）。一次性设置 X/Y/宽/高
  - POS_X（端口 2）：VariableData，左上角 X（整数）
  - POS_Y（端口 3）：VariableData，左上角 Y（整数）
  - WIDTH（端口 4）：VariableData，宽度（整数）
  - HEIGHT（端口 5）：VariableData，高度（整数）
- 输出（1）
  - IMAGE（端口 0）：ImageData，裁剪后的图像

说明：
- 你可以只用 POS_X/POS_Y/WIDTH/HEIGHT，也可以直接用 RECT 端口一次性设置完整区域。
- 任意参数变化都会触发重新裁剪并更新输出。

## 使用步骤
1. 将图像源节点连接到 IMAGE 输入端口。
2. 设定裁剪区域：
   - 方式 A：给 POS_X / POS_Y / WIDTH / HEIGHT 输入数值
   - 方式 B：给 RECT 输入一个 QRect（例如上游输出了 rect）
3. 从输出端口 IMAGE 获取裁剪结果，并连接到下游节点。

## 外部控制（可选）
支持以下外部地址（整数）：
- `/topLeftX`
- `/topLeftY`
- `/width`
- `/height`

## 注意事项
- WIDTH/HEIGHT 小于等于 0 时不会输出图像。
- 裁剪区域越界时会自动裁到有效范围；如果有效范围为空则输出为空。
- 输入图像为空时输出为空。
