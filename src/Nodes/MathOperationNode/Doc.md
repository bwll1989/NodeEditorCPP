# Math Operation 节点

## 1. 节点说明

Math Operation 插件提供一组**双输入算术**节点，对两个数值型 VariableData 做运算，结果从单一输出端口输出。无内嵌界面。

| 变体名称 | 运算 |
|----------|------|
| Math Add | 加法 |
| Math Sub | 减法 |
| Math Mul | 乘法 |
| Math Div | 除法（除数为 0 时结果为 0） |
| Math Mod | 浮点取模（`fmod`） |
| Math Pow | 幂运算（`pow`） |

输入会按 `toDouble()` 转为浮点数参与计算；任一输入更新后输出立即刷新。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT 0 | VariableData | 第一操作数 |
| 1 | INPUT 1 | VariableData | 第二操作数 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT 0 | VariableData | 运算结果（数值） |

## 3. 界面说明

各变体无内嵌面板，无额外控件。

## 4. 使用说明

1. 在节点库中选择所需变体（如 Math Mul）。
2. 将两路数值数据接到 INPUT 0、INPUT 1。
3. 将 OUTPUT 0 接到后续节点（如 Extract、Merge、显示等）。

除法节点在除数为 0 时输出 0，避免异常中断流程。

## 5. 示例

**亮度缩放：** Math Mul 将 0–1 的系数（INPUT 1）与 DMX 通道值（INPUT 0）相乘，输出接 Artnet 或下游控制。  
**周期性数值：** Math Mod 对递增计数取模，实现循环索引。
