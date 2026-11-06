# SlideShow

## 1. 节点说明

连接 [SlideShow](https://slideshow.digital/) 数字标牌设备，通过局域网 REST API 查询播放列表并切换播放。

| 能力 | API | 说明 |
|------|-----|------|
| 查询 / 保活 | `GET /ajax/content/get` | 节点就绪后立即查询，之后固定每 **5 秒** 轮询 |
| 切换播放 | `PUT /ajax/playlist/set?playlistName=...` | 由排序号（index，从 0 起）查名称后切换；可选 `zoneName` |

鉴权：HTTP Basic Auth（界面中的 Username / Password）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | —（不可新增） | 无 |
| 输出 | 2 | 否 | 2 | 2 | —（不可新增） | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

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

## 3. 界面与属性

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

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/baseUrl` | 读写 | 设备地址；写入后立即重新查询列表。 |
| `/zoneName` | 读写 | 分区名（空 = 主分区）；写入不触发查询，下次切换生效。 |
| `/index` | 读写 | 排序号；**写入即切换播放**（同步 Index 控件并 PUT playlist/set）。 |
| `/connected` | 只读 | 最近一次列表查询结果；变化时自动推送。外部写入无效（属性无 WRITE）。 |
| `/play` | 写 | `true` 按当前 Index 切换；发起时推送 `/play=true`、请求结束推 `false`。 |

`/baseUrl`、`/zoneName`、`/index`、`/connected` 注册了带属性 member 的绑定：模型就绪时推送初始值、属性变化自动反馈。`/play` 的 member 不是真实属性，仅由节点在请求前后手动推送反馈。

**不进属性系统**（避免默认向外发布）：

| 数据 | 说明 |
|------|------|
| Username / Password | 故意不声明 Q_PROPERTY，仅界面编辑 + `save`/`load` 本地持久化 |
| 当前播放名（内部） | 仅用于列表高亮与状态文案 |

## 5. 行为细节

- 就绪后立即查询一次播放列表，之后固定每 5 秒保活轮询；上一查询未返回时跳过。查询需 HTTP 2xx、返回 JSON 且（若含 `success` 字段）为 true 才算成功并置已连接；列表显示 `[排序号] 名称  路径/类型`。
- 所有请求带 HTTP Basic Auth（凭据仅本地存盘，不进属性系统）；改账号 / 密码 / Base URL 后立即重新查询。
- 切换按「排序号 → 名称 → `PUT playlist/set?playlistName=…`（可选 `zoneName`）」；Index 越界时钳回 0。
- 切换请求在途时新意图排队，完成后自动执行最新意图（后到优先）。
- 发起切换即记下名称并高亮列表（乐观）；回包 HTTP 2xx 且业务 success 才置 DONE=true，失败显示 HTTP 码或业务失败。
- 双击列表项切换但**不改** Index 控件；TRIGGER / 播放按钮 / `/index` / `/play` 按 Index 切换并同步控件。
- INDEX 端口取值支持 `{index: n}` 键或 default 数值（布尔忽略，数字字符串需与解析值一致）；TRIGGER 仅真值（`true`/`1`/`"true"`）触发。
- 端口连接策略：输入、输出均为 Many；控件面板在右侧属性面板（`WidgetEmbeddable=false`），节点高度可调。
- 持久化：`values` 子对象 6 键（baseUrl / username / password / playlistName / zoneName / index）；加载只恢复参数与高亮，不触发切换。

## 6. 使用说明与典型接法

1. 填写 Base URL / 账号后等待「连接: 已连接」与列表加载。  
2. 用 Index 或列表双击选定内容；TRIGGER / 「播放」按 Index 控件值切换。  
3. 双击不会改动 Index，便于 TRIGGER 始终按控件中的序号触发。

## 7. 与 VLC Remote 的对应关系

交互与端口已对齐：均为 TRIGGER / INDEX → DONE / CONNECTED，5s 轮询保活，Index + 双击（不改 Index）。

| 维度 | SlideShow | VLC Remote |
|------|-----------|------------|
| 对象 | 数字标牌 | VLC 播放器 |
| 切换 API | `playlist/set`（名称） | `pl_play`（playlistId） |
| INDEX | 排序号 → 名称 | 排序号 → playlistId |

详见 `VLC_RemoteNode/Doc.md`。

