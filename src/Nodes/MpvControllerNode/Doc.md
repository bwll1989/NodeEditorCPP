# Mpv Controller

## 1. 节点说明

通过本机 **mpv media-api**（`portable_config/scripts/media-api.lua`）控制播放。固定轮询保活、按排序号切换、双击播放；播放、停止与循环均可走端口。

| 能力 | API | 说明 |
|------|-----|------|
| 查询 / 保活 | `GET /api/v1/library` | 节点就绪后立即查询，之后固定每 **5 秒** |
| 切换播放 | `POST /api/v1/playback/play` | Body：`{"id":"<mediaId>"}`；由排序号（从 0 起）取媒体 id |
| 停止 | `POST /api/v1/playback/stop` | 停止当前播放；不修改 Index |
| 循环 | `POST /api/v1/settings` | Body：`{"loop":"file"}` / `{"loop":"off"}` |

- **默认 Base URL：** `http://127.0.0.1:8080`
- **API 前缀：** `/api/v1`
- **响应壳：** `{ "ok": true/false, "data": ..., "error": ... }`（节点按 `ok` 判断成败）

完整协议见同目录 [media-api.md](media-api.md)。

## 2. 端口

**输入（4）：** PLAY / INDEX / STOP / LOOP  
**输出（2）：** DONE / CONNECTED

| 输入 | 行为 |
|------|------|
| PLAY | 仅为 **true** / `1` / `"true"` 时触发；按 **Index 控件** 当前值播放 |
| INDEX | 整数（从 0 起）→ 同步 Index 控件并播放 |
| STOP | 仅为 **true** / `1` / `"true"` 时触发；停止当前播放，不修改 Index |
| LOOP | 布尔值；`true` → `loop=file`，`false` → `loop=off`；同步「循环」勾选 |

| 输出 | 含义 |
|------|------|
| DONE | 最近一次播放 **或停止** 请求是否成功（`true` / `false`） |
| CONNECTED | 最近一次媒体库查询是否成功（与属性 `connected` 同步） |

### 其它控制入口

| 入口 | 行为 |
|------|------|
| 「播放」按钮 | 按 Index 控件当前值播放 |
| 「停止」按钮 | 停止当前播放，**不修改** Index |
| 「循环」勾选 | 开启/关闭单文件循环 |
| 双击列表项 | 按该项排序号播放，**不修改** Index |
| OSC `/play` | `true` 时按当前 Index 播放 |
| OSC `/stop` | `true` 时停止播放 |
| OSC `/loop` | 布尔值，同 LOOP 端口 |

播放路径：`index → mediaId → POST /playback/play`。  
停止路径：`POST /playback/stop`。  
循环路径：`POST /settings`（`file` / `off`）。

## 3. 界面

| 控件 | 说明 |
|------|------|
| Base URL | media-api 根地址，如 `http://127.0.0.1:8080` |
| Index | 排序号，默认 **0**；与 INDEX 端口 / 属性 `index` 同步 |
| 播放 | 按当前 Index 播放 |
| 停止 | 停止当前播放 |
| 循环 | 单文件循环；与 LOOP 端口 / 属性 `loop` 同步 |
| 连接 | 已连接（绿）/ 未连接（红） |
| 状态 | 最近一次操作摘要 |
| 媒体列表 | `[排序号] 名称`；双击播放；当前播放项高亮（停止后清除） |

无轮询间隔调节。列表序号为连续排序号；内部用媒体库 `id`（如 `demo.mp4`）发播放命令。

## 4. 属性与 OSC

对外属性（可走属性树 / OSC，由属性系统发布）：

| 属性 | OSC 相对地址 | 说明 |
|------|----------------|------|
| `baseUrl` | `/baseUrl` | 服务根地址 |
| `index` | `/index` | 排序号；写入会触发播放 |
| `loop` | `/loop` | 循环开关；写入会 `POST /settings` |
| `connected` | `/connected` | 只读连接状态 |
| （触发） | `/play` | `true` 时按当前 Index 播放 |
| （触发） | `/stop` | `true` 时停止当前播放 |

当前播放媒体 id 仅内部用于列表高亮，不进属性系统。

## 5. 使用提示

1. 启动带 media-api 的 mpv（监听 `127.0.0.1:8080`，建议 `idle=yes`）。  
2. 填写 Base URL，等待「连接: 已连接」与媒体列表加载。  
3. 用 Index / 「播放」/ PLAY / INDEX 播放；双击列表可临时切曲且不改 Index。  
4. 需要收场/黑屏时用 STOP 端口、「停止」按钮或 OSC `/stop`。  
5. 需要单文件循环时勾选「循环」，或用 LOOP 端口 / OSC `/loop`。  
6. 从 DONE / CONNECTED 接后续逻辑节点（播放与停止都会更新 DONE）。

## 6. 与 SlideShow / VLC Remote 的对应

| 维度 | Mpv Controller | SlideShow | VLC Remote |
|------|----------------|-----------|------------|
| 对象 | mpv + media-api | 数字标牌 | VLC 播放器 |
| 保活 | 5s `GET /library` | 5s `content/get` | 5s `playlist.json` |
| 切换 | `POST /playback/play` + id | `playlist/set` + name | `pl_play` + playlistId |
| 停止 | STOP 端口 / 按钮 / OSC | — | STOP 端口 / 按钮 / OSC |
| 循环 | LOOP 端口 / 勾选 / OSC | — | — |
| INDEX | 排序号 → 媒体 id | 排序号 → 名称 | 排序号 → playlistId |
| 端口 | PLAY / INDEX / STOP / LOOP → DONE / CONNECTED | TRIGGER / INDEX → DONE / CONNECTED | PLAY / INDEX / STOP → DONE / CONNECTED |
| 双击 | 播放且不改 Index | 相同 | 相同 |
