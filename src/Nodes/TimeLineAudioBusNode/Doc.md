# TimeLineAudioBus

时间线音频进入节点图的入口节点（2026-11-13《TimeLineAudioBus实施方案.md》T2 落地）。

## 功能

- **无输入端口**。0 号口 = SYNC（SyncData：持续发布 timeline 播放头快照），1..N 号口 = AudioData（每口一个命名环形队列，bus 名 = 端口名 `BUS i`）。
- 端口编辑：`PortEditable=true`，节点右键/`Ctrl+P` 增删输出端口（0 号 SYNC 口建议勿删）。端口数持久化进 `.flow`。
- 队列唯一出生点在本节点：创建即注册进 `TimelineAudioBusRegistry`（DataTypes），runtime/timeline 混音侧按名 `acquire` 后作为**唯一写者**每 tick 写入（时间戳 = 全局帧计数 + `FIXED_DELAY_FRAMES`，块长 = `getSamplesPerFrame(48000)`）。

## 属性（OSC 可控）

| 属性 | 说明 |
|---|---|
| `audioPorts` | 音频输出端口数（1..16，不含 SYNC 口），增删即注册/注销对应 bus |
| `driveMode` | `external`（默认，runtime 写）/ `test`（440Hz 正弦冒烟）/ `silence`（静音占位） |
| `followClock` | true：SYNC 播放头 = 全局时钟换算秒；false：使用 `positionSec` 属性值 |
| `positionSec` | timeline 播放头（秒）——runtime 写入真实时间轴位置的入口 |
| `playing` | 播放态（进 SyncState.playing） |

**播放态发布面（2026-11-17 T3 落地）**：编辑器宿主（`TimelineAudioBusHost`）在线时（`Registry.playbackActive=true`），`publishSync` 的播放头/播放态以 Host 发布面为准——命令面（play/pause/seek/stop）即时写、worker 每播放拍连续写会话秒 + 写入时刻全局帧锚点。节点本地 `playing`/`positionSec`/`followClock` 属性仅在无宿主（节点单独联调）时兜底生效。此修复解决：SyncData 接 LTCGenerator 不出时间码（playing 恒 false → 下游 hold 定格静音）。

## 驱动权约定（单写者铁律）

- `driveMode=external`：节点不写队列；runtime（后续接入）是唯一写者。
- `driveMode=test/silence`：节点是写者（联调用）；runtime 接入时应跳过非 external 的 bus。
- 读侧（效果器/AudioDeviceOut）与既有音频链完全同构，零改动。

## 下游

`BUS n` → 效果器（Gate/Delay/CrossFader…）→ `AudioDeviceOutNode`（唯一设备拥有者）。

## 待办（对应实施方案 T1/T4）

- runtime 侧混音写入（`ms::AudioEngine::mixFrames` → `TimelineAudioBusRegistry::acquire(busName)`）；
- seek 时 flush（队列 `clear()` + 重锚）由 runtime 侧触发；
- 电平/水位观测接 StatusContainer。
