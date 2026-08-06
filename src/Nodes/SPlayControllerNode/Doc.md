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

## 2. 端口

默认 **1 入 / 1 出**。`PortEditable`：用户在节点上**手动**增删端口；列表轮询与脚本**不会**改端口数。

| 方向 | 命名 | 行为 |
|------|------|------|
| 输入 | `[0] name` … | 每次写入：`true`/`1` → PLAY；`false`/`0` → STOP（可重复触发） |
| 输出 | `[0] name` … | `true` = 设备该路正在播放（`status==1`） |

- 列表勾选与设备 `status` 同步（轮询校正）；也可手动点选控制。
- 端口 `i` 映射列表排序号 `i`；caption 有名称时显示 `[i] name`，否则 `[i] —`。
- 需要控多路时，手动加端口到所需数量即可。

## 3. 界面

| 控件 | 说明 |
|------|------|
| 主机 | 默认 `192.168.1.13`（HTTP :80，无独立端口配置） |
| 全部播放 | `PLAY_ALL`，并将本地期望全部置 true |
| 全停 | `STOP_ALL`，并将本地期望全部置 false |
| 连接 | 已连接（绿）/ 未连接（红） |
| 状态 | 操作摘要 |
| 播放列表 | 勾选跟设备 Playing 状态；也可点选播/停 |

## 4. 属性与 OSC

| 属性 / 触发 | OSC | 说明 |
|-------------|-----|------|
| `hostAddress` | `/host` | 主机 IP |
| `connected` | `/connected` | 只读连接状态 |
| （触发） | `/playAll` | `true` 时全部播放 |
| （触发） | `/stopAll` | `true` 时全停 |

各路 playlist 仅通过节点输入端口控制，不提供 `/0` `/1` … 外部地址。

## 5. 使用提示

1. 填写 S-Play IP（实物默认 `:80`，只填 IP）；模拟器填 `127.0.0.1:8080`。
2. 需要多路时，在节点上手动增加 In/Out 端口（数量自定）。
3. 向某路输入接 `true` 开播、接 `false` 停止；或用列表勾选。
4. 用对应输出接指示灯 / 后续逻辑。

## 6. 无设备联调（Python 模拟器）

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

## 7. 与 VLC Remote 的差异

| 维度 | VLC Remote | SPlay Controller |
|------|------------|------------------|
| 播放模型 | 单路当前项 | **多路并发** |
| 端口 | PLAY / INDEX / STOP | **每路一入一出（手动编辑数量）** |
| 停止 | 全局 pl_stop | **按 playlist_id** |
| 列表映射 | Index → id | Index → id（隐藏） |
| 保活 | 5s | 1s |
