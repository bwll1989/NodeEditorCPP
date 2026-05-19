# HoldNode 使用说明

## 用途
HoldNode 用于“保持信号一段时间”：当 INPUT 收到数据后，节点会立即输出该数据，并在设定的保持时间内保持输出；时间到期后输出会被清空。

## 端口
- 输入（2）
  - INPUT（端口 0）：VariableData，需要保持的数据
  - HOLD_TIME（端口 1）：VariableData，动态设置保持时间（ms）
- 输出（1）
  - OUTPUT（端口 0）：VariableData（保持期间输出为输入值，到期后清空）

## 参数/模式
- holdTime：保持时间（ms）
- ignoreRepeat：重复信号处理方式
  - true：忽略模式（保持期间忽略新输入）
  - false：重置模式（默认；新输入会重新开始计时）

## 使用步骤
1. 将信号源连接到 INPUT。
2. 设置 holdTime（或把时间源连接到 HOLD_TIME）。
3. 从 OUTPUT 取值，在保持时间内用于触发/控制下游节点。

## 外部控制（可选）
- `/time`：设置 holdTime（int，ms）
- `/ignoreRepeat`：设置 ignoreRepeat（bool）

## 注意事项
- holdTime 小于等于 0 时不会进入保持状态，输入会直接透传输出。
