# NDVServerNode 使用说明

## 用途
NDVServerNode 用于监听 NDV 设备的连接/握手，并把 NDVPlayerNode 输出的 COMMAND 指令转发给指定设备（按设备 ID 或按 IP）。

## 端口
- 输入（可编辑）
  - PLAYER 0..N：VariableData（指令数据，通常连接 NDVPlayerNode 的 COMMAND）
- 输出（1）
  - CLIENT_STATUS：VariableData（客户端列表与状态）

COMMAND 字段（常用）：
- `type`：`play` / `stop` / `loop` / `next` / `prev` / `close` / `handshake`
- `fileIndex`：文件索引（play/loop 时有效）
- `targetId`：目标设备 ID；为 0 时表示广播到所有已连接设备
- `targetIP`：可选；如果设置了该字段，会优先按 IP 发送

CLIENT_STATUS 字段：
- `ConnectedClients`：map，key 为设备 ID（string），value 包含：
  - `ip` / `id` / `status` / `lastSeen` / `lastHandshake`
- `ClientCount`：已连接设备数量（int）
- `HandshakeActive`：是否启用自动握手（bool）
- `LastHandshakeTime`：最近一次自动握手时间（string）

## 节点参数
- ip：监听 IP（默认 `0.0.0.0`）
- port：监听端口（默认 `9008`）

## 外部控制（可选）
- `/ip`：设置监听 IP（string）
- `/port`：设置监听端口（int）

## 使用步骤
1. 设置 ip/port 并启动（节点创建后会自动监听）。
2. 等待 NDV 设备上线并握手，CLIENT_STATUS 会出现设备列表。
3. 将一个或多个 NDVPlayerNode 的 COMMAND 输出连接到 PLAYER 输入端口。
4. 在 NDVPlayerNode 里设置 playerId（目标设备 ID）并触发播放/停止等操作。

## 注意事项
- 设备列表来自设备主动握手；未握手的设备不会出现在 ConnectedClients 中。
- 自动握手默认每 5 秒向所有已连接设备发送一次。
