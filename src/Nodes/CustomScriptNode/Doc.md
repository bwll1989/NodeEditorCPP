# Custom Script

## 1. 节点说明

> **注意**：本节点的注册当前已在代码中注释（`PluginDefinition.cpp` 中 `registerModel<CustomScriptDataModel>` 被注释），节点库中不可用；以下说明供参考与历史工程兼容。

运行 **插件式 JavaScript** 的脚本节点：逻辑与 JavaScriptNode 类似，但无内置代码编辑器，界面与端口由加载的 **JS 插件** 元数据决定（名称、输入/输出数量、是否可缩放等）。适合发布封装好的自定义控件插件。

脚本在 GUI 线程中执行；控件加入布局后由 C++ 持有，避免删节点时崩溃。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 是 | 1 | 不限 | VariableData | 端口数量由 JS 插件元数据 `inputs` 决定（默认 4） |
| 输出 | 1 | 是 | 1 | 不限 | VariableData | 由插件 `outputs` 决定（默认 1） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| IN 0 … IN n | VariableData | 上游数据（数量由插件 `inputs` 定义，默认 4） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| OUT 0 … OUT n | VariableData | 脚本输出（数量由插件 `outputs` 定义，默认 1） |

## 3. 界面与属性

- **控件区**：插件脚本在 `initInterface` 中用 `Node.addToLayout` 创建的按钮、滑块等。
- 节点标题为插件名称；是否可调整大小、端口是否可编辑由插件元数据 `resizable`、`portEditable` 等决定。

脚本 API：`Node.getInputValue`、`Node.setOutputValue`、`Node.addToLayout`、`initInterface`、`inputEventHandler` 等与 JavaScriptNode 相同。

## 4. 外部控制（OSC 地址）

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用）。

地址前缀：`/dataflow/{别名}/{节点ID}/…`。若节点定义了公开 Q_PROPERTY，框架会自动为其生成默认属性地址；本节点当前无此类属性。

## 5. 行为细节

- 输入更新：`setInData` 缓存该口 `asMap()` 数据后触发插件脚本的 `inputEventHandler(portIndex)`；跨线程输入经 QueuedConnection 转到 GUI 线程执行。
- 端口/界面由插件元数据覆盖：`inputs`、`outputs`、`embeddable`、`resizable`、`portEditable`。
- **不持久化**：`save()` 返回空对象，脚本代码随插件包管理，不写入工程。

## 6. 使用说明与典型接法

1. 从节点库选择已注册的 Custom Script / JS 插件节点（非空白脚本节点）。
2. 将上游接到 IN 端口，下游接 OUT 端口。
3. 输入更新时插件脚本的 `inputEventHandler` 会被调用。
4. 更换插件需使用对应插件类型的节点实例；代码随插件包更新。

## 7. 示例

安装「调光面板」插件 → 拖入节点 → IN 0 接 DMX 反馈，OUT 0 接 DMX Device 的通道输入；在插件生成的滑块上手动调光。
