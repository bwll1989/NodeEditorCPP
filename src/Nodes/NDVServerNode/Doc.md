# NDVServerNode

## 1. 节点说明

**NDV 服务端**：监听 TCP，接收 **NDV Player** 发来的控制指令并转发给已注册的 NDV 播放客户端；维护设备 ID 与 IP 映射，并定时向所有客户端发送握手。输出当前在线客户端列表。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| PLAYER 0 … | VariableData | 来自 NDV Player 的指令包（`type`、`fileIndex`、`targetId` 或 `targetIP` 等）。端口可增删 |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| CLIENT_STATUS | VariableData | `ConnectedClients`、`ClientCount`、`HandshakeActive` 等 |

## 3. 界面说明

本节点无可嵌入界面。绑定 IP、端口通过属性或外部控制设置（默认 `0.0.0.0:9008`）。

外部控制：`/ip`、`/port`。

## 4. 使用说明

1. 配置监听 IP 与端口，与 NDV 客户端网段互通。
2. 将 **NDV Player** 的 COMMAND 输出接到 PLAYER 输入。
3. 客户端握手后出现在 CLIENT_STATUS；`targetId` 为 0 时广播到所有客户端。
4. 支持指令类型：`play`、`stop`、`loop`、`close`、`next`、`prev`、`handshake`。

## 5. 示例

`NDV Player`（COMMAND）→ `NDV Server`（PLAYER 0）→ 多台 NDV 客户端；CLIENT_STATUS 接监控面板显示在线数量。
