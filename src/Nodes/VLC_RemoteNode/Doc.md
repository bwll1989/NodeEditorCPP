# VLC Controller

## 1. 节点说明

通过 **VLC HTTP Web 接口**远程控制播放器。交互与 **Mpv Controller** 对齐：固定轮询保活、按排序号切换、双击播放；播放与停止均可走端口。

| 能力 | API | 说明 |
|------|-----|------|
| 查询 / 保活 | `GET /requests/playlist.json` | 节点就绪后立即查询，之后固定每 **5 秒** |
| 切换播放 | `GET /requests/status.json?command=pl_play&id=...` | 由排序号（从 0 起）取 playlistId 后切换 |
| 停止 | `GET /requests/status.json?command=pl_stop` | 停止当前播放；不修改 Index |

VLC 启用示例：

```text
vlc --extraintf http --http-password xxx --http-port 8080
```

鉴权：HTTP Basic（密码与 VLC Lua HTTP 密码一致）。

## 2. 端口

**输入（3）：** PLAY / INDEX / STOP  
**输出（2）：** DONE / CONNECTED

| 输入 | 行为 |
|------|------|
| PLAY | 仅为 **true** / `1` / `"true"` 时触发；按 **Index 控件** 当前值播放 |
| INDEX | 整数（从 0 起）→ 同步 Index 控件并播放 |
| STOP | 仅为 **true** / `1` / `"true"` 时触发；停止当前播放，不修改 Index |

| 输出 | 含义 |
|------|------|
| DONE | 最近一次播放 **或停止** 请求是否成功（`true` / `false`） |
| CONNECTED | 最近一次列表查询是否成功（与属性 `connected` 同步） |

### 其它控制入口

| 入口 | 行为 |
|------|------|
| 「播放」按钮 | 按 Index 控件当前值播放 |
| 「停止」按钮 | 停止当前播放，**不修改** Index |
| 双击列表项 | 按该项排序号播放，**不修改** Index |
| OSC `/play` | `true` 时按当前 Index 播放 |
| OSC `/stop` | `true` 时停止播放 |

播放路径：`index → playlistId → pl_play`。  
停止路径：`pl_stop`。

## 3. 界面

| 控件 | 说明 |
|------|------|
| 主机 / 端口 | VLC HTTP 地址（默认 `127.0.0.1:8080`） |
| 密码 | **仅界面 + 本地存盘**，不进属性系统 |
| Index | 排序号，默认 **0**；与 INDEX 端口 / 属性 `index` 同步 |
| 播放 | 按当前 Index 切换 |
| 停止 | 停止当前播放 |
| 连接 | 已连接（绿）/ 未连接（红） |
| 状态 | 操作摘要 |
| 播放列表 | `[排序号] 名称`；双击切换；当前项高亮（停止后清除） |

列表显示为连续排序号；内部仍用 VLC 的 playlistId 发命令。

## 4. 属性与 OSC

| 属性 | OSC | 说明 |
|------|-----|------|
| `hostAddress` | `/host` | 主机 |
| `port` | `/port` | 端口 |
| `index` | `/index` | 排序号；写入会切换 |
| `connected` | `/connected` | 只读连接状态 |
| （触发） | `/play` | `true` 时按当前 Index 切换 |
| （触发） | `/stop` | `true` 时停止当前播放 |

**不进属性系统：** 密码。

## 5. 使用提示

1. 启动带 HTTP 接口的 VLC，填写主机 / 端口 / 密码。  
2. 等待「连接: 已连接」与播放列表加载。  
3. 用 Index / 「播放」/ PLAY / INDEX 播放；双击列表可临时切曲且不改 Index。  
4. 需要收场时用 STOP 端口、「停止」按钮或 OSC `/stop`。  
5. 从 DONE / CONNECTED 接后续逻辑（播放与停止都会更新 DONE）。

## 6. 与 Mpv / SlideShow 的对应

| 维度 | VLC Remote | Mpv Controller | SlideShow |
|------|------------|----------------|-----------|
| 保活 | 5s `playlist.json` | 5s `GET /library` | 5s `content/get` |
| 切换 | `pl_play&id=` | `POST /playback/play` | `playlist/set` |
| 停止 | STOP / 按钮 / OSC | 相同 | — |
| INDEX | 排序号 → playlistId | 排序号 → 媒体 id | 排序号 → 名称 |
| 端口 | PLAY / INDEX / STOP → DONE / CONNECTED | 相同 | TRIGGER / INDEX → DONE / CONNECTED |
| 双击 | 播放且不改 Index | 相同 | 相同 |
| 凭据 | 密码不进属性 | — | Username/Password 不进属性 |
