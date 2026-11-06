# Flow 节点图（.flow / DataFlow）JSON 格式规范

本规范描述 Flow 工程文件中 `DataFlow` 字段的 JSON 结构。你（AI）生成或修改节点图时必须严格遵守。

## 顶层结构

完整 `.flow` 工程文件顶层字段（**你只需要生成/修改 `DataFlow` 字段，其余由工具自动保留**）：

```json
{
  "appVersion": "1.6.14",
  "DataFlow": {
    "scene": { "nodes": [...], "connections": [...], "groups": [...] }
  },
  "TimeLine": { "...": "时间轴数据，不要改" },
  "ScheduledTasks": { "...": "计划任务，不要改" }
}
```

⚠️ **scene 包装**：当前版本 Flow 的 `DataFlow` 字段内层有一个 `"scene"` 键
（`DataflowViewsManger::load` 只读 `scene`，找不到就返回空图）。
你不需要自己包 scene：`apply_graph` 接受裸的 `{nodes, connections, groups}` 并自动包装；
`get_current_graph` 返回的也是拆包后的内容。

`apply_graph` 工具接受的输入：`{"nodes": [...], "connections": [...], "groups": [...]}`
（也兼容传入完整工程或已带 scene 包装的对象）。

## nodes 数组——节点对象

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | int | 是 | 节点唯一编号，工程内不得重复。建议用不与现有冲突的递增整数 |
| `type` | string | 是 | **节点注册名，必须与 `search_nodes`/`get_node_doc` 返回的规范名完全一致**（含大小写、空格、斜杠） |
| `position` | object | 建议 | `{"x": 数值, "y": 数值}`，画布坐标。新增节点时给出，避免全部叠在原点 |
| `input-count` | int | 是 | 当前输入端口数量（可编辑端口的节点按实际数量填写） |
| `output-count` | int | 是 | 当前输出端口数量 |
| `port-editable` | bool | 建议 | 端口数量是否可编辑（与节点文档"端口数可编辑"一致，一般保持原图中的值） |
| `remarks` | string | 建议 | 节点备注，显示在画布上。给它一个说明用途的中文名 |
| `internal-data` | object | 谨慎 | 节点内部序列化状态（脚本、文件路径、已配置参数）。**见下方"internal-data 规则"** |

## connections 数组——连接对象

⚠️ **字段名陷阱**：目标节点的字段名是 `intNodeId`（Qt 历史拼写，意为 into-node-id），**不是** `inNodeId`！必须写 `intNodeId`，写 `inNodeId` 会被静默忽略导致连接丢失。

```json
{
  "outNodeId": 6,
  "outPortIndex": 0,
  "intNodeId": 5,
  "inPortIndex": 0
}
```

| 字段 | 说明 |
|------|------|
| `outNodeId` | 源节点 id |
| `outPortIndex` | 源节点输出端口索引，从 0 开始，必须 < 该节点 `output-count` |
| `intNodeId` | 目标节点 id（注意拼写：int，不是 in） |
| `inPortIndex` | 目标节点输入端口索引，从 0 开始，必须 < 该节点 `input-count` |

数据流向：`outNodeId → intNodeId`。

## internal-data 规则（最高优先级）

1. **新建节点**：`internal-data` 写空对象 `{}`，或者干脆省略。**绝不凭空构造**——每个节点的内部数据结构各不相同，捏造会导致节点加载异常。
2. **保留已有节点**：修改图时，原图中已有节点的 `internal-data` 必须**原样复制**到新 JSON 中（除非你明确要改它的某个已文档化配置）。`get_current_graph` 默认会剥离该字段——需要改已有节点的内部配置时，用 `full=true` 拿完整数据。
3. 不确定某节点有哪些可配参数时，先用 `get_node_doc` 查它的属性说明；属性值通常通过外部控制地址（OSC）或端口设置，而不是直接改 `internal-data`。

## 工作流程（搭图/改图标准步骤）

1. `get_app_info` — 确认 Flow 在线。
2. `get_current_graph` — 拿当前图（精简，无 internal-data）。无工程时告知用户先在 Flow 里保存一个空工程，或直接从空图开始（工具会用最小模板打底）。
3. `search_nodes(关键词)` / `get_node_doc(类型名)` — 查节点能力、端口、属性、端口数可编辑范围。**不确定节点名时必须先查**。
4. 在当前图基础上做**增量修改**：保留不变的节点与连接，只增/删/改需要的部分。禁止推倒重画。
5. 生成新的 `DataFlow` JSON → 调 `apply_graph`。工具会做校验（type 存在性、id 重复、连接索引越界、别名归一化）并自动备份。
6. 应用成功后图已在 Flow 中加载。如需运行时调参数，用 `send_command`（地址形如 `/dataflow/{别名}/{节点ID}/{参数}`）。

## 端口与连接注意事项

- 每个节点的默认端口数、是否可编辑、最小/最大数量、末端口差异，查 `get_node_doc`。
- 可编辑端口的节点（如求和、选择器）：增加输入时同步修改 `input-count`，新增端口的类型见文档"新增端口类型"列。
- **末端口差异**：部分节点最后一个端口的类型/语义与前面的通用端口不同（文档"末端口差异"列有说明），连线前务必确认。
- 端口类型要匹配（VariableData 通用变量 / AudioData 音频 / ImageData 图像 / GpuTexture 纹理）；类型不匹配的连接可能被 Flow 拒绝或产生无数据输出。
