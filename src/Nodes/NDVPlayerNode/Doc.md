# NDVPlayerNode

## 1. 节点说明

**NDV 播放器控制端**：选择文件索引与目标播放器 ID，生成播放/停止/循环/上一首/下一首等指令，供 **NDV Server** 转发到 NDV 设备；并输出当前播放器状态。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| INDEX | VariableData | 文件索引（整数） |
| PLAY | VariableData | `true` 播放，`false` 停止 |
| STOP | VariableData | `true` 时停止 |
| LOOP | VariableData | `true` 时循环播放当前索引 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| COMMAND | VariableData | 发往 NDV Server 的指令（`type`、`fileIndex`、`targetId` 等） |
| STATUS | VariableData | `playerId`、`currentFile`、`state`、`lastUpdate` |

## 3. 界面说明

- **文件索引 / 播放器 ID**：目标素材与设备编号。
- **播放、停止、循环、上一首、下一首**：生成对应 COMMAND。

外部控制：`/index`、`/playerID`、`/play`、`/stop`、`/loop`、`/next`、`/prev`。

## 4. 使用说明

1. 设置播放器 ID 与文件索引。
2. COMMAND 接到 **NDV Server** 的 PLAYER 输入。
3. 可用输入端口远程触发，或点界面按钮。
4. STATUS 可显示当前状态（Playing / Stopped 等）。

## 5. 示例

Inject 设置 INDEX=3 → PLAY 触发 → COMMAND → NDV Server → 指定 ID 的客户端播放第 3 号文件。
