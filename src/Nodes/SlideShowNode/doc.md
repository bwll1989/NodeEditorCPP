# SlideShow

## 1. 节点说明

连接 [SlideShow](https://slideshow.digital/) 数字标牌设备，通过局域网 REST API 查询播放列表并切换播放。

| 能力 | API | 说明 |
|------|-----|------|
| 查询 / 保活 | `GET /ajax/content/get` | 节点就绪后立即查询，之后固定每 **5 秒** 轮询 |
| 切换播放 | `PUT /ajax/playlist/set?playlistName=...` | 由排序号（index，从 0 起）查名称后切换；可选 `zoneName` |

鉴权：HTTP Basic Auth（界面中的 Username / Password）。

## 2. 端口

**输入（2）：** TRIGGER / INDEX  
**输出（2）：** DONE / CONNECTED

| 输入 | 行为 |
|------|------|
| TRIGGER | 仅为 **true** / `1` / `"true"` 时触发；按 **Index 控件** 当前值切换 |
| INDEX | 整数（从 0 起）→ 同步 Index 控件并切换 |

| 输出 | 含义 |
|------|------|
| DONE | 最近一次切换是否成功（`true` / `false`） |
| CONNECTED | 最近一次列表查询是否成功（与属性 `connected` 同步） |

### 其它切换入口

| 入口 | 行为 |
|------|------|
| 「播放」按钮 | 按 Index 控件当前值切换 |
| 双击列表项 | 按该项排序号切换，**不修改** Index 控件 |

共用路径：`index → playlistName → playlist/set`。

## 3. 界面

| 控件 | 说明 |
|------|------|
| Base URL | 设备地址，如 `http://192.168.x.x:8081` |
| Username / Password | 鉴权账号；**仅界面编辑 + 工程本地存盘** |
| Zone | 分区名，留空 = 主分区 |
| Index | 排序号，默认 **0**；与 INDEX 端口 / 属性 `index` 同步 |
| 播放 | 按当前 Index 切换播放列表 |
| 连接 | 已连接（绿）/ 未连接（红） |
| 状态 | 最近一次操作摘要 |
| 播放列表 | 显示 `[排序号] 名称  路径`；双击切换；当前播放项高亮 |

无轮询间隔调节、无刷新按钮。

## 4. 属性与 OSC

对外属性（可走属性树 / OSC，由属性系统发布，无需额外 `stateFeedBack`）：

| 属性 | OSC 相对地址 | 说明 |
|------|----------------|------|
| `baseUrl` | `/baseUrl` | 设备地址 |
| `zoneName` | `/zoneName` | 分区 |
| `index` | `/index` | 排序号；写入会切换播放 |
| `connected` | `/connected` | 只读连接状态 |
| （触发） | `/play` | `true` 时按当前 Index 切换 |

**不进属性系统**（避免默认向外发布）：

| 数据 | 说明 |
|------|------|
| Username / Password | 仅界面 + `save`/`load` 本地持久化 |
| 当前播放名（内部） | 仅用于列表高亮与状态文案 |

## 5. 使用提示

1. 填写 Base URL / 账号后等待「连接: 已连接」与列表加载。  
2. 用 Index 或列表双击选定内容；TRIGGER / 「播放」按 Index 控件值切换。  
3. 双击不会改动 Index，便于 TRIGGER 始终按控件中的序号触发。

## 6. 与 VLC Remote 的对应关系

交互与端口已对齐：均为 TRIGGER / INDEX → DONE / CONNECTED，5s 轮询保活，Index + 双击（不改 Index）。

| 维度 | SlideShow | VLC Remote |
|------|-----------|------------|
| 对象 | 数字标牌 | VLC 播放器 |
| 切换 API | `playlist/set`（名称） | `pl_play`（playlistId） |
| INDEX | 排序号 → 名称 | 排序号 → playlistId |

详见 `VLC_RemoteNode/Doc.md`。
