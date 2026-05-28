# CustomScriptNode

## 1. 节点说明

运行 **插件式 JavaScript** 的脚本节点：逻辑与 JavaScriptNode 类似，但无内置代码编辑器，界面与端口由加载的 **JS 插件** 元数据决定（名称、输入/输出数量、是否可缩放等）。适合发布封装好的自定义控件插件。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| IN 0 … IN n | VariableData | 上游数据（数量由插件 `inputs` 定义，默认 4） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| OUT 0 … OUT n | VariableData | 脚本输出（数量由插件 `outputs` 定义，默认 1） |

## 3. 界面说明

- **控件区**：插件脚本在 `initInterface` 中用 `Node.addToLayout` 创建的按钮、滑块等。
- 节点标题为插件名称；是否可调整大小、端口是否可编辑由插件元数据 `resizable`、`portEditable` 等决定。

脚本 API：`Node.getInputValue`、`Node.setOutputValue`、`Node.addToLayout`、`initInterface`、`inputEventHandler` 等与 JavaScriptNode 相同。

## 4. 使用说明

1. 从节点库选择已注册的 Custom Script / JS 插件节点（非空白脚本节点）。
2. 将上游接到 IN 端口，下游接 OUT 端口。
3. 输入更新时插件脚本的 `inputEventHandler` 会被调用。
4. 更换插件需使用对应插件类型的节点实例；代码随插件包更新。

## 5. 示例

安装「调光面板」插件 → 拖入节点 → IN 0 接 DMX 反馈，OUT 0 接 DMX Device 的通道输入；在插件生成的滑块上手动调光。
