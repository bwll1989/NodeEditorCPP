# ExtractNode 使用说明

## 用途
ExtractNode 用于从输入的 VariableData（键值表/嵌套结构）中“提取/计算”出一个值，并输出给下游节点。你可以写一段 JavaScript 表达式来取字段、做简单计算或拼装结果。

## 端口
- 输入（1）
  - INPUT 0：VariableData
- 输出（1）
  - OUTPUT 0：VariableData（表达式计算结果）

## 表达式说明
- 节点会把输入数据注入到脚本环境：
  - `input`：整份输入数据（对象）
  - 同时会把输入的“顶层键”也作为同名变量注入，方便直接写 `position.x` 这类表达式
- 表达式为空时：直接输出整份输入数据（便于整体透传）。

## 常用示例
- 取某个字段：
  - `input["temperature"]`
  - `temperature`
- 取嵌套字段：
  - `position.x`
  - `input["position"]["x"]`
- 简单计算：
  - `input["a"] + input["b"]`
  - `speed * 3.6`

## 注意事项
- 表达式区分大小写。
- 字段不存在或表达式报错时，输出为空。
