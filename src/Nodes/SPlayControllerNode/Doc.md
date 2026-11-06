# SPlay Controller

## 1. 节点说明

通过 **ENTTEC S-Play HTTP API**（`POST /api`）控制多条 playlist **并发**播放与停止。

每一路输入/输出对应列表排序号；**端口数量与 playlist 数量无关**，避免脚本或列表刷新改端口、拆连线。

| 能力 | API | 说明 |
|------|-----|------|
| 查询 / 保活 | `command: 8` `GET_ALL_PLAYLISTS` | 就绪后立即查询，之后固定每 **1 秒** |
| 播放 | `command: 0` `PLAY` + `playlist_id` | 由排序号取内部 id |
| 停止 | `command: 2` `STOP` + `playlist_id` | 只停该路 |
| 全部播放 | `command: 5` `PLAY_ALL_PLAYLISTS` | 界面「全部播放」按钮 |
| 全停 | `command: 7` `STOP_ALL_PLAYLISTS` | 界面「全停」按钮 |

固件建议 ≥ v2.2；设备 IP 建议静态。文档：[ENTTEC/SplayApi](https://github.com/ENTTEC/SplayApi)。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 是 | 1 | 不限 | VariableData | 无 |
| 输出 | 1 | 是 | 1 | 不限 | VariableData | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



默认 **1 入 / 1 出**。`PortEditable`：用户在节点上**手动**增删端口；列表轮询与脚本**不会**改端口数。

| 方向 | 命名 | 行为 |
|------|------|------|
| 输入 | `[0] name` … | 每次写入：`true`/`1` → PLAY；`false`/`0` → STOP（可重复触发） |
| 输出 | `[0] name` … | `true` = 设备该路正在播放（`status==1`） |

- 列表勾选与设备 `status` 同步（轮询校正）；也可手动点选控制。
- 端口 `i` 映射列表排序号 `i`；caption 有名称时显示 `[i] name`，否则 `[i] —`。
- 需要控多路时，手动加端口到所需数量即可。

## 3. 界面与属性

| 控件 | 说明 |
|------|------|
| 主机 | 默认 `192.168.1.13`（HTTP :80，无独立端口配置） |
| 全部播放 | `PLAY_ALL`，并将本地期望全部置 true |
| 全停 | `STOP_ALL`，并将本地期望全部置 false |
| 连接 | 已连接（绿）/ 未连接（红） |
| 状态 | 操作摘要 |
| 播放列表 | 勾选跟设备 Playing 状态；也可点选播/停 |

## 4. 外部控制（OSC 地址）

| 属性 / 触发 | OSC | 说明 |
|-------------|-----|------|
| `hostAddress` | `/host` | 主机地址（`IP` / `IP:端口` / `http(s)://…`）；写入后立即重新查询。 |
| `connected` | `/connected` | 只读连接状态；变化时自动推送。外部写入无效（属性无 WRITE）。 |
| （触发） | `/playAll` | `true` 时全部播放；节点推送 true→false 反馈序列。 |
| （触发） | `/stopAll` | `true` 时全停；同样有 true→false 反馈序列。 |

`/host`、`/connected` 注册了带属性 member 的绑定：模型就绪时推送初始值、变化自动反馈。`/playAll`、`/stopAll` 的 member 不是真实属性，仅由节点手动推送。各路 playlist 仅通过节点输入端口控制，不提供 `/0` `/1` … 外部地址。

## 5. 行为细节

- 就绪后立即查询一次 playlist，之后固定每 1 秒保活轮询；上一轮未返回时跳过。列表按 `order` 升序排序（同 order 按 playlist_id），端口 `i` 对应排序号 `i`。
- 连接判定有防抖：已连接时单次轮询失败只提示不断开，连续失败 2 次才置为未连接，避免迟到超时误判。
- 所有命令（play / stop / playAll / stopAll）经队列串行发送，同一时刻只有一个在途请求；命令成功或失败 200ms 后主动补一次轮询校正状态。
- 输入端口每次写入都发令（`true`→PLAY、`false`→STOP，可重复触发重播）；列表勾选仅状态变化时发令。发令后先乐观更新勾选，下一轮轮询用设备真实 status 校正。
- 排序号无对应 playlist（序号越界或 id<0）时仅提示、不发命令。
- 输出端口 `true` = 设备该路 `status==1`（Playing），每轮轮询全量刷新。
- host 解析支持 `IP`、`IP:端口`、`http(s)://…`（默认端口 80）；换主机时丢弃全部在途请求与待发命令队列。
- 端口数量由用户在节点上手动增删（`PortEditable=true`），与 playlist 数量无关，轮询不增删端口、不拆连线；端口连接策略输入输出均为 Many。
- 持久化：`values` 子对象仅 `hostAddress` 一键；端口数由框架恢复，加载时只对齐内部缓存。

## 6. 使用说明与典型接法

1. 填写 S-Play IP（实物默认 `:80`，只填 IP）；模拟器填 `127.0.0.1:8080`。
2. 需要多路时，在节点上手动增加 In/Out 端口（数量自定）。
3. 向某路输入接 `true` 开播、接 `false` 停止；或用列表勾选。
4. 用对应输出接指示灯 / 后续逻辑。

## 7. 无设备联调（Python 模拟器）

无实物时可在本目录启动模拟 API：

```bash
python mock_splay_server.py
# 或指定端口 / 列表数量
python mock_splay_server.py --port 8080 --count 4
```

节点「主机」填：`127.0.0.1:8080`（支持 `IP` 或 `IP:端口`）。

自测：

```bash
curl -H "Content-Type: application/json" -d "{\"command\":8}" http://127.0.0.1:8080/api
curl -H "Content-Type: application/json" -d "{\"command\":0,\"playlist_id\":1}" http://127.0.0.1:8080/api
```

模拟器覆盖：PLAY(0) / PAUSE(1) / STOP(2) / PLAY_ALL(5) / STOP_ALL(7) / GET_ALL_PLAYLISTS(8) / GET_INFO(88)。

## 8. 与 VLC Remote 的差异

| 维度 | VLC Remote | SPlay Controller |
|------|------------|------------------|
| 播放模型 | 单路当前项 | **多路并发** |
| 端口 | PLAY / INDEX / STOP | **每路一入一出（手动编辑数量）** |
| 停止 | 全局 pl_stop | **按 playlist_id** |
| 列表映射 | Index → id | Index → id（隐藏） |
| 保活 | 5s | 1s |

