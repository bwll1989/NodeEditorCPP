# Mpv Controller

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

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

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 4 | — | 末端口为 LOOP（单文件循环） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



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

## 3. 界面与属性

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

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/baseUrl` | 读写 | media-api 根地址；写入后立即重新查询媒体库。 |
| `/index` | 读写 | 排序号；**写入即触发播放**（同步 Index 控件并 POST play）。 |
| `/loop` | 读写 | 循环开关；值变化才 `POST /settings`（file / off）。 |
| `/connected` | 只读 | 最近一次媒体库查询结果；变化时自动推送。外部写入无效（属性无 WRITE）。 |
| `/play` | 写 | `true` 按当前 Index 播放；发送前推送 `/play=true`、请求结束推 `false`。 |
| `/stop` | 写 | `true` 停止播放；同样有 true→false 反馈序列。 |

`/baseUrl`、`/index`、`/loop`、`/connected` 注册了带属性 member 的绑定：模型就绪时推送初始值、属性变化自动反馈。`/play`、`/stop` 的 member 不是真实属性，仅由节点在请求前后手动推送反馈。

## 5. 行为细节

- 节点就绪后立即查询一次媒体库，之后固定每 5 秒保活轮询；上一查询未返回时跳过本轮。查询成功即视为已连接并重建列表，失败则断开状态。
- 列表序号是响应顺序的连续排序号（从 0 起），内部映射到媒体库 `id` 发播放命令；Index 超出列表上限时自动归 0。
- 双击列表项按该项播放但**不改** Index 控件；播放按钮 / PLAY / `/index` 按 Index 播放并同步控件。
- 播放与停止互斥排队：同一时刻只有一个在途请求，期间的新意图记为待发标志，当前请求完成后自动执行最新意图（后到优先）；循环设置同样有排队重发。
- 响应按「HTTP 2xx 且壳内 `ok=true`」判成功；失败时状态栏显示壳内 `error.message`。
- 循环状态以服务器回包为准回写勾选（回包缺省时用请求值）。
- INDEX 端口取值支持 `{index: n}` 键或 default 数值（布尔忽略，数字字符串需与解析值一致）；PLAY / STOP 仅真值（`true`/`1`/`"true"`）触发。
- 端口连接策略：输入、输出均为 Many；控件面板在右侧属性面板（`WidgetEmbeddable=false`），节点高度可调。
- 持久化：`values` 子对象 3 键（baseUrl / index / loop）；加载只恢复参数与控件显示，**不触发播放**、不推服务器。

## 6. 使用说明与典型接法

1. 启动带 media-api 的 mpv（监听 `127.0.0.1:8080`，建议 `idle=yes`）。  
2. 填写 Base URL，等待「连接: 已连接」与媒体列表加载。  
3. 用 Index / 「播放」/ PLAY / INDEX 播放；双击列表可临时切曲且不改 Index。  
4. 需要收场/黑屏时用 STOP 端口、「停止」按钮或 OSC `/stop`。  
5. 需要单文件循环时勾选「循环」，或用 LOOP 端口 / OSC `/loop`。  
6. 从 DONE / CONNECTED 接后续逻辑节点（播放与停止都会更新 DONE）。

## 7. 与 SlideShow / VLC Remote 的对应

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

