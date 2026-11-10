# MTC Generator

## 1. 节点说明

向指定 MIDI 输出端口发送 MTC（MIDI Time Code），供 DAW、音视频设备、灯光台等外部硬件/软件跟随时间码走带。

同时是 **SYNC 体系的输出边界节点**：可绑定图内同步时钟（SyncData 输入），把图内播放位置实时转换为 MTC QF 流发出——图内同步不绑定 MIDI 语义，换算只在边界完成。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | SyncData | 末端口（2）为 SYNC 同步源输入 |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

### 输入

- **START**（VariableData）：非 0 = 开始发送，0 = 停止。
- **RESET**（VariableData）：上升沿触发归零——时间码复位到 00:00:00:00 并立即发 Full-Frame 定位。
- **SYNC**（SyncData）：**图内同步源**——绑定后进入跟随模式：源播放中按全局时钟外推期望位置（含 `syncOffset` 偏移），帧差超过死区（1 帧）时重新对齐（Full-Frame + QF 重置）；源暂停/失效时停发 QF 并定格；断开连线立即定格并停发。未绑定则按本地时钟自由生成。

### 输出

- **TIMECODE**（VariableData）：当前时间码字符串（时:分:秒.帧，含用户偏移）。
- **START**（VariableData）：当前运行状态（true = 发送中）。

## 3. 界面与属性

时间码显示面板 + 状态（ready / Generating / stopped / 设备错误）+ **MIDI 输出**下拉 + **偏移**（帧，-100~100，仅影响显示与 TIMECODE 输出，不影响发出的 MTC）+ **类型**下拉（Film 24 / Film Drop 23.976 / NTSC 30 / NTSC Drop 29.97 / PAL 25）+ Start/Reset 按钮。`WidgetEmbeddable=false`，控件在右侧属性面板。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 类型 | 说明 |
|------|------|------|
| `/start` | bool | 播放/停止 |
| `/offset` | int | 显示偏移（帧） |
| `/reset` | bool | 瞬时命令：true 触发归零（自动回 false） |
| `/device` | string | 切换 MIDI 输出设备（`"索引: 名称"`） |
| `/syncOffset` | double | SYNC 秒 → MTC 时间码的映射偏移（秒，可负） |

## 5. 行为细节

- **发送架构**：MtcSender 用 5ms 精确 QTimer 按耗时折算应发 QF 数（追赶式，单次追赶上限 64 条），不依赖 Windows 定时器精度，长期走带不漂移。
- **消息形态**：播放启动/跳转/停止/同步对齐时先发一条 Full-Frame SysEx（`F0 7F 7F 01 01 …`）定位，随后进入 QF 流（0xF1，4×帧率）。piece 0~3 携带 2 帧窗口起始时间，piece 4~7 携带 +1 帧时间（MTC 规范）。
- **帧率标志**：Full-Frame 小时字节高 2 位与 QF piece 7 编码帧率（24/25/30DF/30），接收端据此自动识别格式。
- **SYNC 跟随**：每 tick 读源快照，`期望位置 = expectedNowSec() + syncOffset`；与当前编码位置帧差 >1 帧时 `setTimecode + Full-Frame` 重对齐；源失效（active=false 或 >500ms 无心跳）→ 定格（`_syncHold`），恢复后自动重新锁定。
- **设备防御**：未选设备/空设备名不打开任何端口；设备或类型变更时重建发送器并恢复播放状态。
- **持久化**：工程 JSON 顶层 `offset / device / timecodeType / running / syncOffset` 字段。

## 6. 使用说明与典型接法

1. 安装 loopMIDI（或使用声卡 MIDI 输出），创建虚拟 MIDI 端口。
2. 本节点 **MIDI 输出** 选择该端口；对端 DAW/设备把该端口设为 MTC 输入源。
3. 点 Start（或 START 端口/OSC `/start`）开始发送。
4. **图内同步**：将时间线等节点的 SYNC 输出接入本节点 SYNC 端口，图内播放位置即以 MTC 形式对外发布（`syncOffset` 可做音画/演出偏移补偿）。

## 7. 示例

**自由生成**：MTC Generator（loopMIDI Port 1）→ DAW 开启 MTC 同步 → DAW 走带跟随。

**SYNC→MTC 边界**：TimeLineNode Out(SYNC) → MTC Generator In(SYNC) → loopMIDI → 灯光台/音视频播放器跟随时间线。
