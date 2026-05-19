# DMXUniverseNode 使用说明

## 用途
DMXUniverseNode 用于把多个 DMXDeviceNode 输出的 `addressMap` 合并为一个完整的 512 通道 Universe，并输出包含 Art-Net 发送所需信息的数据包结构。

## 端口
- 输入（默认 8 个，可编辑）
  - DEVICE1..DEVICE8：接收 DMXDeviceNode 的 DEVICE 输出（VariableData）
- 输出（1）
  - UNIVERSE：VariableData，包含 512 通道 DMX 数据与 Universe 寻址信息

输出 `default` 字段常用内容：
- `protocol`：Art-Net
- `opcode`：0x5000
- `universe/subnet/net`：寻址参数
- `fullUniverse`：组合后的完整 Universe 值
- `dmxData`：长度 512 的通道值列表（0-255）
- `activeChannels`：非零通道数量
- `maxValue`：最大通道值
- `timestamp`：时间戳

## 节点参数/界面
- universe：Universe（0-15）
- subnet：Subnet（0-15）
- net：Net（0-127）
- enabled：启用/禁用输出（禁用时输出的 dmxData 全为 0）

## 使用步骤
1. 添加一个或多个 DMXDeviceNode，分别设置它们的 startAddress / channels，并输出 DEVICE。
2. 将各 DEVICE 连接到 DMXUniverseNode 的 DEVICE1..DEVICE8。
3. 设置 universe/subnet/net。
4. 将 UNIVERSE 输出连接到 ArtnetOut 等发送节点。

## 外部控制（可选）
- `/universe`：设置 Universe（int）
- `/subnet`：设置 Subnet（int）
- `/net`：设置 Net（int）
- `/enable`：启用/禁用（bool）

## 注意事项
- 地址冲突：多个 DEVICE 若写入同一 DMX 地址，后到的数据会覆盖先到的数据。
- 该节点不提供 `/clear` 外部地址；如需清零，建议在上游设备节点禁用输出或将通道值置 0。
