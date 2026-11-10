# TimeLineNode

## 1. 节点说明

TimeLineNode 是工程内的**多轨时间轴**节点：可编排轨道与剪辑，控制播放、暂停、停止与循环，并支持通过数据流端口或外部命令驱动。时间轴状态（轨道、剪辑、当前帧等）随工程保存。

时间线可作为**图内时钟源**：通过 Out SYNC 端口持续发布播放头位置（播放、拖动、seek 均触发心跳），驱动 Audio/Video Decoder、Universe Playback 等所有 SYNC 从机节点；也可通过 In SYNC 端口作为从机跟随外部时钟源。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | SyncData | 末端口（4）为 SYNC 跟随源 |
| 输出 | 1 | 否 | 1 | 1 | SyncData | 0 = SYNC 发布（本时间线作为图内时钟源） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | PLAY | VariableData | 布尔 true → 开始播放 |
| 1 | STOP | VariableData | 布尔 true → 停止播放 |
| 2 | PAUSE | VariableData | 布尔 true → 暂停 |
| 3 | LOOP | VariableData | 布尔 → 设置是否循环 |
| 4 | SYNC | SyncData | **跟随源**：接入后时间线播放头跟随源（源播放态联动启停，位置偏差 >1 帧自动纠偏）；断开恢复本地时钟 |

### 输出

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | SYNC | SyncData | **图内时钟源发布**：播放头位置（当前帧/帧率）+ 全局帧计数锚点，播放/拖动/seek 均触发心跳，可驱动多个下游从机 |

## 3. 界面与属性

节点控件为完整的**时间轴编辑器**（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：轨道列表、剪辑编辑、播放控制条（播放/暂停/停止、循环、播放头拖动、当前帧显示）。

节点公开 3 个 `Q_PROPERTY`：

| 属性 | 类型 | 说明 |
|------|------|------|
| `playing` | bool | 是否正在播放（写 `true` 开始、`false` 停止） |
| `looping` | bool | 是否循环播放 |
| `currentFrame` | qint64 | 当前帧（写入即跳转，等效拖动播放头） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/play` | 可写（命令） | 布尔（无效载荷视为 `true`）：开始播放；`false` 停止 |
| `/stop` | 可写（命令） | 停止播放（无 member 绑定，纯命令地址） |
| `/pause` | 可写（命令） | 暂停播放，保持当前帧（纯命令地址） |
| `/loop` | 可写（命令） | 布尔，设置循环 |
| `/currentFrame` | 可写（命令） | 整型，跳转到指定帧 |

`/play`、`/loop`、`/currentFrame` 通过 `registerExternalBinding` 注册（member 绑定到同名属性）；`/stop`、`/pause` 仅订阅 Command 事件，无 member 绑定。

**状态反馈**：播放状态变化反馈到 `/stop`（值 = !playing）、循环状态反馈到 `/loop`、当前帧每次变化反馈到 `/currentFrame`，节点就绪时先发一次 `/stop` 初始状态，便于控台同步。

## 5. 行为细节

- **端口语义**：PLAY=布尔直接映射播放/停止；STOP/PAUSE 仅 `true` 时生效；LOOP=布尔设置循环。端口更新后即回发 `dataUpdated`。
- **暂停与停止的区别**：暂停（PAUSE/`/pause`）保持当前帧可继续；停止（STOP/`/play=false`）回到停止状态。
- **时钟**：内部由 `TimeLineNodeClock` 驱动帧推进；属性值与工具栏操作最终都汇入同一时钟，状态以时钟信号为准同步（去重：相同值不重复发信号）。
- **SYNC 主机（Out 0）**：`currentFrameChanged`（tick/拖动/seek）与播放态变化时即时发布同步状态——位置=当前帧/帧率，锚点=写入时刻的 TimestampGenerator 全局帧计数，跟随端按全局时钟外推。
- **SYNC 从机（In 4）**：40ms（25Hz）轮询同步源——源播放态联动本机启停；播放中按 `期望秒 × 帧率` 换算目标帧，与本地播放头偏差 >1 帧时 `setCurrentFrame` 纠偏（本地 Internal tick 继续填充帧间空隙，外部驱动为主），位置按时间线时长钳制、循环模式下取模回绕。
- **剪辑就绪**：节点就绪后为所有已加载剪辑调用 `onModelReady()`，完成剪辑的控件注册与初始状态反馈；工具栏绑定到本节点的 OSC 前缀。
- **持久化**：`values` 保存整个时间轴状态（轨道、剪辑、时钟状态等，由内部 `TimeLineNodeModel` 序列化/反序列化）；加载后按数据重建轨道与剪辑。
- **析构**：自动停止播放。

## 6. 使用说明与典型接法

1. 在节点内创建轨道与剪辑，保存工程。
2. 用 PLAY/STOP/PAUSE/LOOP 端口或外部地址驱动演出控制。
3. **作为时钟源**：Out SYNC 接 Audio/Video Decoder、Universe Playback 等节点的 SYNC 输入，时间线播放头即驱动全场同步。
4. **作为从机**：外部时钟源（如另一条时间线、LTC Decoder 的 SYNC 输出）接 In 4，时间线跟随外部时钟；断开连线恢复本地时钟。

## 7. 示例

**自动开场：** 上游 Condition 为真时向 PLAY 发送 true，时间轴开始播放开场序列。  
**控台同步：** 外部 OSC 映射到 `/currentFrame`，跳转到指定帧做 cue 点定位。  
**全场同步**：TimeLineNode Out(SYNC) 同时接 Video Decoder In 4、Audio Decoder In 3、Universe Playback In 4，一条时间线驱动音视频与灯光同步播放。

