# FlowDisplay Controller

## 1. 节点说明
通过 **FlowDisplay**（display 独立程序，v1.8.01 起）的 **standalone HTTP 控制面**控制时间轴播放。固定轮询保活、按排序号切换、双击播放；播放、停止与循环均可走端口。

| 能力 | API | 说明 |
|------|-----|------|
| 查询 / 保活 | `GET /api/v1/timelines` | 节点就绪后立即查询，已连接每 **5 秒**保活；**未连接时每 1 秒快速重试**；单请求 3 秒传输超时 |
| 切换播放 | `POST /api/v1/timelines/{id}/play` | 由排序号（从 0 起）取时间轴 id，id 经 URL 百分号编码 |
| 停止 | `POST /api/v1/playback/stop` | 回待机，播放头归零；不修改 Index |
| 循环 | `GET /api/v1/timelines/{id}` → `PUT /api/v1/timelines/{id}` | 读完整文档 → 改 `loop` 字段 → 写回；**乐观锁 rev**（`409 CONFLICT` 自动重读重写，≤2 次）；作用于**当前 Index 指向的时间轴** |

- **默认 Base URL：** `http://127.0.0.1:8995`（display config.ini `[standalone] http_port` 默认值）
- **API 前缀：** `/api/v1`
- **响应壳：** `{ "ok": true/false, "data": ..., "error": ... }`（节点按 `ok` 判断成败）
- **模式门禁：** 时间轴/播放接口仅在 display 处于 **standalone（独立模式）** 可用；sync（Editor 受控）模式返回 `409 MODE_FORBIDDEN`，节点在状态栏显示错误消息

完整协议见 [`docs/display-standalone-api.md`](../../../docs/display-standalone-api.md)。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 4 | — | 末端口为 LOOP（时间轴循环） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

**输入（4）：** PLAY / INDEX / STOP / LOOP
**输出（2）：** DONE / CONNECTED

| 输入 | 行为 |
|------|------|
| PLAY | 仅为 **true** / `1` / `"true"` 时触发；按 **Index 控件** 当前值播放 |
| INDEX | 整数（从 0 起）→ 同步 Index 控件并播放 |
| STOP | 仅为 **true** / `1` / `"true"` 时触发；停止播放回待机，不修改 Index |
| LOOP | 布尔值；`true` → 当前 Index 时间轴开循环，`false` → 关；同步「循环」勾选 |
| 输出 | 含义 |
|------|------|
| DONE | 最近一次播放 **或停止** 请求是否成功（`true` / `false`；循环写文档不更新 DONE） |
| CONNECTED | 最近一次时间轴列表查询是否成功（与属性 `connected` 同步） |

### 其它控制入口

| 入口 | 行为 |
|------|------|
| 「播放」按钮 | 按 Index 控件当前值播放 |
| 「停止」按钮 | 停止播放回待机，**不修改** Index |
| 「循环」勾选 | 对当前 Index 时间轴开/关循环（写回文档） |
| 双击列表项 | 按该项排序号播放，**不修改** Index |
| OSC `/play` | `true` 时按当前 Index 播放 |
| OSC `/stop` | `true` 时停止播放 |
| OSC `/loop` | 布尔值，同 LOOP 端口 |

播放路径：`index → timelineId → POST /timelines/{id}/play`。
停止路径：`POST /playback/stop`。
循环路径：`GET /timelines/{id}` → 改 `loop` → `PUT /timelines/{id}`（乐观锁）。

## 3. 界面与属性

| 控件 | 说明 |
|------|------|
| Base URL | display 控制面根地址，如 `http://127.0.0.1:8995` |
| Index | 排序号，默认 **0**；与 INDEX 端口 / 属性 `index` 同步；切换后「循环」勾选回显该时间轴的 loop |
| 播放 | 按当前 Index 播放 |
| 停止 | 停止播放回待机 |
| 循环 | 当前 Index 时间轴循环开关；与 LOOP 端口 / 属性 `loop` 同步 |
| 连接 | 已连接（绿）/ 未连接（红） |
| 状态 | 最近一次操作摘要 |
| 时间轴列表 | `[排序号] 名称`；`note` 非空（装配降级/素材未就位）附加括号提示；活跃项追加 ▶ 并高亮（停止后清除）；悬浮显示 index / id / 时长 / loop / note |

无轮询间隔调节。列表序号为连续排序号；内部用时间轴文档 `id`（即文件名）发播放命令。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/baseUrl` | 读写 | display 控制面根地址；写入后立即重新查询时间轴列表。 |
| `/index` | 读写 | 排序号；**写入即触发播放**（同步 Index 控件并 POST play）。 |
| `/loop` | 读写 | 循环开关；值变化才启动 GET→PUT 循环链（写当前 Index 时间轴文档）。 |
| `/connected` | 只读 | 最近一次时间轴列表查询结果；变化时自动推送。外部写入无效（属性无 WRITE）。 |
| `/play` | 写 | `true` 按当前 Index 播放；发送前推送 `/play=true`、请求结束推 `false`。 |
| `/stop` | 写 | `true` 停止播放；同样有 true→false 反馈序列。 |

`/baseUrl`、`/index`、`/loop`、`/connected` 注册了带属性 member 的绑定：模型就绪时推送初始值、属性变化自动反馈。`/play`、`/stop` 的 member 不是真实属性，仅由节点在请求前后手动推送反馈。

## 5. 行为细节

- 节点就绪后立即查询一次时间轴列表；已连接后每 5 秒保活轮询，**未连接时降为每 1 秒快速重试**。所有请求统一 **3 秒传输超时**（含 TCP 建连）——目标不可达时 SYN 重试可卡 20s+，超时兜底保证失败立即回落、连接状态不被卡死请求拖慢。上一查询未返回时跳过本轮。查询成功即视为已连接并重建列表，失败则断开状态。
- 列表序号是响应顺序的连续排序号（从 0 起），内部映射到时间轴 `id` 发播放命令；Index 超出列表上限时自动归 0。
- 双击列表项按该项播放但**不改** Index 控件；播放按钮 / PLAY / `/index` 按 Index 播放并同步控件。
- 循环链：`GET /timelines/{id}` 取完整文档（含 `rev`）→ 仅替换 `loop` 字段 → `PUT /timelines/{id}` 整体写回。返回 `409 CONFLICT`（文档被其他窗口修改）时自动重读重写，最多 2 次；`.show`/ini 装配的**只读条目**（GET 返回 `readonly:true`）拒绝设置并提示。循环链与播放/停止相互独立（各自排队），同一时刻只有一条循环链在途，新意图记待发标志、完成后执行最新值。
- 循环勾选回显：轮询列表后按当前 Index 条目的 `loop` 回写勾选（循环链在途时跳过，防抖动）；切换 Index 后立即从列表回写。
- 响应按「HTTP 2xx 且壳内 `ok=true`」判成功；失败时状态栏显示壳内 `error.message`（如 sync 模式的 `MODE_FORBIDDEN`、无法装配的 `ASSEMBLE_FAILED`）。
- 活跃高亮：优先取列表响应的 `is_active`；播放成功后先用响应 `data.id` 即时高亮，下一轮轮询校正；停止成功后清除。
- 时间轴 id 经 `QUrl::toPercentEncoding` 百分号编码后拼 URL（id 允许中文）。
- INDEX 端口取值支持 `{index: n}` 键或 default 数值（布尔忽略，数字字符串需与解析值一致）；PLAY / STOP 仅真值（`true`/`1`/`"true"`）触发。
- 端口连接策略：输入、输出均为 Many；控件面板在右侧属性面板（`WidgetEmbeddable=false`），节点高度可调。
- 持久化：`values` 子对象 3 键（baseUrl / index / loop）；加载只恢复参数与控件显示，**不触发播放**、不推服务器（勾选框随后按轮询到的服务器 loop 校正）。

## 6. 使用说明与典型接法

1. 启动 FlowDisplay 并确认处于 **独立模式**（HTTP 控制面默认 `8995`；屏幕待机卡上即显示控制面地址）。
2. 填写 Base URL，等待「连接: 已连接」与时间轴列表加载。
3. 用 Index / 「播放」/ PLAY / INDEX 播放时间轴；双击列表可临时切换且不改 Index。
4. 需要收场/回待机时用 STOP 端口、「停止」按钮或 OSC `/stop`。
5. 需要循环时先选中 Index，再勾选「循环」或 LOOP 端口给 `true`（写回该时间轴文档，网页编排台可见同步）。
6. 从 DONE / CONNECTED 接后续逻辑节点（播放与停止都会更新 DONE）。

## 7. 与 Mpv Controller / VLC Remote 的对应

| 维度 | FlowDisplay Controller | Mpv Controller | VLC Remote |
|------|------------------------|----------------|------------|
| 对象 | FlowDisplay standalone | mpv + media-api | VLC 播放器 |
| 保活 | 5s `GET /timelines` | 5s `GET /library` | 5s `playlist.json` |
| 切换 | `POST /timelines/{id}/play` + id | `POST /playback/play` + id | `pl_play` + playlistId |
| 停止 | STOP 端口 / 按钮 / OSC | STOP 端口 / 按钮 / OSC | STOP 端口 / 按钮 / OSC |
| 循环 | LOOP 端口 / 勾选 / OSC（写回时间轴文档，乐观锁） | LOOP 端口 / 勾选 / OSC（POST settings） | — |
| INDEX | 排序号 → 时间轴 id | 排序号 → 媒体 id | 排序号 → playlistId |
| 端口 | PLAY / INDEX / STOP / LOOP → DONE / CONNECTED | 同左 | PLAY / INDEX / STOP → DONE / CONNECTED |
| 双击 | 播放且不改 Index | 相同 | 相同 |
| 活跃高亮 | 列表 `is_active`（服务器真相） | play 响应的 media id | — |
