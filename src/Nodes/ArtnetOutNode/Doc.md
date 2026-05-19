# ArtnetOutNode 使用说明

## 用途
ArtnetOutNode 用于把上游产生的 Art-Net/DMX 数据发送到网络指定主机（可以是设备 IP，也可以是广播地址）。

## 输入端口
- UNIVERSE DATA（最多 4 路）：接收上游的 Universe 数据包（通常来自 ArtnetUniverse 节点）。

## 输出端口
- STATUS：发送状态（VariableData），常见字段：
  - `status`：`success` / `failed`
  - `universe`：已发送的 Universe
  - `host`：发送目标
  - `error`：失败原因（仅 failed 时）

## 配置项
- 目标主机（targetHost）：例如 `192.168.1.100` 或 `192.168.1.255`（广播）。

## 外部控制（OSC）
- `/targetHost`：设置目标主机（string）。
- `/isReady`：可读状态（bool），表示发送器是否就绪。

## 快速上手
1. 放入 ArtnetOutNode。
2. 设置 targetHost（建议先用广播地址测试）。
3. 把上游 Universe 数据连接到任意输入端口。
4. 观察 STATUS 输出，确认 `success/failed`。

## 常见问题
- STATUS 显示 failed：检查目标 IP 是否可达、网络是否允许广播、是否被防火墙拦截。
- 没有任何 STATUS：确认上游确实在持续输出 Art-Net 数据包。