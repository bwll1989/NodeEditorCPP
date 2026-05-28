# Condition 节点

## 1. 节点说明

Condition 插件提供两种与「条件表达式」相关的节点，均使用 **JavaScript** 对输入 VariableData 求值（表达式内通过 `$input` 访问整份输入）。

| 变体 | 节点库名称 | 行为 |
|------|------------|------|
| Condition | Condition | 每次输入更新都求值，**持续输出**表达式结果的布尔值 |
| Filter | Filter | 仅当表达式为 **true** 时才向输出发送一次更新（脉冲式通过） |

适合数据过滤、门控、与 Count 配合的逻辑判断。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | （默认） | VariableData | 供 `$input` 使用的键值数据 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | （默认） | VariableData | Condition：布尔结果；Filter：仅在条件为真时输出 true（否则不主动推送） |

## 3. 界面说明

- **表达式编辑框**（单行）：默认示例 `$input["default"]`。编辑完成（失焦）后 Condition 会刷新输出；Filter 在下次输入且条件为真时触发。

无其它控件。Filter 的输入端口数量**可编辑**（逻辑不变）。

## 4. 使用说明

**Condition：**

1. 编写返回布尔值的表达式，如 `$input["enabled"] == true`。
2. 输入变化时输出始终反映当前条件是否成立。
3. 接到 Switch 的 INDEX、显示节点等。

**Filter：**

1. 同样编写布尔表达式。
2. 仅当条件从输入数据求值为 true 时，输出端口 `dataUpdated` 一次（适合触发后续一次性动作）。
3. 与 Condition 不同：不会在条件为 false 时持续推送 false。

表达式错误时，Condition 输出 false；Filter 不触发。

配置保存在 `values.expression`。

## 5. 示例

**门控播放：** Filter 条件 `$input["go"]`，输出接 Inject 的 TRIGGER，仅在收到 go 信号时注入场景号。  
**实时指示灯：** Condition 条件 `$input["fault"]`，输出接 Data Visual 或内部状态显示。
