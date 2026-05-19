# ImageThresholdNode 使用说明

## 用途
ImageThresholdNode 用于对输入图像做阈值化（黑白化/截断/置零等），常用于二值分割、轮廓提取前处理等。

## 端口
- 输入（4）
  - IMAGE（端口 0）：ImageData
  - THRESHOLD（端口 1）：VariableData（int）
  - MAXVAL（端口 2）：VariableData（int）
  - METHOD（端口 3）：VariableData（int，方法索引）
- 输出（1）
  - OUTPUT 0（端口 0）：ImageData（阈值化后的图像）

## 方法索引（METHOD）
- 0：Binary
- 1：BinaryInv
- 2：Trunc
- 3：ToZero
- 4：ToZeroInv

## 外部控制（可选）
- `/thresh`：设置阈值（int）
- `/maxval`：设置最大值（int）
- `/method`：设置方法索引（int）

## 使用步骤
1. 将图像源连接到 IMAGE。
2. 设置 THRESHOLD/MAXVAL 与 METHOD（可通过端口或外部控制）。
3. 将输出图像连接到下游节点查看结果。

## 注意事项
- 输入图像若是彩色，会先自动转为灰度后再阈值化。
