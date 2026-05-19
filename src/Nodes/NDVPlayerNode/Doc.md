# NDVPlayerNode 使用说明

## 用途
NDVPlayerNode 用于生成 NDV 播放控制指令（play/stop/loop/next/prev），并从 COMMAND 端口输出给 NDVServerNode 发送到设备。

## 端口
- 输入（4）
  - INDEX（端口 0）：VariableData（int），节目/文件索引
  - PLAY（端口 1）：VariableData（bool），true 播放；false 会触发 stop
  - STOP（端口 2）：VariableData（bool），true 触发 stop
  - LOOP（端口 3）：VariableData（bool），true 触发 loop
- 输出（2）
  - COMMAND（端口 0）：VariableData（指令数据）
  - STATUS（端口 1）：VariableData（播放器状态）

COMMAND 字段：
- `type`：`play` / `stop` / `loop` / `next` / `prev`
- `fileIndex`：文件索引（play/loop 时有效）
- `targetId`：目标设备 ID（来自 playerId）
- `playerId`：同 targetId
- `timestamp`：时间戳字符串

STATUS 字段：
- `playerId`：当前 playerId
- `currentFile`：当前 fileIndex
- `state`：`Stopped` / `Playing` / `Loop Playing`
- `lastUpdate`：更新时间

## 节点参数
- playerId：目标设备 ID（用于 NDVServerNode 选择目标设备）
- fileIndex：文件索引

## 外部控制（可选）
属性：
- `/playerID`：设置 playerId（int）
- `/index`：设置 fileIndex（int）

命令（发送任意值或 true 触发一次）：
- `/play` `/stop` `/loop` `/next` `/prev`

## 使用步骤
1. 设置 playerId（目标设备 ID）与 fileIndex。
2. 点击节点按钮或通过输入端口触发播放/停止/循环/切换。
3. 将 COMMAND 输出连接到 NDVServerNode 的 PLAYER 输入端口。
