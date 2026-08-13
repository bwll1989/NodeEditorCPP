# NDV Player

## 1. 节点说明

按 **Player ID** 通过全局 **NDVController**（TCP `0.0.0.0:9008`）控制对应 NDV 客户端。界面显示连接状态与设备 IP；仅输出是否已停止（不解析播放进度）。

## 2. 端口

**输入（4）：** INDEX / PLAY / STOP / LOOP  
**输出（1）：** STOPPED

| 端口 | 行为 |
|------|------|
| INDEX | 文件索引 |
| PLAY | `true` 播放，`false` 停止 |
| STOP | `true` 停止 |
| LOOP | `true` 循环播放当前索引 |
| STOPPED | `true`：在线且未在播放；`false`：正在播放（发 play/loop 后乐观置位，或收到停播回包 `20`） |

## 3. 界面

- Player ID / File Index
- 连接状态（Button，可拖拽绑定）
- Host / IP（Button，可拖拽绑定）
- Play / Loop Play / Stop

外部控制：`/index`、`/playerID`、`/play`、`/stop`、`/loop`。  
只读反馈：`connected` / `/connected`，`host` / `/host`。

## 4. 使用说明

1. 拖入 NDV Player，设置 Player ID（与 NDV 握手 ID 一致）。
2. NDV 客户端连接本机 9008，握手成功后显示在线与 IP。
3. 设备 ID 绑定在对应 TCP 连接上。
4. 播放/停止状态来源：
   - 本软件发 play/loop/stop 时乐观更新
   - 设备回包 `0E`：视为在播（不解析进度数值，状态不变则不刷 UI）
   - 设备回包 `20`：视为停播
   - 在播后约 0.8s 无 `0E`：视为停播
5. 多个 Player 共享同一个 NDVController。
