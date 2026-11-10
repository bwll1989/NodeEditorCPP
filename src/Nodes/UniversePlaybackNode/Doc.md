# Universe Playback

## 1. 节点说明

从媒体库中的 **FFV1** 视频文件回放 DMX 数据：视频须为宽 512 像素、高为 Universe 路数；每行 512 字节对应一路 Universe 的 512 通道。支持播放、循环、停止、清空、seek 定位，并可输出多路 Universe 数据包（与 DMX Universe 格式兼容）。

支持**节点间同步（SyncData 体系）**：可作为从机被时间线等时钟源驱动（播放/暂停/seek 全跟随），也可作为主机发布自身回放位置供下游节点跟随。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | SyncData | 末端口（4）为 SYNC 从机跟随源 |
| 输出 | 5 | 是 | 1 | 不限 | VariableData | Universe 1…n + **末端口 SYNC 发布**；总数量 = 视频高度 + 1（末端口固定为 SYNC） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| PLAY | VariableData | `true` 开始播放，`false` 暂停 |
| LOOP | VariableData | 是否循环播放 |
| STOP | VariableData | `true` 时停止 |
| CLEAR | VariableData | `true` 时清空所有 DMX 数据 |
| SYNC | SyncData | **从机跟随源**：接入后播放/暂停/位置由源驱动（偏差超死区自动纠偏）；断开恢复本地控制 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| UNIVERSE1 … UNIVERSEn | VariableData | 各 Universe 的 Art-Net 数据包；数量由视频高度决定 |
| SYNC（末端口） | SyncData | **同步发布（主机）**：每帧发布回放位置 + 全局帧锚点，供下游节点跟随 |

## 3. 界面与属性

节点控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 说明 |
|------|------|
| Universe / Subnet / Net（数字拖动框） | 起始 Universe 编号；多路输出时按索引递增 Universe（模 16） |
| 文件（下拉/选择） | 从媒体库选择 FFV1 视频（512×N）；打开失败节点报错 |
| 播放按钮 | 开始/暂停回放 |
| 循环复选框 | 结束后自动回到开头继续 |
| 清空按钮 | 全部 DMX 通道清零 |
| 当前时间 | 显示回放进度 |

节点公开 7 个 `Q_PROPERTY`：

| 属性 | 类型 | 说明 |
|------|------|------|
| `universe` | int | 起始 Universe（0–15） |
| `subnet` | int | 子网 |
| `net` | int | Net |
| `isLooping` | bool | 是否循环 |
| `isPlaying` | bool | 是否播放（读写即启停） |
| `fileName` | QString | 媒体库中的文件名（赋值即尝试打开） |
| `position` | double | 回放位置（秒，读写：写入即 seek 定位） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/universe` | 可写（命令） | 整型，改起始 Universe |
| `/subnet` | 可写（命令） | 整型，改子网 |
| `/net` | 可写（命令） | 整型，改 Net |
| `/play` | 可写（命令） | 布尔（member=`isPlaying`），`true` 开始播放、`false` 停止 |
| `/loop` | 可写（命令） | 布尔，设置循环 |
| `/clear` | 可写（命令） | 清空所有 DMX 数据；模型回发 `/clear` true→false 脉冲反馈 |
| `/filename` | 可写（命令） | 字符串，改媒体库文件名并尝试打开 |
| `/position` | 可写（命令） | 数值（秒）：定位到指定位置（seek），支持播放中与停止态 |

全部 8 个地址均通过 `registerExternalBinding` 注册（绑定界面控件）并订阅 Command 事件；**没有 `/stop` 命令地址**（源码中已注释），外部停止用 `/play=false`。

## 5. 行为细节

- **端口语义**：PLAY=布尔直接映射播放/停止（`false` 即暂停/停止）；STOP/CREAL 仅 `true` 时生效；LOOP=布尔设置循环。
- **文件校验**：仅接受 **FFV1** 编码、宽 **512** 像素、高 ≥1 的视频；打开失败（尺寸/编码不符、文件不存在）→ 节点 Error + 界面提示。Universe 数 = 视频高度，输出端口数量 = Universe 数 + 1（末端口固定为 SYNC 发布）。
- **像素→DMX 映射**：每行 = 一个 Universe 的 512 通道；GRAY8 直接取字节，其他格式取每像素第 1 字节。
- **播放调度**：内部 QTimer 以 `Art-Net` 输出帧率（50fps，20ms）驱动，每 tick 解码一帧并重建全部 Universe 输出；**播放按解码帧节拍推进，不按 wall-clock 补帧**。
- **输出包结构**：每路输出为 Art-Net 变量包 `{protocol:"Art-Net", version:14, opcode:0x5000, universe:(起始+索引) mod 16, subnet, net, fullUniverse:(net<<8)|(subnet<<4)|universe, dmxData:512 字节列表, dataLength:512, activeChannels:0, timestamp}`。
- **seek（新增）**：`seekToSec` 按秒定位（钳制 [0, 时长]），seek 后 flush 解码器，下一 tick 输出目标位置帧；入口：`/position` OSC、`position` 属性、从机纠偏。
- **循环与停止**：读到文件尾时循环模式 seek 回 0 继续，否则自动停止；**非从机模式下停止操作会 seek 回 0**（再次播放从头开始，暂停用 PLAY=false）；**从机模式下停止定格在当前位置**（以源为准，不本地回绕），EOF 也不本地回绕。
- **SYNC 从机（In 4）**：100ms watcher 联动源播放态启停——源开始播放时**先定位到源位置再起播**（不从文件头闪帧）；位置纠偏在播放定时器内（源期望位置偏差超 1.5 帧时长即 seek 对齐）。**源位置超出本媒体时长时不起播**（定格待命，避免"起播→EOF 停→又起播"闪烁循环），源位置回到媒体范围内自动恢复跟随。
- **SYNC 主机（Out 末端口）**：每帧发布回放位置 + 全局帧计数锚点；停止/自然播完发布定格快照（playing=false），下游跟随端联动停止。
- **播放前无文件**：`isPlaying=true` 时若视频未打开且 `fileName` 非空，会自动尝试按媒体库路径重新打开再播放。
- **持久化**：`UniverseSettings` 子对象（**非 values**）：`universe`/`subnet`/`net`/`isLooping`/`videoFilePath`/`isPlaying`（兼容 `play`/`autoPlay` 旧键）；加载后恢复文件与编号，若文件存在则延迟 100ms 按保存状态恢复播放。
- **析构**：停止定时器（含 SYNC watcher）、清理 FFmpeg 上下文。

## 6. 使用说明与典型接法

1. 准备 FFV1、分辨率 512×Universe 数量的视频，放入媒体库。
2. 选择文件后，输出端口数量随视频高度自动调整（+1 个 SYNC 发布口）。
3. 各 UNIVERSE 输出可接 **Artnet Out**。
4. 也可用 PLAY / STOP 等输入端口做远程触发，`/position` 做 cue 点定位。
5. **节点间同步**：TimeLineNode 等时钟源的 SYNC 输出接 In 4 即成为从机（时间线驱动灯光）；Out 末端口 SYNC 可作为时钟源驱动其他节点。

## 7. 示例

预录灯光秀视频 → 本节点播放 → UNIVERSE1～4 分别接 4 个 Artnet Out 端口或合并后发送。

**时间线同步灯光**：TimeLineNode Out(SYNC) → Universe Playback In 4，时间线播放头驱动 DMX 素材回放；与视频/音频节点挂同一时钟源即可全场同步。

