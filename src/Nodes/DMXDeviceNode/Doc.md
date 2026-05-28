# DMXDeviceNode

## 1. 节点说明

表示一台 DMX 灯具或设备：设置起始地址、通道数，并为每个通道维护 0～255 的数值。可将多路输入合并为带地址映射的设备数据，供 **DMX Universe** 节点合并进完整 Universe。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| CH0 … CHn | VariableData | 各通道数值（0～255），端口数随「通道数」变化，默认 5 个，可编辑增删 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| DEVICE | VariableData | 设备数据（`default` 为键值表），含 `startAddress`、`channelCount`、`endAddress`、`enabled`、`channels`、`addressMap` |

## 3. 界面说明

- **通道数**：设备占用 DMX 通道数量（默认 5）。
- **起始地址**：DMX 起始地址（默认 1）。
- **启用**：关闭时输出通道值均为 0。
- **通道滑块**：手动调节各通道 0～255。
- **地址范围**：显示当前占用的 DMX 地址区间。

外部控制：`/channels`、`/startAddress`、`/enable`、`/channels/N`（单通道）。

## 4. 使用说明

1. 设置起始地址与通道数，与现场编址一致。
2. 用滑块或 CH 输入端口写入各通道值。
3. 将 DEVICE 输出接到 **DMX Universe** 的 DEVICE 输入。
4. 多台设备时，每台一个本节点，地址不要重叠。

## 5. 示例

一台 4 通道调光：起始地址 1，CH0～CH3 接曲线或逻辑节点；DEVICE → DMX Universe DEVICE1。
