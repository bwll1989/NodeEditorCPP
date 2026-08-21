**Inject**

本插件包含 **Inject** 与 **Distribute** 两个节点，常用于按键映射、预设值注入与条件分发。

---

# Inject

## 1. 节点说明

Inject 按**索引注入预设值**：当某个 TRIGGER 输入收到布尔 `true` 时，将列表中对应行的 Value 从输出端口送出。适用于快捷键映射、场景号注入、预设参数触发等。

每次有效触发会在输出中附带递增字段 `_pulse`，便于下游区分「相同数值的重复触发」。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 … N | TRIGGER 0、TRIGGER 1 … | VariableData | 值为 `true` 时触发对应索引的注入 |

默认 5 个输入，**可编辑**。仅 `true` 触发，其它值忽略。

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT 0 | VariableData | 被触发行的解析结果；含 `_pulse` |

## 3. 界面说明

可编辑表格：

| 列 | 说明 |
|----|------|
| Index | 行号，与 TRIGGER 端口索引一致（0 对应 TRIGGER 0） |
| Value | 触发时输出的值（支持整数、浮点、布尔、字符串） |
| Action | 删除该行 |

- **Add Item**：增加行；行数建议不少于当前 TRIGGER 端口数。

Value 解析规则：优先整数，其次浮点，再识别 `true`/`false`，否则为字符串。

## 4. 使用说明

1. 按需调整输入端口数量。
2. 为每个要用的索引填写 Value。
3. 将布尔触发源接到对应 TRIGGER。
4. 从 OUTPUT 0 读取注入结果。

若索引无对应行或 Value 为空，触发后输出为空 VariableData。

列表保存在工程 `list` 字段（字符串数组顺序与行一致）。

## 5. 示例

**五键场景：** 五行 Value 分别为 `1`…`5`，Keyboard In 五路输出分别接 TRIGGER 0–4，OUTPUT 接 Switch 的 INDEX 或场景加载逻辑。

**同一值重复触发：** 下游根据 `_pulse` 区分每次按键，即使 Value 同为 `1` 也能计数。

---

# Distribute

## 1. 节点说明

Distribute 根据**单路输入**匹配规则表，在命中的输出端口发出布尔 `true` 触发，**不转发**输入的原始数值。适用于一路输入按条件驱动多路分支（如 MIDI 音符、场景号、状态码、定位数据分发）。

同一输入可匹配**多行**规则，从而同时触发多个输出端口。输出端口数量会随规则中的最大 Port 号自动扩展（默认至少 5 个）。

条件列支持 **JavaScript 表达式**，整份输入 `VariableData` 注册为全局变量 `$input`（与 Extract、Lookup、Condition 等节点一致）。

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT | VariableData | 参与条件匹配的完整输入对象 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 … N | OUTPUT 0、OUTPUT 1 … | VariableData | 规则命中时为 `true`；未命中过的端口读取为 `false` |

输出端口**可编辑**；规则表中最大 Port + 1 决定最少端口数。

命中时输出附带：`_pulse`（递增）、`_distributedPort`、`_matchedRow`。

## 3. 界面说明

规则表三列：

| 列 | 说明 |
|----|------|
| Condition | JS 条件表达式（见下表） |
| Port | 命中后输出 `true` 的端口索引 |
| Action | 删除该行 |

- **Add Rule**：新增规则。
- 默认示例含 `$input.default == 0` 与 `*` 两行，可按需修改。

**条件写法：**

| 写法 | 含义 |
|------|------|
| `*` | 任意输入均匹配 |
| `$input.default == 0` | 标量输入（`default` 键）等于 0 |
| `$input['key'] > 10` | 取对象字段做比较 |
| `$input.boatId == 1` | 访问嵌套或顶层字段 |
| `== 0`、`> 10` 等 | 旧版比较写法；JS 解析失败时自动回退到数值比较（针对 `default` 值） |

自上而下扫描**所有**行，所有命中行都会触发对应 Port。修改规则后，若已有输入缓存，会自动重新匹配。

## 4. 使用说明

1. 配置 Condition 与 Port 列。
2. 将待分发的 VariableData 接到 INPUT。
3. 将各 OUTPUT 接到 Inject 的 TRIGGER 或仅需布尔触发的下游。
4. 无规则命中时不更新输出。

规则保存在 `rules`：`[{ "condition": "...", "port": "0" }, ...]`。

## 5. 示例

| Condition | Port | 效果 |
|-----------|------|------|
| `$input.default == 45` | `0` | 标量输入 45 → OUTPUT 0 为 true |
| `$input.default == 45` | `1` | 标量输入 45 → OUTPUT 1 也为 true |
| `$input.pointId == 5` | `2` | 定位点位 ID 为 5 → OUTPUT 2 为 true |
| `*` | `3` | 任意输入 → OUTPUT 3 为 true |

**典型链路：** 一路 MIDI 或 FT-LocationProto 的 POINT 输出接入 INPUT，多路 OUTPUT 分别接 Inject 的 TRIGGER，实现「按条件选路 → 注入不同预设值」。
