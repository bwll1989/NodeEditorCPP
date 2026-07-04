# Condition 节点

## 1. 节点说明

Condition 插件提供两种与「条件表达式」相关的节点，均使用 **JavaScript** 对输入 VariableData 求值（表达式内通过 `$input` 访问整份输入）。

| 变体 | 节点库名称 | 行为 |
|------|------------|------|
| Condition | Condition | 每次输入更新都求值，**持续输出**表达式结果的布尔值 |
| Filter | Filter | 仅当表达式为 **true** 时才向输出发送一次更新（脉冲式通过），并透传输入数据 |

适合数据过滤、门控、与 Count 配合的逻辑判断。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | （默认） | VariableData | 供 `$input` 使用的键值数据 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | （默认） | VariableData | Condition：`default` 为布尔结果；Filter：条件为真时输出输入副本，并将 `default` 设为条件结果（`true`），否则不主动推送 |

## 3. 界面说明

- **表达式编辑框**（单行）：占位提示 `JS Expression (e.g., "input['key']")`。新建节点时为空，需自行填写表达式；表达式内请使用 `$input` 访问输入对象。
- 编辑完成（失焦）后 Condition 会刷新输出；Filter 在下次输入且条件为真时触发。

无其它控件。Filter 的输入端口数量**可编辑**（逻辑不变）。

## 4. 使用说明

**Condition：**

1. 编写返回布尔值的表达式，如 `$input["enabled"] == true`。
2. 输入变化时输出始终反映当前条件是否成立（结果写入 `default`）。
3. 接到 Switch 的 INDEX、显示节点等。

**Filter：**

1. 同样编写布尔表达式。
2. 仅当条件从输入数据求值为 true 时，输出端口 `dataUpdated` 一次（适合触发后续一次性动作）。
3. 输出为**输入数据的完整副本**，其中 `default` 被设为条件结果（`true`）；其余字段原样保留。
4. 与 Condition 不同：不会在条件为 false 时持续推送。

表达式错误时，Condition 输出 false；Filter 不触发。

配置保存在 `values.expression`。

## 5. 示例

**门控播放：** Filter 条件 `$input["go"]`，输出接 Inject 的 TRIGGER；条件成立时下游可读取 `$input["scene"]` 等原输入字段。  
**实时指示灯：** Condition 条件 `$input["fault"]`，输出接 Data Visual 或内部状态显示。

**Filter 输出示例：**

输入 `{ "scene": 3, "go": true }`，表达式 `$input["go"] == true`，输出：

```json
{ "scene": 3, "go": true, "default": true }
```
