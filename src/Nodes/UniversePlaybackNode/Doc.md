# UniversePlaybackNode 使用说明

## 用途
把一个特定格式的视频文件解码为多路 Art-Net Universe 数据输出（每个输出端口对应 1 个 Universe）。

## 文件要求（重要）
- 使用 FFmpeg 7.1 解码。
- 编码必须为 FFV1。
- 分辨率必须为 512xH（H≥1）。
  - H = 输出 Universe 的数量。
- 像素格式：
  - 灰度（GRAY8）：每行 512 个字节直接映射为 DMX 512 通道。
  - 其它格式：每行按 4 字节/像素取第 1 个字节（R 通道）映射为 DMX（仍输出 512 通道）。

## 端口
### 输入（VariableData）
- PLAY：播放开关（bool）
- LOOP：循环开关（bool）
- STOP：停止（bool，true 时停止）
- CLEAR：清空 DMX 数据（bool，true 时清空）

### 输出（VariableData，动态）
- UNIVERSE1..UNIVERSEN：N = 视频高度 H
  - 每个端口的 default 为一个键值表（见下方输出字段）

## 外部控制（可选）
- /universe（int，0-15）：起始 Universe
- /subnet（int，0-15）
- /net（int，0-127）
- /loop（bool）
- /play（bool）
- /clear（bool 或任意值）：清空一次
- /filename（string）：选择文件

## 输出字段（UNIVERSE*）
每个输出端口的 `default` 是一个键值表，常用字段：
- protocol：Art-Net
- opcode：0x5000
- universe / subnet / net：当前输出地址
- fullUniverse：(net<<8) | (subnet<<4) | universe
- dmxData：长度 512 的数组（0-255）
- dataLength：512
- timestamp：时间戳（ms）

## 使用步骤
1. 选择视频文件（/filename 或界面选择）。
2. 设置 universe/subnet/net。
3. 将 PLAY 置为 true 开始输出；需要循环则开启 LOOP。
4. 将 UNIVERSE1..N 连接到下游 Art-Net 发送节点（或其他消费 DMX 数据的节点）。 
