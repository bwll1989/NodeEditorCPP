# ConditionNode 使用说明

## 用途
ConditionNode 用于对输入数据做“条件判断”，输出 true/false，常用于触发开关、分支控制、事件过滤等。

## 输入端口（VariableData）
- INPUT 0：输入数据。

## 输出端口
- OUTPUT 0：判断结果（bool）。

## 如何写表达式
- 节点会提供一个变量 `$input`，表示当前输入数据。
- 默认表达式：`$input["default"]`。

## 示例
- 数值判断：`$input.value > 10`
- 嵌套字段：`$input.position.x > 100 && $input.position.y < 200`
- 事件触发：`$input.triggered === true`
- 类型判断：`typeof $input.value === 'number'`

## 使用步骤
1. 连接数据到 INPUT 0。
2. 在节点文本框里输入表达式。
3. 连接 OUTPUT 0 到下游节点作为条件/开关。

## 注意事项
- 输入为空时通常输出 false。
- 表达式修改在按回车或失去焦点后生效。
- 表达式应尽量返回布尔值（true/false）。