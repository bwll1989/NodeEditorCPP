# CountNode 使用说明

## 用途
CountNode 用于对输入数据进行“条件计数”：当你设定的条件表达式为 true 时，计数值 +1。

## 端口
- 输入（1）
  - INPUT 0：VariableData
- 输出（1）
  - OUTPUT 0：当前计数值（整数）

## 节点参数/界面
- 条件表达式：在文本框里填写条件（需要返回 true/false）
- clear：清零计数

## 使用步骤
1. 将任意 VariableData 数据源连接到 INPUT 0。
2. 在表达式框输入条件，例如：
   - `$input.value > 10`
   - `$input.position.x > 100 && $input.position.y < 200`
   - `$input.triggered === true`
3. 当输入更新且表达式结果为 true 时，计数器自动 +1。
4. 需要重新计数时，点击 clear。

## 注意事项
- 表达式里可直接使用 `$input` 访问输入数据。
- 表达式区分大小写，且建议始终返回布尔值。
- 输入为空时，节点保持当前计数不变。