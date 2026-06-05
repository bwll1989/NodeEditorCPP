# VLC Remote 节点

## 1. 节点说明

通过 **VLC HTTP Web 接口**远程控制本机或局域网内的 VLC 播放器，支持播放/暂停、停止、音量、切换播放列表、全屏。输出端口以 JSON 形式提供播放状态。需先在 VLC 中启用 Web 界面并设置密码。

## 2. 端口说明

### 输入

| 端口 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 0 | PLAY/STOP | VariableData | 传入 `true` / `1` 播放，传入 `false` / `0` 停止 |
| 1 | STOP | VariableData | 传入 `true` / `1` 时停止播放 |
| 2 | VOLUME | VariableData | 设置音量；传入 0～100 的数值（百分比，内部换算为 VLC 绝对值 256=100%） |
| 3 | INDEX | VariableData | 按 playlistID 切换文件；传入列表项 ID（整数） |
| 4 | FULLSCREEN | VariableData | 传入 `true` / `1` 时切换全屏 |

### 输出

| 端口 | 名称 | 类型 | 说明 |
|------|------|------|------|
| 0 | STATUS | VariableData | 播放状态 JSON（见下表） |

**STATUS 输出字段：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `connected` | bool | 是否已连接 VLC |
| `playing` | bool | 是否正在播放 |
| `paused` | bool | 是否暂停 |
| `stopped` | bool | 是否停止 |
| `state` | string | VLC 原始状态（playing / paused / stopped） |
| `volume` | int | VLC 原始音量值（256 = 100%） |
| `volumePercent` | int | 音量百分比（0～100，基于 256 = 100% 换算） |
| `currentFile` | string | 当前播放文件名 |
| `time` | number | 当前进度（秒） |
| `length` | number | 总时长（秒） |
| `position` | number | 进度比例（0～1） |
| `progressPercent` | int | 进度百分比（0～100） |
| `currentPlId` | string | 当前播放列表项 ID |
| `fullscreen` | bool | 是否全屏 |
| `rate` | number | 播放倍速 |

## 3. 界面说明

- **主机 / 端口 / 密码**：VLC HTTP 服务地址（默认端口 8080）。密码须与 VLC 中 Lua HTTP 密码一致。
- **播放/暂停、停止、全屏**：常用播放控制。
- **音量**：`IntDragValueWidget` 滑块，范围 0～100%，拖动或输入后同步到 VLC；状态刷新时自动回显当前音量。
- **播放列表**：显示当前列表；单击或双击条目切换播放；当前项高亮，格式为 `[playlistID] 文件名`。
- **状态栏**：简要显示连接与播放信息。

外部控制：`/host`、`/port`、`/password`、`/play`、`/stop`、`/volume`、`/index`、`/fullscreen`、`/refresh_playlist`。

## 4. 使用说明

1. 启动 VLC 并开启 HTTP 接口（`--extraintf http --http-password xxx --http-port 8080`）。
2. 在节点中填写主机、端口、密码。
3. 用界面按钮控制播放，或从输入端口远程控制。
4. 从 STATUS 端口读取 JSON 状态；操作后约 0.9 秒自动刷新。

## 5. 示例

- Inject 向 PLAY/STOP 发送 `true` 开始播放，发送 `false` 停止播放。
- Inject 向 STOP 发送 `true` 停止播放。
- Inject 向 VOLUME 发送 `80` 将音量设为 80%。
- Inject 向 INDEX 发送 `3` 切换到 playlistID 为 3 的列表项。
- Inject 向 FULLSCREEN 发送 `true` 切换全屏。
- STATUS 接 Data Info 或 Extract，读取 `playing`、`currentFile`、`progressPercent` 等字段做条件分支。
