# Logic Operation 节点使用说明

## 用途
本插件提供 9 个“逻辑/比较”节点：每个节点固定一种运算方式，对两路输入进行判断并输出结果。适用于条件判断、阈值触发、流程分支等场景。

## 节点列表（9 个）
- Logic And：判等（按字符串比较 INPUT0 与 INPUT1 是否相等）
- Logic Or：或运算（按 bool 做 OR）
- Logic NotEqual：不等（按字符串比较 INPUT0 与 INPUT1 是否不相等）
- Logic Max：取大（按 double 取较大值，再转为 bool 输出）
- Logic Min：取小（按 double 取较小值，再转为 bool 输出）
- Logic Less：小于（按 float 比较 INPUT0 < INPUT1）
- Logic LessEqual：小于等于（按 float 比较 INPUT0 <= INPUT1）
- Logic Greater：大于（按 float 比较 INPUT0 > INPUT1）
- Logic GreaterEqual：大于等于（按 float 比较 INPUT0 >= INPUT1）

## 端口（所有节点一致）
- 输入（2）
  - INPUT 0：VariableData
  - INPUT 1：VariableData
- 输出（1）
  - OUTPUT 0：VariableData（bool）

## 使用步骤
1. 从节点面板中选择需要的逻辑节点（例如 Logic Less）。
2. 将两个输入信号分别连接到 INPUT 0 / INPUT 1。
3. 从 OUTPUT 0 读取 bool 结果，连接到 Condition/Switch/Hold 等节点实现流程控制。

## 注意事项
- 类型转换规则与实现一致：
  - And / NotEqual：使用 `toString()` 比较（例如 "1" 与 "1.0" 会被视为不同）
  - Or：使用 `toBool()`（非空字符串通常为 true）
  - Less/LessEqual/Greater/GreaterEqual：使用 `toFloat()`
  - Max/Min：使用 `toDouble()` 取值后再转 bool 输出
- 这些节点不再支持 `/method` 切换方法：方法已通过“节点类型”固定。

