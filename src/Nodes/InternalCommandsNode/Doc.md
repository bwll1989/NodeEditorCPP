# Internal Commands

## 1. 节点说明

Internal Commands 插件用于**批量发送 OSC 类命令**与**节点参数快照切场**，包含三种节点：

| 变体 | 节点库名称 | 行为 |
|------|------------|------|
| Internal Commands | Internal Commands | 将列表中的消息交给**内部状态容器**解析（`StatusContainer`），用于应用内命令 |
| Osc Out Group | Osc Out Group | 通过 **OSCSender** 向外部发送 OSC 消息（标签在库中为 Connect） |
| Snapshot | Snapshot | 捕获选中节点的 `save()` 状态，点击互斥预设按钮通过 `load()` 召回 |

二者界面相同：OSC 消息列表 + 发送测试按钮；触发后按列表顺序执行全部消息。

**Snapshot** 用于切场：先在画布选中要管理的节点 → 点「捕获选中」→ 点击预设按钮一键恢复参数。

## 2. 端口说明

### Internal Commands / Osc Out Group

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 0 | 否 | 0 | 0 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | TRIGGER | VariableData | 值为 true 时触发发送 |

### 输出

无（Internal Commands 的 outData 返回空 VariableData；Osc Out Group 同样无业务输出）。

## 3. 界面与属性

- **消息列表**（OSCMessageListWidget）：编辑多条 OSC 消息。Internal Commands 变体通常仅允许**内部**地址；Osc Out Group 可配置对外发送。
- **发送按钮**（图标）：手动触发一次，等效于 TRIGGER 为 true。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/trigger` | 读/写 | 布尔 true 时按列表顺序执行（与点击发送按钮相同）；触发后发送 `/trigger` 状态反馈脉冲 |

## 5. 行为细节

- **触发语义**：TRIGGER 端口 `asBool()==true` 才执行（false/空数据忽略）；OSC `/trigger` 同样仅响应 true（`setTrigger(false)` 直接返回）。
- **执行方式**：逐条取出列表消息交给 `StatusContainer::parseOSC` 解析（应用内命令路由）；无输出端口（outData 恒为空 VariableData）。
- **状态反馈**：每次执行后向 `/trigger` 发送 `stateFeedBack(true)` 脉冲。
- **连接策略**：输入/输出均 Many。
- **持久化**：`values` = 消息列表控件的整体 `save()`（列表内容随工程保存）。

## 6. 使用说明与典型接法

1. 在列表中添加目标 OSC 消息（地址与参数）。
2. 将 TRIGGER 接布尔脉冲，或点击发送按钮测试。
3. 需要由其它图节点远程触发时，向 `/trigger` 发送 true。

**单条消息多参数：** value 填 JSON/JS 数组即可，例如 `[0.1, 0.2, 0.3]` 或 `[1, "go"]`（以 `[` 开头即按列表解析）。发送时由 `OSCSender` 展开成一条多参 OSC（如 `,fff` / `,is`）。

Internal Commands 适合控制本应用内路由；Osc Out Group 适合发到外部设备或其它软件。

## 7. 示例

**场景宏：** 列表内写多条 `/cue/1`、`/light/master` 等，Show 开始时 TRIGGER 一次全部下发。  
**与 Switch 联动：** Switch 输出接不同 Internal Commands 节点的 TRIGGER，实现多套路由。  
**发三维坐标：** Osc Out Group 一条消息，address `/position`，value `[x,y,z]`。

---

# Osc Out Group

## 1. 节点说明

与 **Internal Commands** 界面相同：编辑 OSC 消息列表并批量发送。区别在于通过 **OSCSender** 向**外部**发送（节点库标签为 Connect），而非仅交给应用内 `StatusContainer`。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 0 | 否 | 0 | 0 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | TRIGGER | VariableData | 值为 true 时按列表顺序向外部发送；断开时：仅界面发送按钮或 OSC `/trigger` 可触发 |

### 输出

无业务输出。

## 3. 界面与属性

内嵌界面与 Internal Commands 相同：OSC 消息列表（可配置对外地址与参数，value 写 `[...]` 即多参数）+ 发送按钮。

本节点除触发外无可配置 Q_PROPERTY 属性。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/trigger` | 读/写 | 布尔 true 时按列表顺序向外部发送（与点击发送按钮相同）；触发后发送 `/trigger` 状态反馈脉冲 |

## 5. 行为细节

与 Internal Commands 一致：TRIGGER `asBool()==true` 执行、OSC `/trigger` 仅响应 true、逐条发送后回读 `/trigger` 脉冲、列表存 `values`。区别仅在执行通道——消息经 `OSCSender` 发往外部目标，而非应用内 `StatusContainer`。

## 6. 使用说明与典型接法

1. 配置对外 OSC 地址与参数（标量用 Int/Float/String；多参直接在 value 写 `[...]`）。
2. TRIGGER 或发送按钮触发后，按列表顺序发往外部。

## 7. 示例

**外部灯光台：** 列表写 `/light/master` 值 `[0.8]`，Trigger Source → TRIGGER，一键推总控。  
**多参坐标：** address `/position`，value `[x,y,z]`，发往外部追踪系统。

---

# Snapshot

## 1. 节点说明

Snapshot 将当前 dataflow 中**选中节点**的配置序列化为快照，存入预设槽位。点击互斥预设按钮（同一时刻仅一个高亮）时，对原节点调用 `load()` 恢复参数，**不会创建新节点**。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 末端口为 CAPTURE（捕获当前选中节点到活动预设） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | INDEX | 收到索引值时召回对应预设 |
| 1 | CAPTURE | 收到 `true` 时捕获当前选中节点到活动预设 |

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | ACTIVE | 当前活动预设索引 |

## 3. 界面与属性

| 控件 | 说明 |
|------|------|
| 预设按钮 | 互斥按钮，点击即召回该预设 |
| 添加 | 新增空预设槽 |
| 删除 | 删除当前活动预设 |
| 捕获选中 | 将画布当前选中的节点（不含 Snapshot 自身）写入**当前活动预设** |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/index` | 读/写 | 召回指定索引的预设（payload 转 int；越界时状态栏提示「预设索引无效」） |
| `/capture` | 写 | payload 为 true 时捕获当前选中节点到活动预设 |

## 5. 行为细节

- **召回流程**：`/index` 或点击预设按钮 → 校验索引 → 切换活动预设 → 校验当前 dataflow 标题 → `GraphSnapshotBridge::applySnapshot` 对快照内节点逐个调用 `load()`（按节点 ID 绑定，不创建新节点）→ 成功后输出端口更新为活动索引并 emit dataUpdated。
- **捕获流程**：「捕获选中」或 `/capture`=true → 收集画布选中节点 ID（排除自身）→ `GraphSnapshotBridge::captureNodes` 序列化各节点 `save()` 状态写入活动预设；未选中节点时状态栏提示。
- **预设管理**：添加自动切到新槽；删除至少保留 1 个预设，活动索引自动纠正；空预设召回时状态栏提示「请先点更新写入」。
- **持久化**：`values.activeIndex` + `values.presets`（各预设的节点快照数组）。
- **已知限制**：仅恢复节点 `save()/load()` 字段，不含连线拓扑；删除重建节点后需重新捕获；部分依赖初始化时序的节点（如 Audio Decoder 自动播放）召回后可能不按期望执行。

## 6. 使用说明与典型接法

1. 在画布中选中要纳入快照的节点（如 Switch、Color、DMX Device 等）。
2. 在 Snapshot 节点点「捕获选中」，保存到当前预设。
3. 调整现场参数后，点击预设按钮即可恢复。
4. INDEX 端口可接 Keyboard In、Switch、Inject 等驱动切场。
5. 部分节点由于初始化需要一定的时间及前后关系，可能出现不能按期望的执行。例如audio decoder不能自动播放等情况

**注意：** 快照按节点 ID 绑定；删除并重建节点后需重新捕获。仅恢复节点 `save()/load()` 中的字段，不含连线拓扑。

## 7. 示例

**双场景切换：** 预设 1 捕获 Switch 位置 A + Color 暖色；预设 2 捕获位置 B + 冷色；Keyboard In 数字键 → INDEX 端口，按 1/2 一键切场。

