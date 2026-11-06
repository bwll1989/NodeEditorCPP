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

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 3 | 否 | 3 | 3 | — | 末端口为 STOP（停止播放） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



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

## 3. 界面与属性

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

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/host` | 读写 | 主机；写入后立即重新查询播放列表。 |
| `/port` | 读写 | 端口（钳制 1–65535）；写入后立即重新查询播放列表。 |
| `/index` | 读写 | 排序号；**写入即触发播放**（同步 Index 控件并 `pl_play`），负数忽略。 |
| `/connected` | 只读 | 最近一次列表查询结果；变化时自动推送。外部写入无效（属性无 WRITE，也未订阅）。 |
| `/play` | 写 | `true` 按当前 Index 播放；发送前推送 `/play=true`、请求结束推 `false`。 |
| `/stop` | 写 | `true` 停止播放；同样有 true→false 反馈序列。 |

`/host`、`/port`、`/index`、`/connected` 注册了带属性 member 的绑定：模型就绪时推送初始值、属性变化自动反馈。`/play`、`/stop` 的 member 不是真实属性，仅由节点在请求前后手动推送反馈。

**不进属性系统：** 密码（仅界面 + 本地存盘）。

## 5. 行为细节

- 节点就绪后立即查询一次播放列表，之后固定每 5 秒保活轮询；上一查询未返回时跳过本轮。列表查询与命令请求使用**两个独立 HTTP 客户端**，互不阻塞；查询成功即视为已连接并重建列表，失败（含主机为空等 URL 无效情况）即断开。
- 鉴权为 HTTP Basic：默认凭证格式 `:password`（空用户名 + Lua HTTP 密码），收到 401 自动改用 `password:`（密码作用户名）重试一次；密码为空时不附加认证头。
- 列表解析递归展开 `children` 树（`node` 容器跳过、只收叶子项），显示为从 0 起的连续排序号，内部保存 VLC 的 playlistId；名称缺省时依次回退 URI、`Item {id}`。部分 VLC 版本返回扁平数组，客户端包装为 `_playlistItems` 后统一解析。
- 列表刷新后 Index 控件上限设为项数-1，当前 Index 超上限时自动归 0；序号无对应列表项时不发请求，直接 DONE=false。
- 播放与停止互斥排队：同一时刻仅一个在途请求，期间的新意图记为待发标志（后到优先，设播放清停止标志、反之亦然），当前请求结束后自动执行最新意图；播放、停止同时在队时**停止优先**。
- 成功判定：VLC 命令响应无业务层 success 字段，HTTP 成功且响应为合法 JSON 即 DONE=true；播放成功后 300ms 额外刷新一次列表以更新"正在播放"标记。
- 当前项高亮来自两处：列表 JSON 中 `current=="current"` 的项，或 status.json 回包的 `currentplid`（`-1` 视为未播放）；停止成功后清空高亮。
- INDEX 端口取值支持 `{index: n}` 键或 default 数值（布尔忽略，数字类型直收，字符串需与解析值一致），非法值静默忽略；PLAY / STOP 仅真值（`true`/`1`/`"true"`）触发。
- 端口连接策略：输入、输出均为 Many；控件面板在右侧属性面板（`WidgetEmbeddable=false`），节点高度可调。主机 / 端口 / 密码以控件为数据源（每次请求前从控件回读）；Index 控件手动调整只更新属性、**不触发播放**。
- 持久化：`values` 子对象 4 键（hostAddress / port / password / index），密码随工程存盘但不进属性系统；加载只恢复参数与控件显示，**不触发播放**、不发请求。

## 6. 使用说明与典型接法

1. 启动带 HTTP 接口的 VLC，填写主机 / 端口 / 密码。  
2. 等待「连接: 已连接」与播放列表加载。  
3. 用 Index / 「播放」/ PLAY / INDEX 播放；双击列表可临时切曲且不改 Index。  
4. 需要收场时用 STOP 端口、「停止」按钮或 OSC `/stop`。  
5. 从 DONE / CONNECTED 接后续逻辑（播放与停止都会更新 DONE）。

## 7. 与 Mpv / SlideShow 的对应

| 维度 | VLC Remote | Mpv Controller | SlideShow |
|------|------------|----------------|-----------|
| 保活 | 5s `playlist.json` | 5s `GET /library` | 5s `content/get` |
| 切换 | `pl_play&id=` | `POST /playback/play` | `playlist/set` |
| 停止 | STOP / 按钮 / OSC | 相同 | — |
| INDEX | 排序号 → playlistId | 排序号 → 媒体 id | 排序号 → 名称 |
| 端口 | PLAY / INDEX / STOP → DONE / CONNECTED | 相同 | TRIGGER / INDEX → DONE / CONNECTED |
| 双击 | 播放且不改 Index | 相同 | 相同 |
| 凭据 | 密码不进属性 | — | Username/Password 不进属性 |

