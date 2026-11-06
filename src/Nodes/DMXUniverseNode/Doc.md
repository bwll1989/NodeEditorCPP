# DMX Universe

## 1. 节点说明

将多台 **DMX Device** 的输出按 DMX 地址合并为一路完整的 512 通道 Universe，并打包为 Art-Net 协议数据，供 **Artnet Out** 等节点发送。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 8 | 是 | 1 | 不限 | VariableData | 无 |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| DEVICE1 … DEVICE8 | VariableData | 各 DMX Device 的 DEVICE 输出（默认 8 个，可编辑增删） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| UNIVERSE | VariableData | 完整 Art-Net 包：`protocol`、`universe`、`subnet`、`net`、`fullUniverse`、`dmxData`（512 路）等 |

## 3. 界面与属性

- **Universe / Subnet / Net**：Art-Net 寻址（默认均为 0）。
- **启用**：关闭时对外输出的 DMX 数据全部为 0。
- **清空**：将所有 512 通道清零。

公开属性：`universe`、`subnet`、`net`（int）、`enabled`（bool）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/universe` | 读/写 | 绑定属性 `universe`（Art-Net Universe 号） |
| `/subnet` | 读/写 | 绑定属性 `subnet`（Subnet 号） |
| `/net` | 读/写 | 绑定属性 `net`（Net 号） |
| `/enable` | 读/写 | 绑定属性 `enabled`（启用开关） |

## 5. 行为细节

- 按 DMX Device 输出中的 `addressMap` 将各通道值写入 512 通道缓冲；重叠地址以后写入的设备为准。
- 输出为完整 Art-Net 包键值表（`protocol`、`universe`、`subnet`、`net`、`fullUniverse`、`dmxData` 等）。
- 持久化：`UniverseSettings`（universe / subnet / net）。

## 6. 使用说明与典型接法

1. 配置 Universe、Subnet、Net，与控台或接收端一致。
2. 将各设备的 DEVICE 输出接到 DEVICE 端口。
3. UNIVERSE 输出接 **Artnet Out** 或其它需要完整 DMX 包的节点。
4. 地址 1～512 与设备 `addressMap` 对应；重叠地址以后写入为准。

## 7. 示例

两台设备：Device A 地址 1～4、Device B 地址 10～13 → 均接入本节点 → UNIVERSE → Artnet Out。

