# DMX Device

## 1. 节点说明

表示一台 DMX 灯具或设备：设置起始地址、通道数，并为每个通道维护 0～255 的数值。可将多路输入合并为带地址映射的设备数据，供 **DMX Universe** 节点合并进完整 Universe。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 是 | 1 | 不限 | VariableData | 末端口为 Vec（通道值列表整包写入，始终位于最后） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| CH0 … | VariableData | 单通道数值（0～255）；前 `n-1` 口对应下标，可编辑增删 |
| Vec（最后一口） | VariableData | 通道值列表；按当前通道数截取或补 0，一次性写入全部通道 |

默认 5 个输入口：CH0～CH3 + Vec。

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| DEVICE | VariableData | 设备数据（`default` 为键值表），含 `startAddress`、`channelCount`、`endAddress`、`enabled`、`channels`、`addressMap` |

## 3. 界面与属性

- **通道数**：设备占用 DMX 通道数量（默认 5）。
- **起始地址**：DMX 起始地址（默认 1）。
- **启用**：关闭时输出通道值均为 0。
- **通道滑块**：手动调节各通道 0～255。
- **地址范围**：显示当前占用的 DMX 地址区间。

公开属性：`channelCount`（int）、`startAddress`（int）、`enabled`（bool）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/channels` | 读/写 | 绑定属性 `channelCount`（通道数） |
| `/startAddress` | 读/写 | 绑定属性 `startAddress`（起始地址） |
| `/enable` | 读/写 | 绑定属性 `enabled`（启用开关） |
| `/channels/N` | 读/写 | 单通道值（0～255，越界钳制） |

## 5. 行为细节

- 单通道口写入 `qBound(0, round(值), 255)`；Vec 口按当前通道数截取/补 0 整包写入。
- 输出 DEVICE 键值表：`startAddress`、`channelCount`、`endAddress`、`enabled`、`channels`（当前值数组）、`addressMap`。
- 持久化：`ChannelSettings`（通道数、起始地址、启用、通道值数组）。

## 6. 使用说明与典型接法

1. 设置起始地址与通道数，与现场编址一致。
2. 用滑块、CH 输入或 **Vec** 列表写入各通道值。
3. 将 DEVICE 输出接到 **DMX Universe** 的 DEVICE 输入。
4. 多台设备时，每台一个本节点，地址不要重叠。

## 7. 示例

一台 4 通道调光：起始地址 1，CH0～CH3 接曲线或逻辑节点；或 Vec 接 `[dimmer, r, g, b]`；DEVICE → DMX Universe DEVICE1。

