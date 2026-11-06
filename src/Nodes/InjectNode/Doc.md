本插件包含 **Inject** 与 **Distribute** 两个节点，常用于按键映射、预设值注入与条件分发。

---

# Inject

## 1. 节点说明

Inject 按**索引注入预设值**：当某个 TRIGGER 输入收到布尔 `true` 时，将表格中对应行的 Value 从输出端口送出。适用于快捷键映射、场景号注入、预设参数触发等。

每次有效触发会在输出中附带递增字段 `_pulse`，便于下游区分「相同数值的重复触发」。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 是 | 1 | 不限 | VariableData | 输入口由界面行列表驱动（Add Item 增行加口、行 Action 删行减口），不走 PortEdit；至少保留 1 行 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 … N-1 | TRIGGER 0、TRIGGER 1 … | VariableData | 值为 `true` 时触发对应索引的注入；`false`/空数据忽略 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | OUTPUT 0 | VariableData | 被触发行的解析结果，附带 `_pulse`（递增计数） |

## 3. 界面与属性

控件位于右侧属性面板（`WidgetEmbeddable=false`，不嵌入节点体）。可编辑表格：

| 列 | 说明 |
|----|------|
| Index | 行号，与 TRIGGER 端口索引一致（0 对应 TRIGGER 0） |
| Value | 触发时输出的值（支持整数、浮点、布尔、字符串） |
| Action | 删除该行（同时删除对应输入端口，至少保留 1 行） |

- **Add Item**：增加行并同时新增一个 TRIGGER 输入端口。

Value 解析规则：优先整数，其次浮点，再识别 `true`/`false`（不区分大小写），否则为字符串；留空输出空值。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），注入值表只能在属性面板中编辑，触发只能经输入端口。

## 5. 行为细节

- **触发语义**：仅 `asBool()==true` 触发；连接瞬间的 `false` 或非 VariableData 数据不会注入。
- **注入过程**：读取该行文本 → `parseListValue` 解析（int → double → true/false → string，空文本为无效值）→ 生成新输出 VariableData 并插入 `_pulse`（全局递增计数器）→ 刷新输出。
- **行-端口联动**：增删行同步增删 TRIGGER 端口（portsAboutToBeInserted/Deleted）；删行后端口索引上移，建议同步调整连线。
- **持久化**：工程 JSON 顶层 `list` 字段（字符串数组，顺序与行一致；非 `values` 子对象）。

## 6. 使用说明与典型接法

1. 用 Add Item 调整行数（每行一个 TRIGGER 端口）。
2. 为每个要用的索引填写 Value。
3. 将布尔触发源接到对应 TRIGGER。
4. 从 OUTPUT 0 读取注入结果。

若索引无对应行或 Value 为空，触发后输出为空 VariableData（仍带 `_pulse`）。

## 7. 示例

**五键场景：** 五行 Value 分别为 `1`…`5`，Keyboard In 五路输出分别接 TRIGGER 0–4，OUTPUT 接 Switch 的 INDEX 或场景加载逻辑。

**同一值重复触发：** 下游根据 `_pulse` 区分每次按键，即使 Value 同为 `1` 也能计数。

---

# Distribute

## 1. 节点说明

Distribute 根据**单路输入**匹配规则表，在命中的输出端口发出布尔 `true` 触发，**不转发**输入的原始数值。适用于一路输入按条件驱动多路分支（如 MIDI 音符、场景号、状态码、定位数据分发）。

同一输入可匹配**多行**规则，从而同时触发多个输出端口。输出端口与规则行一一对应：Add Rule 增行加口，删行减口（至少保留 1 行）。

条件列支持 **JavaScript 表达式**，整份输入 `VariableData` 的 map 注册为全局变量 `$input`（与 Extract、Lookup、Condition 等节点一致）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | — |
| 输出 | 4 | 是 | 1 | 不限 | VariableData | 输出口由界面规则行驱动（Add Rule 增行加口、行 Action 删行减口），不走 PortEdit；至少保留 1 行 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | INPUT | VariableData | 参与条件匹配的完整输入对象 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 … N-1 | OUTPUT 0、OUTPUT 1 … | VariableData | 本轮命中的端口为 `true`（附 `_pulse`、`_distributedPort`、`_matchedRow`）；未命中的端口读取为 `false` |

## 3. 界面与属性

控件位于右侧属性面板（`WidgetEmbeddable=false`，不嵌入节点体）。规则表三列：

| 列 | 说明 |
|----|------|
| Condition | JS 条件表达式（见下表） |
| Port | 行号即输出端口索引（表格行与端口一一对应） |
| Action | 删除该行（同时删除对应输出端口） |

- **Add Rule**：新增规则并增加一个输出端口。
- 默认示例含 `$input.default == 0` 与 `*` 两行，可按需修改。

**条件写法：**

| 写法 | 含义 |
|------|------|
| `*` | 任意输入均匹配 |
| `$input.default == 0` | 标量输入（`default` 键）等于 0 |
| `$input['key'] > 10` | 取对象字段做比较 |
| `$input.boatId == 1` | 访问嵌套或顶层字段 |
| `== 0`、`> 10` 等 | 旧版比较写法；JS 求值出错时自动回退到数值比较（针对输入原始值） |
| （空） | 永不匹配 |

自上而下扫描**所有**行，所有命中行都会触发对应 Port。修改规则后，若已有输入缓存，会自动重新匹配。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），规则表只能在属性面板中编辑。

## 5. 行为细节

- **匹配流程**：输入到达（非空 VariableData）即缓存并分发——逐行求值条件（先 JS：`$input` 为输入 map；JS 报错回退 `ConditionMatch` 旧式数值比较；`*` 直接命中；空条件跳过）→ 每个命中行生成 `true` 输出（含 `_pulse`/`_distributedPort`/`_matchedRow`，三者值相同均为行号）并 emit dataUpdated。
- **输出缓存**：每轮分发前清空端口缓存，未命中端口回退为恒 `false` 的共享空输出；行删除时额外清空全部缓存并用当前输入重算。
- **规则变更**：界面 rulesChanged 即用缓存输入重新分发。
- **端口-行同步**：Add Rule / 删行联动输出端口增删（与 Inject 相同的 portsAboutToBeInserted/Deleted 机制）；`load` 时按规则行数直接对齐端口数。
- **持久化**：工程 JSON 顶层 `rules` 字段：`[{ "condition": "...", "port": "0" }, ...]`（非 `values` 子对象）。

## 6. 使用说明与典型接法

1. 配置 Condition 列（行号即输出端口）。
2. 将待分发的 VariableData 接到 INPUT。
3. 将各 OUTPUT 接到 Inject 的 TRIGGER 或仅需布尔触发的下游。
4. 无规则命中时不更新输出（端口读取为 false）。

## 7. 示例

| Condition | 行号（端口） | 效果 |
|-----------|------|------|
| `$input.default == 45` | `0` | 标量输入 45 → OUTPUT 0 为 true |
| `$input.default == 45` | `1` | 标量输入 45 → OUTPUT 1 也为 true |
| `$input.pointId == 5` | `2` | 定位点位 ID 为 5 → OUTPUT 2 为 true |
| `*` | `3` | 任意输入 → OUTPUT 3 为 true |

**典型链路：** 一路 MIDI 或 FT-LocationProto 的 POINT 输出接入 INPUT，多路 OUTPUT 分别接 Inject 的 TRIGGER，实现「按条件选路 → 注入不同预设值」。
