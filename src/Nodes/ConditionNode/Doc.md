# Condition 插件

本插件（Controls）包含两个节点：**Condition**（条件判断与门控）与 **Edge Trigger**（边沿触发）。

---

# Condition 节点

## 1. 节点说明

Condition 使用 **JavaScript** 对输入 `VariableData` 求值（表达式内通过 `$input` 访问整份输入），并通过**两个输出端口**同时提供：

| 能力 | 输出端口 | 行为 |
|------|----------|------|
| 条件判断 | **CONDITION** | 每次输入更新都输出表达式布尔结果 |
| 门控透传 | **DATA** | 仅条件为 **true** 时透传输入数据副本 |

适合实时状态监测、门控触发、与 Count / Switch / Inject / Delay 配合的逻辑流程。

## 2. 端口说明

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

表达式错误时，CONDITION 输出 false；DATA 不触发。

## 3. 界面说明

- **表达式编辑框**（单行）：占位提示 `JS Expression (e.g., "$input['key']")`。
- 表达式内请使用 `$input` 访问整份输入对象；也可使用 `$input["key"]`、`$input.key` 等写法。
- 编辑完成（失焦）后，若已有输入数据会重新求值并刷新输出。

无其它控件。

## 4. 使用说明

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

配置保存在 `values.expression`。

## 5. 示例

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

# Edge Trigger 节点

## 1. 节点说明

Edge Trigger 检测输入信号的**边沿变化**，仅在指定边沿发生时输出一次脉冲。与 Condition 的持续判断不同，本节点只在 **0→1（上升沿）** 或 **1→0（下降沿）** 的瞬间触发，适合按钮按下/释放、状态跳变等一次性动作。

## 2. 端口说明

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

## 3. 界面说明

- **输入值**：JS 表达式，留空则读取 `default` 键的布尔/真值；示例 `$input["button"]`
- **触发沿**：
  - **0 → 1 (上升沿)**：信号从 false 变为 true 时触发
  - **1 → 0 (下降沿)**：信号从 true 变为 false 时触发

**外部控制地址：**

- `/edgeMode` — 设置触发沿（0 = 上升沿，1 = 下降沿）

## 4. 使用说明

1. 将布尔或真值信号源接到 INPUT。
2. 选择上升沿或下降沿。
3. **PULSE** 接 Inject TRIGGER、Delay TRIGGER、Count 等只需「跳变一次」的下游。
4. **DATA** 接需要同时读取输入字段的下游。

配置保存在 `values.expression`、`values.edgeMode`。

## 5. 示例

**按钮按下触发：** 表达式 `$input["pressed"]`，上升沿，PULSE → Inject TRIGGER。

**按钮释放触发：** 同一表达式，下降沿，PULSE → 停止动作或复位逻辑。

**IO 输入去抖后边沿：** USR-IO808 布尔输入 → Edge Trigger（上升沿）→ DATA → 场景切换。

