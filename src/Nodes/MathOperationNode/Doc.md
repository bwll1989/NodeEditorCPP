# Math Operation 节点使用说明

## 用途
本插件提供 6 个“数学运算”节点：每个节点固定一种运算方式，对两路输入进行计算并输出结果。

## 节点列表（6 个）
- Math Add：加法（INPUT0 + INPUT1）
- Math Sub：减法（INPUT0 - INPUT1）
- Math Mul：乘法（INPUT0 * INPUT1）
- Math Div：除法（INPUT0 / INPUT1）
- Math Mod：取模（fmod(INPUT0, INPUT1)）
- Math Pow：幂（pow(INPUT0, INPUT1)）

## 端口（所有节点一致）
- 输入（2）
  - INPUT 0：VariableData
  - INPUT 1：VariableData
- 输出（1）
  - OUTPUT 0：VariableData（double）

## 使用步骤
1. 从节点面板中选择需要的数学节点（例如 Math Div）。
2. 将两路数值分别连接到 INPUT 0 / INPUT 1。
3. 从 OUTPUT 0 读取结果并接入下游节点。

## 注意事项
- 所有运算都使用 `toDouble()` 转换输入值；非数值输入可能会得到 0 或非预期结果。
- Math Div：当 INPUT 1 转换后为 0 时，输出固定为 0。
- 这些节点不再支持 `/math` 切换运算：运算已通过“节点类型”固定。
