# ArtnetOutNode

## 1. 节点说明

将上游 **DMX Universe** 节点输出的 Art-Net 数据包，通过 UDP 发送到指定目标主机。支持多个 Universe 输入（端口可增删），每次收到数据会自动发送；并输出最近一次发送的成功或失败状态。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| UNIVERSE DATA | VariableData | 来自 DMXUniverseNode 的 Universe 数据包（须含 `protocol: Art-Net` 等字段）。可连接多个端口，每个端口对应一路 Universe |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| STATUS | VariableData | 发送结果：`status`（success/failed）、`universe`、`host`、`bytesWritten` 或 `error`、`timestamp` |

## 3. 界面说明

本节点无可嵌入界面。目标地址通过属性或外部控制 `/targetHost` 设置，默认 `192.168.0.255`（广播）。

## 4. 使用说明

1. 将 **DMX Universe** 节点的 UNIVERSE 输出接到本节点 UNIVERSE DATA。
2. 设置目标 IP（单播或广播）。
3. 需要多路 Universe 时，增加输入端口并分别连接。
4. 可从 STATUS 读取 `isReady` 及发送反馈；外部可读 `/isReady`。

## 5. 示例

单 Universe 发送：

`DMX Universe` → `Artnet Out`（targetHost = 舞台控台 IP）→ 可选：STATUS 接日志或条件节点。
