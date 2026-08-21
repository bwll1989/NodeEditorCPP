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

## 2. 端口说明（Internal Commands / Osc Out Group）

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | TRIGGER | VariableData | 值为 true 时触发发送 |

### 输出

无（Internal Commands 的 outData 返回空 VariableData；Osc Out Group 同样无业务输出）。

## 3. 界面说明

- **消息列表**（OSCMessageListWidget）：编辑多条 OSC 消息。Internal Commands 变体通常仅允许**内部**地址；Osc Out Group 可配置对外发送。
- **发送按钮**（图标）：手动触发一次，等效于 TRIGGER 为 true。

**外部控制地址：**

- `/trigger` — 布尔 true 时执行列表（与点击发送按钮相同）；触发后会反馈 `/trigger` 状态。

列表内容保存在 `values` 中。

## 4. 使用说明

1. 在列表中添加目标 OSC 消息（地址与参数）。
2. 将 TRIGGER 接布尔脉冲，或点击发送按钮测试。
3. 需要由其它图节点远程触发时，向 `/trigger` 发送 true。

**单条消息多参数：** value 填 JSON/JS 数组即可，例如 `[0.1, 0.2, 0.3]` 或 `[1, "go"]`（以 `[` 开头即按列表解析）。发送时由 `OSCSender` 展开成一条多参 OSC（如 `,fff` / `,is`）。

Internal Commands 适合控制本应用内路由；Osc Out Group 适合发到外部设备或其它软件。

## 5. 示例

**场景宏：** 列表内写多条 `/cue/1`、`/light/master` 等，Show 开始时 TRIGGER 一次全部下发。  
**与 Switch 联动：** Switch 输出接不同 Internal Commands 节点的 TRIGGER，实现多套路由。  
**发三维坐标：** Osc Out Group 一条消息，address `/position`，value `[x,y,z]`。

---

# Osc Out Group

## 1. 节点说明

与 **Internal Commands** 界面相同：编辑 OSC 消息列表并批量发送。区别在于通过 **OSCSender** 向**外部**发送（节点库标签为 Connect），而非仅交给应用内 `StatusContainer`。

端口、界面与触发方式见上文（同一套 TRIGGER / 消息列表 / `/trigger`）。value 写数组即可发多参数，与 OSC Output 节点一致。

## 2. 使用说明

1. 配置对外 OSC 地址与参数（标量用 Int/Float/String；多参直接在 value 写 `[...]`）。
2. TRIGGER 或发送按钮触发后，按列表顺序发往外部。

---

# Snapshot


## 1. 节点说明

Snapshot 将当前 dataflow 中**选中节点**的配置序列化为快照，存入预设槽位。点击互斥预设按钮（同一时刻仅一个高亮）时，对原节点调用 `load()` 恢复参数，**不会创建新节点**。

## 2. 端口说明

### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | INDEX | 收到索引值时召回对应预设 |
| 1 | CAPTURE | 收到 `true` 时捕获当前选中节点到活动预设 |

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | ACTIVE | 当前活动预设索引 |

## 3. 界面说明

| 控件 | 说明 |
|------|------|
| 预设按钮 | 互斥按钮，点击即召回该预设 |
| 添加 | 新增空预设槽 |
| 删除 | 删除当前活动预设 |
| 捕获选中 | 将画布当前选中的节点（不含 Snapshot 自身）保存到活动预设 |

**外部控制：** `/index` 召回指定预设；`/capture` 发送 `true` 捕获。

## 4. 使用说明

1. 在画布中选中要纳入快照的节点（如 Switch、Color、DMX Device 等）。
2. 在 Snapshot 节点点「捕获选中」，保存到当前预设。
3. 调整现场参数后，点击预设按钮即可恢复。
4. INDEX 端口可接 Keyboard In、Switch、Inject 等驱动切场。
5. 部分节点由于初始化需要一定的时间及前后关系，可能出现不能按期望的执行。例如audio decoder不能自动播放等情况

**注意：** 快照按节点 ID 绑定；删除并重建节点后需重新捕获。仅恢复节点 `save()/load()` 中的字段，不含连线拓扑。
