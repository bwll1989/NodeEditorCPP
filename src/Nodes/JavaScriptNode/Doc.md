# JavaScriptNode

## 1. 节点说明

在节点内编写 **JavaScript** 脚本，处理多路输入并写出多路输出。脚本可通过 `Node` 对象读写端口、用 Qt 控件 API 动态生成界面；支持 `initInterface` 初始化 UI、`inputEventHandler` 响应输入变化。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| OUT 0 … OUT n | VariableData | 上游数据（默认 4 个，可编辑增删）。注意：端口名为 OUT，表示从图上看为连入本节点的边 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| IN 0 … IN n | VariableData | 脚本通过 `Node.setOutputValue` 写出的数据（默认 1 个，可编辑） |

## 3. 界面说明

- **代码编辑器**：编写脚本；**运行/导入** 加载并执行代码。
- **更新 UI**：重新执行 `initInterface` 刷新动态控件区域。
- **上方布局区**：脚本用 `Node.addToLayout` 添加的控件（按钮、滑块等）。

脚本内可用：`Node.getInputValue(i)`、`Node.setOutputValue(i, value)`、`SpinBox`、`Button`、`Label` 等全局类型。

## 4. 使用说明

1. 在编辑器中编写 `initInterface` 与 `inputEventHandler`（及自定义函数）。
2. 点击运行加载脚本；用「更新 UI」重建界面。
3. 将上游节点接到 OUT 端口；下游接 IN 端口读取结果。
4. 输入变化时会调用 `inputEventHandler(索引)`。
5. 节点可缩放；端口数量会随工程保存。

## 5. 示例

```javascript
function inputEventHandler(index) {
  var v = Node.getInputValue(index)["default"];
  Node.setOutputValue(0, { default: v * 2 });
}
```

上游数值 → OUT 0 → 本节点 → IN 0 输出加倍结果。
