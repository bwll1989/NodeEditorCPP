# OSC Output

## 1. 节点说明

OSC 发送节点（界面标题为「OSC Output」），向指定主机和端口发送 Open Sound Control 消息。适合控制外部媒体服务器、灯光台或任何支持 OSC 的软件。

底层 `OSCSender` 支持**一条消息多个参数**：`default` 为列表时会展开成多个 OSC 参数（例如 `/vec 0.1 0.2 0.3`，format `fff`）。

## 2. 端口说明

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

## 3. 界面说明

- **host**：目标 IP，默认 `127.0.0.1`。
- **port**：目标端口，默认 8000。
- **address**：OSC 路径，默认 `/test`。
- **type**：单参数类型 Int、Float 或 String（仅标量时生效；列表发送时忽略）。
- **value**：参数值；也可直接填 JSON 数组，如 `[0.1,0.2,0.3]`。
- **Send**：手动发送一次。

## 4. 使用说明

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

**外部控制路径**（完整地址：`/dataflow/{父级别名}/{节点ID}{相对路径}`）：

| 相对路径 | 作用 |
|----------|------|
| `/host` | 目标 IP |
| `/port` | 目标端口 |
| `/address` | OSC 地址路径 |
| `/value` | 参数值（可为列表） |
| `/type` | 类型索引：0=Int，1=Float，2=String（标量） |
| `/send` | 触发发送（命令；无 payload 或 `true` 时执行） |

支持 OSC / 全局事件总线命令。

## 5. 示例

- **控制 Resolume**：host 填控台 IP，address `/composition/columns/1/connect`，type String，value `1`。
- **发三维坐标**：Vec Source（Size=3）→ OscOut **VALUE**，address `/position`。
- **发颜色**：RGBA Color → OscOut **VALUE**，address `/color`（`ffff`）。
- **节拍闪灯**：LFO 输出连 **TRIGGER**，address `/light/strobe`，value 交替 0/1。
