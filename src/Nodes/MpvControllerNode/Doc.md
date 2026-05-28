# MpvControllerNode

## 1. 节点说明

通过 HTTP 控制运行在本机或局域网内的 **MPV 播放服务**（默认端口 8080）：播放/暂停、全屏、音量、播放列表、倍速等。输入端口可发送任意 API 路径；输出端口返回播放器状态 JSON。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| 输入 0 | VariableData | 写入 API 路径字符串（如 `toggle_pause`），将 POST 到 `http://<主机>:8080/api/<路径>` |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| 输出 0 | VariableData | 播放器状态（JSON，由 GET `/api/status` 更新） |

## 3. 界面说明

- **主机地址**：MPV HTTP 服务 IP（默认 `127.0.0.1`）。
- **音量**：滑块调节，会调用 `set_volume` API。
- **播放、全屏、上一首、下一首、加速、减速、复位速度**：对应各 API 按钮。

外部控制：`/host`、`/volume`、`/play`、`/fullscreen`、`/playlist_prev`、`/playlist_next`、`/speed_add`、`/speed_sub`、`/speed_reset`。

## 4. 使用说明

1. 确保目标机器已启动带 HTTP API 的 MPV 服务（8080）。
2. 填写主机地址；用按钮或输入端口发送命令。
3. 操作后约 0.9 秒会自动拉取一次状态；也可从输出端口读取。
4. 输入端口适合接 Inject、逻辑节点，传入 API 子路径字符串。

## 5. 示例

- 界面点「播放」控制暂停/继续。
- Inject 向输入 0 发送字符串 `playlist_next` 切歌。
- 输出 0 接显示或条件节点，根据 JSON 中的播放状态分支。
