# QmlScriptNode 使用说明

## 用途
在流程里执行 JavaScript 脚本：读取输入端口的数据、计算后写入输出端口；并支持用 QML 面板动态生成/编辑脚本参数（settings/uiSchema）。

## 端口
- 输入（默认 4 个，可编辑）：VariableData
- 输出（默认 1 个，可编辑）：VariableData

说明：
- 当前端口标题显示为 “OUT n/IN n”，可能与实际方向相反；以连线方向为准即可。

## 快速上手
1. 添加 QmlScriptNode，打开脚本编辑器并写入代码。
2. 连接上游到输入端口，连接下游到输出端口。
3. 在脚本里实现 `inputEventHandler(index)`，当输入更新时计算并输出。

## 常用 API（脚本侧）
- `Node.getInputValue(portIndex)`：获取输入（对象/键值表）
- `Node.setOutputValue(outIndex, value)`：写入输出
- `Node.getOutputValue(outIndex)`：读取当前输出
- `Node.inputIndex()`：当前触发的输入端口索引
- `Node.getInputCount()` / `Node.getOutputCount()`：端口数量

## QML 面板能力（可选）
- settings：脚本可读写的持久化配置（随工程保存/加载）
- uiSchema：用于描述面板控件的结构（由脚本设置，供面板生成控件）

## 注意事项
- 端口数据以键值表（VariableData）传递；多数情况下读写 `default` 即可。
- 脚本执行耗时过长会影响整体刷新，建议把重计算拆分/节流。 
