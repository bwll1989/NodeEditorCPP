# NDV Player

## 1. 节点说明

按 **Player ID** 通过全局 **NDVController**（TCP `0.0.0.0:9008`）控制对应 NDV 客户端。界面显示连接状态与设备 IP；仅输出是否已停止（不解析播放进度）。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 否 | 4 | 4 | — | 末端口为 LOOP（循环播放当前索引） |
| 输出 | 1 | 否 | 1 | 1 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



**输入（4）：** INDEX / PLAY / STOP / LOOP  
**输出（1）：** STOPPED

| 端口 | 行为 |
|------|------|
| INDEX | 文件索引 |
| PLAY | `true` 播放，`false` 停止 |
| STOP | `true` 停止 |
| LOOP | `true` 循环播放当前索引 |
| STOPPED | `true`：在线且未在播放；`false`：正在播放或离线（播放状态由 NDVController 判定） |

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 默认 / 范围 | 作用 |
|------|-------------|------|
| Player ID | 1（1–99） | 对应 NDV 握手 ID，指令只发给该 ID 的连接 |
| File Index | 0（0–100） | play / loop 指令携带的文件索引 |
| 连接状态 | 离线（红字） | 只读显示按钮，在线变绿；可拖拽绑定到其他节点控件 |
| Host / IP | — | 只读显示该 ID 的设备 IP（去 `::ffff:` 前缀）；可拖拽绑定 |
| Play / Loop Play / Stop | — | 分别发送 play / loop（带当前索引）/ stop 指令 |

公开属性：`fileIndex`、`playerId` 可写；`connected`、`host` 只读（仅有变更信号，无 WRITE）。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/index` | 读写 | 写 File Index（整数）；值变化时自动推送反馈。 |
| `/playerID` | 读写 | 写 Player ID（整数，1–99）；值变化时自动推送反馈。 |
| `/play` | 写 | 发送 play（携带当前 File Index）。绑定指向的 `play` 不是真实属性，无自动反馈。 |
| `/stop` | 写 | 发送 stop；节点发送后主动推送 `/stop = true`。 |
| `/loop` | 写 | 发送 loop（携带当前 File Index）；节点发送后主动推送 `/loop = true`。 |
| `/connected` | 只读 | 当前 Player ID 的在线状态；状态变化时自动推送。外部写入无效（属性无 WRITE、事件分支无此地址）。 |
| `/host` | 只读 | 当前 Player ID 的设备 IP；变化时自动推送。外部写入无效。 |

说明：`/play`、`/stop`、`/loop` 的绑定 member（play/stop/loop）不是节点的 Q_PROPERTY，框架不会为它们自动反馈；其中 `/stop`、`/loop` 由节点在发送指令后手动推送，`/play` 完全无反馈。`/connected`、`/host` 注册了带 member 的绑定，模型就绪时也会各推送一次初始值。

## 5. 行为细节

- 依赖全局单例 NDVController（TCP `0.0.0.0:9008`），多个 Player 节点共享同一个控制器实例。
- 状态刷新：设备状态变化回调只在 deviceId 匹配当前 Player ID 时应用；客户端列表变化时按当前 Player ID 重新拉取。切换 Player ID 会立即刷新状态显示。
- STOPPED 输出：在线且设备状态非 "Playing" 视为已停止（`true`）；离线输出 `false`（不报停止）。值变化才重新推送，不重复发 dataUpdated。
- 播放/停止状态的判定（乐观置位、回包 `0E`/`20`、0.8s 无回包超时）全部在 NDVController 内完成，本节点只消费其结果。
- 输入端口语义：INDEX 写 File Index；PLAY `true` 播放 / `false` 停止；STOP 仅 `true` 时停止；LOOP 仅 `true` 时循环。输入断开（空数据）不触发任何动作。
- 端口连接策略：输入、输出均为 Many，可多点连接。
- 指令转发：play / loop 携带当前 File Index，stop 固定携带 0；实际寻址与投递由 NDVController 按 Player ID 完成。
- 持久化：`PlayerSettings` 子对象两键 `FileIndex` / `PlayerID`；加载仅认 `PlayerSettings` 对象（不存在则跳过），键存在才应用。
- 析构时退订全局事件总线并断开与 NDVController 的全部信号连接。

## 6. 使用说明与典型接法

1. 拖入 NDV Player，设置 Player ID（与 NDV 握手 ID 一致）。
2. NDV 客户端连接本机 9008，握手成功后显示在线与 IP。
3. 设备 ID 绑定在对应 TCP 连接上。
4. 播放/停止状态来源：
   - 本软件发 play/loop/stop 时乐观更新
   - 设备回包 `0E`：视为在播（不解析进度数值，状态不变则不刷 UI）
   - 设备回包 `20`：视为停播
   - 在播后约 0.8s 无 `0E`：视为停播
5. 多个 Player 共享同一个 NDVController。

