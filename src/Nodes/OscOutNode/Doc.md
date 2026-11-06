# OSC Output

## 1. 节点说明

OSC 发送节点（界面标题为「OSC Output」），向指定主机和端口发送 Open Sound Control 消息。适合控制外部媒体服务器、灯光台或任何支持 OSC 的软件。

底层 `OSCSender` 支持**一条消息多个参数**：`default` 为列表时会展开成多个 OSC 参数（例如 `/vec 0.1 0.2 0.3`，format `fff`）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | — | 末端口为 TRIGGER（发送触发） |
| 输出 | 0 | 否 | 0 | 0 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | HOST | 设置目标 IP |
| 1 | PORT | 设置目标端口 |
| 2 | ADDRESS | 设置 OSC 地址并发送 |
| 3 | VALUE | 设置参数值并发送（改值即发送）；列表会发成多参 |
| 4 | TRIGGER | 输入为 `true` 时触发发送 |

### 输出

本节点无输出端口。

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 含义 |
|------|------|
| host | 目标 IP，默认 `127.0.0.1` |
| port | 目标端口，默认 8000 |
| address | OSC 路径，默认 `/test` |
| type | 单参数类型 Int / Float / String（仅标量时生效；列表发送时忽略） |
| value | 参数值；也可直接填 JSON 数组，如 `[0.1,0.2,0.3]`（输入即发送） |
| Send | 手动发送一次 |

可外部控制的属性（Q_PROPERTY）：`host`、`port`、`address`、`value`、`typeIndex`、`send`（脉冲）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/host` | 读写 | 目标 IP |
| `/port` | 读写 | 目标端口 |
| `/address` | 读写 | OSC 地址路径 |
| `/value` | 读写 | 参数值（可为列表）；外部写值会**立即发送** |
| `/type` | 读写 | 类型索引：0=Int，1=Float，2=String（标量） |
| `/send` | 命令 | 触发发送（无 payload 或 `true` 时执行） |

## 5. 行为细节

- **发送路径**：所有发送统一走 `OSCSender` 单例的发送队列（多节点共享，不阻塞界面）。
- **值解析**：发送前 value 字符串经 `fromDisplay` 解析——形如 `[..]` 的内容转为列表并按 `List` 类型发送（每个元素自动选 `f`/`i`/`s`）；标量按 type 框类型转换（无效转换回退 String）。
- **端口语义**：HOST/PORT 只更新目标；ADDRESS 更新地址并**立即发送**；VALUE 更新值并立即发送（列表按消费端展示串解析）；TRIGGER 仅在输入为 true 时发送。
- **send 脉冲**：`/send` 或 Send 按钮触发一次性发送，`send` 属性瞬时置 true 后自动复位。
- **界面即发**：value 输入框每敲一个字符（textChanged）都会触发发送，调试时可用；不希望逐字符发送请改用端口或 `/send`。
- **连接策略**：所有输入端口均为 Many。
- **持久化**：`save()` 将 `Port`、`Host`、`Address`、`Type`、`Value` 写入 `values.*`（键名首字母大写）；`load()` 逐项恢复（恢复 value 会触发一次发送）。

## 6. 使用说明与典型接法

### 发单个参数

1. 填写 host / port / address，type 选 Int / Float / String，填 value。
2. 点 **Send**，或从 **VALUE** / **TRIGGER** 驱动。

### 发多个参数（向量 / 颜色等）

任选其一：

1. **接端口（推荐）**：把 Vec Source、Color、LocationProto POS 等输出接到 **VALUE**。只要 `default` 是 float/int/string 列表，就会按元素展开发送（类型按元素自动选 `f`/`i`/`s`）。
2. **界面手写**：value 填 JSON 数组，例如 `[0,0,1]` 或 `[1,"go"]`，点 Send。

示例：

| 输入 `default` | 发出的 OSC |
|----------------|------------|
| `0.5`（Float） | `/test ,f 0.5` |
| `[0.1, 0.2, 0.3]` | `/vec ,fff 0.1 0.2 0.3` |
| `[1, 2, "ok"]` | `/mix ,iis 1 2 ok` |

## 7. 示例

- **控制 Resolume**：host 填控台 IP，address `/composition/columns/1/connect`，type String，value `1`。
- **发三维坐标**：Vec Source（Size=3）→ OscOut **VALUE**，address `/position`。
- **发颜色**：RGBA Color → OscOut **VALUE**，address `/color`（`ffff`）。
- **节拍闪灯**：LFO 输出连 **TRIGGER**，address `/light/strobe`，value 交替 0/1。

