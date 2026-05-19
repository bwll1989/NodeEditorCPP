# JavaScriptNode 使用说明

## 用途
JavaScriptNode 用于在流程里编写 JS 逻辑：读取输入端口数据、处理后写入输出端口；也可以用脚本动态生成节点内的简单 UI（按钮/输入框/滑块等）。

## 端口
- 输入（默认 4 个，可编辑）：VariableData
- 输出（默认 1 个，可编辑）：VariableData

说明：
- 当前版本端口标题显示可能会“反着写”（输入端口标题显示 OUT n、输出端口显示 IN n）。以连线方向为准即可。
- 端口数据底层以键值表（VariableData）传递；多数情况下直接读写 `default` 即可。

## 快速上手
1. 添加 JavaScriptNode，点击“运行/导入”按钮加载脚本。
2. 连接上游到输入端口，连接下游到输出端口。
3. 在脚本里实现 `inputEventHandler(index)`，当输入更新时计算并 `Node.setOutputValue(outIndex, value)`。

## 常用 API
**输入/输出**
- `Node.getInputValue(portIndex)`：获取输入端口值
- `Node.setOutputValue(portIndex, value)`：设置输出端口值
- `Node.getOutputValue(portIndex)`：获取当前输出端口值
- `Node.inputIndex()`：当前触发的输入端口索引
- `Node.getInputCount()` / `Node.getOutputCount()`：端口数量

**UI（可选）**
- `Node.clearLayout()`：清空节点 UI
- `Node.addToLayout(widget, row, col, rowSpan, colSpan)`：把控件加入节点 UI

## 生命周期函数
- `initInterface()`：节点初始化或点击“更新 UI”时调用（可选）
- `inputEventHandler(index)`：任意输入端口更新时调用（可选）

## 支持的 UI 控件
可在脚本里创建并添加到布局：
- `SpinBox`、`DoubleSpinBox`
- `Button`
- `VSlider`、`HSlider`
- `Label`
- `LineEdit`
- `ComboBox`
- `CheckBox`

## 最小示例
输入 0 更新时，把数值乘 2 输出到端口 0：

```js
function inputEventHandler(index) {
    if (index !== 0) return;
    var v = Node.getInputValue(0)["default"];
    Node.setOutputValue(0, v * 2);
}
```

## 注意事项
- JS 引擎基于 Qt 的 QJSEngine；脚本报错会在控制台/日志中显示。
- 不建议在 `inputEventHandler` 里写耗时逻辑（会影响节点响应）。
