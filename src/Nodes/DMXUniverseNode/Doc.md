# DMXUniverseNode

## 1. 节点说明

将多台 **DMX Device** 的输出按 DMX 地址合并为一路完整的 512 通道 Universe，并打包为 Art-Net 协议数据，供 **Artnet Out** 等节点发送。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| DEVICE1 … DEVICE8 | VariableData | 各 DMX Device 的 DEVICE 输出（默认 8 个，可编辑增删） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| UNIVERSE | VariableData | 完整 Art-Net 包：`protocol`、`universe`、`subnet`、`net`、`fullUniverse`、`dmxData`（512 路）等 |

## 3. 界面说明

- **Universe / Subnet / Net**：Art-Net 寻址（默认均为 0）。
- **启用**：关闭时对外输出的 DMX 数据全部为 0。
- **清空**：将所有 512 通道清零。

外部控制：`/universe`、`/subnet`、`/net`、`/enable`。

## 4. 使用说明

1. 配置 Universe、Subnet、Net，与控台或接收端一致。
2. 将各设备的 DEVICE 输出接到 DEVICE 端口。
3. UNIVERSE 输出接 **Artnet Out** 或其它需要完整 DMX 包的节点。
4. 地址 1～512 与设备 `addressMap` 对应；重叠地址以后写入为准。

## 5. 示例

两台设备：Device A 地址 1～4、Device B 地址 10～13 → 均接入本节点 → UNIVERSE → Artnet Out。
