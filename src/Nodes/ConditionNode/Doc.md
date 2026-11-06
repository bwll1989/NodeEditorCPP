# Condition

## 1. 节点说明

Condition 使用 **JavaScript** 对输入 `VariableData` 求值（表达式内通过 `$input` 访问整份输入），并通过**两个输出端口**同时提供：

| 能力 | 输出端口 | 行为 |
|------|----------|------|
| 条件判断 | **CONDITION** | 每次输入更新都输出表达式布尔结果 |
| 门控透传 | **DATA** | 仅条件为 **true** 时透传输入数据副本 |

适合实时状态监测、门控触发、与 Count / Switch / Inject / Delay 配合的逻辑流程。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 2 | 否 | 2 | 2 | — | 末端口为 DATA（门控透传） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT | VariableData | 供 `$input` 使用的键值数据 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | CONDITION | VariableData | 每次输入更新都推送；`default` 为表达式布尔结果（true/false） |
| 1 | DATA | VariableData | 仅条件为 true 时推送；输出为输入数据的完整副本，`default` 设为 `true` |

**两路输出的触发差异：**

- **CONDITION**：条件为 true 或 false 时都会 `dataUpdated`，下游始终能读到最新布尔状态。
- **DATA**：条件为 false 时**不**主动推送（门控语义）；适合触发 Inject、Delay TRIGGER 等「仅在成立时执行一次」的下游。

## 3. 界面与属性

- **表达式编辑框**（单行）：占位提示 `JS Expression (e.g., "$input['key']")`。
- 表达式内请使用 `$input` 访问整份输入对象；也可使用 `$input["key"]`、`$input.key` 等写法。
- 编辑完成（失焦）后，若已有输入数据会重新求值并刷新输出。

无其它控件，无公开 Q_PROPERTY。

## 4. 外部控制（OSC 地址）

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用）。

地址前缀：`/dataflow/{别名}/{节点ID}/…`。若节点定义了公开 Q_PROPERTY，框架会自动为其生成默认属性地址；本节点当前无此类属性，表达式请通过界面或工程文件配置。

## 5. 行为细节

- 表达式以 `$input`（输入数据的键值映射）求值；表达式留空时按输入 `default` 键的布尔真值判断。
- 表达式错误时，CONDITION 输出 false；DATA 不触发。
- 空数据（断开）或非 VariableData 输入被忽略。
- 持久化：`values.expression`。

## 6. 使用说明与典型接法

1. 编写返回布尔值的表达式，如 `$input["enabled"] == true`。
2. **CONDITION** 端口：接到 Switch 的 INDEX、Logic 节点、Data Visual 等，持续反映条件是否成立。
3. **DATA** 端口：接到 Inject 的 TRIGGER、Delay 的 TRIGGER 等，条件成立时透传完整输入供下游读取字段。
4. 同一节点可同时使用两路输出：CONDITION 驱动指示灯，DATA 驱动动作触发。

**表达式示例：**

| 表达式 | 含义 |
|--------|------|
| `$input["go"]` | 读取 `go` 字段的布尔/真值 |
| `$input["level"] > 0.8` | 数值比较 |
| `$input.pointId == 5` | 到点判断 |
| `$input["enabled"] == true` | 显式布尔比较 |

## 7. 示例

**门控播放：** 条件 `$input["go"] == true`，**DATA** 接 Inject 的 TRIGGER；条件成立时下游可读取 `scene` 等原输入字段。

**实时指示灯 + 报警：** 条件 `$input["fault"]`，**CONDITION** 接 Data Visual；**DATA** 接 Delay TRIGGER 做一次性报警。

**到点触发场景：** 定位 POINT 数据 → Condition（`$input.pointId == 5`）→ **DATA** → 下游场景切换。

**与 Count 配合：** **CONDITION** 输出接 Count 的条件输入，统计某状态出现次数。

**DATA 输出示例：**

输入 `{ "scene": 3, "go": true }`，表达式 `$input["go"] == true`，DATA 输出：

```json
{ "scene": 3, "go": true, "default": true }
```

---

# Edge Trigger

## 1. 节点说明

Edge Trigger 检测输入信号的**边沿变化**，仅在指定边沿发生时输出一次脉冲。与 Condition 的持续判断不同，本节点只在 **0→1（上升沿）** 或 **1→0（下降沿）** 的瞬间触发，适合按钮按下/释放、状态跳变等一次性动作。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 2 | 否 | 2 | 2 | — | 末端口为 DATA（当前输入值） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT | VariableData | 待检测的信号源 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | PULSE | VariableData | 边沿发生时输出 `true`；无新边沿时不推送 |
| 1 | DATA | VariableData | 边沿发生时透传输入数据副本，`default` 设为 `true` |

首次输入仅记录当前状态，**不产生触发**，避免启动时的误触发。

## 3. 界面与属性

- **输入值**：JS 表达式，留空则读取 `default` 键的布尔/真值；示例 `$input["button"]`
- **触发沿**：
  - **0 → 1 (上升沿)**：信号从 false 变为 true 时触发
  - **1 → 0 (下降沿)**：信号从 true 变为 false 时触发

公开属性：`edgeMode`（EdgeMode 枚举，0 = 上升沿，1 = 下降沿）。**注意：该属性未注册外部绑定，无法通过 OSC 设置**，只能通过界面下拉框或工程文件配置。

## 4. 外部控制（OSC 地址）

本节点未注册任何专属 OSC 地址（源码中 `/edgeMode` 的 `registerExternalBinding` 调用被注释，实际未生效）。

地址前缀：`/dataflow/{别名}/{节点ID}/…`。若节点定义了公开 Q_PROPERTY，框架会自动为其生成默认属性地址；触发沿请通过界面或工程文件配置。

## 5. 行为细节

- 求值规则：表达式留空时取输入 `default` 键的布尔真值；表达式错误按 false 处理并输出日志。
- 边沿检测：比较本次与上次布尔结果；**首次输入仅记录状态、不产生触发**，避免启动误触发。
- 边沿命中时：PULSE 输出 `true`，DATA 透传输入数据副本（`default` 置 `true`）；无新边沿不推送。
- 切换触发沿会重置边沿状态（下次输入重新作为"首次"记录）。
- 空数据（断开）或非 VariableData 输入被忽略。
- 持久化：`values.expression`、`values.edgeMode`。

## 6. 使用说明与典型接法

1. 将布尔或真值信号源接到 INPUT。
2. 选择上升沿或下降沿。
3. **PULSE** 接 Inject TRIGGER、Delay TRIGGER、Count 等只需「跳变一次」的下游。
4. **DATA** 接需要同时读取输入字段的下游。

## 7. 示例

**按钮按下触发：** 表达式 `$input["pressed"]`，上升沿，PULSE → Inject TRIGGER。

**按钮释放触发：** 同一表达式，下降沿，PULSE → 停止动作或复位逻辑。

**IO 输入去抖后边沿：** USR-IO808 布尔输入 → Edge Trigger（上升沿）→ DATA → 场景切换。

