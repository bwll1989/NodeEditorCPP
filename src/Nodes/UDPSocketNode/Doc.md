# UDP Socket

## 1. 节点说明

UDP 套接字节点，同时支持在本机监听（收包）和向目标地址发送（发包）。无连接、低延迟，适合发现广播、实时控制、音视频同步信号等场景。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 4 | 是 | 1 | 不限 | VariableData | 末端口为 TRIGGER（按当前内容立即发送） |
| 输出 | 4 | 是 | 1 | 不限 | VariableData | 末端口为 HEX（十六进制） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | TARGET HOST | 设置发送目标 IP |
| 1 | TARGET PORT | 设置发送目标端口 |
| 2 | VALUE | 设置发送内容并立即发送 |
| 3 | TRIGGER | 收到数据即按当前内容发送 |

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | RESULT | 完整接收消息 |
| 1 | HOST | 发送方地址 |
| 2 | VALUE | 接收文本 |
| 3 | HEX | 接收数据十六进制 |

## 3. 界面与属性

节点控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 说明 |
|------|------|
| Listening host / port | 本机监听地址与端口（默认 `127.0.0.1:6000`），变更即重新绑定监听 |
| Target host / port | 发送目标地址与端口（默认 `127.0.0.1:6011`），发送时使用 |
| value（行编辑框） | 发送内容；编辑完成立即按当前格式发送 |
| Format（下拉框） | 发送编码：HEX / UTF-8 / ASCII |
| Send（按钮） | 按当前 value 与目标发送 |

节点公开 5 个 `Q_PROPERTY`：

| 属性 | 类型 | 说明 |
|------|------|------|
| `targetHost` | QString | 发送目标 IP |
| `targetPort` | int | 发送目标端口 |
| `value` | QString | 发送内容（赋值即发送） |
| `listeningHost` | QString | 监听 IP（变更即重绑） |
| `listeningPort` | int | 监听端口（变更即重绑） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/targetHost` | 可写（命令） | 字符串，改发送目标 IP |
| `/targetPort` | 可写（命令） | 整型，改发送目标端口 |
| `/value` | 可写（命令） | 字符串，设置发送内容并**立即发送** |
| `/listeningHost` | 可写（命令） | 字符串，改监听 IP 并重新绑定 |
| `/listeningPort` | 可写（命令） | 整型，改监听端口并重新绑定 |
| `/send` | 可写（命令） | 触发一次发送；模型回发 `/send` true→false 脉冲状态反馈 |

`targetHost`/`targetPort`/`value`/`listeningHost`/`listeningPort`/`send` 均通过 `registerExternalBinding` 注册（绑定界面控件）并订阅 Command 事件。

## 5. 行为细节

- **端口语义**：TARGET HOST/PORT 输入等效改属性（仅存值，发送时使用）；VALUE 输入设置内容并**立即发送**；TRIGGER 收到**任何数据**（不检查真假）都按当前内容发送——与 TCP Client 的 TRIGGER（仅 `true` 发送）不同。
- **线程模型**：复用 `UdpSocket` 设备层，socket 在独立工作线程收发，模型层走队列信号。
- **监听绑定**：listening host/port 任一变更即调用设备层 `setHost` 重新绑定监听套接字。
- **发送**：按 Format 编码（HEX=`QByteArray::fromHex`、UTF-8、ASCII/Latin1）后发往 targetHost:targetPort；发送动作伴随 `/send` true→false 脉冲反馈。
- **接收**：每个数据报打包为 QVariantMap `{host（发送方地址）, hex, utf-8, ascii, default（原始字节）}` 推给 RESULT，同时刷新 HOST/VALUE/HEX 三路输出。
- **端口连接策略**：所有输入、输出端口均可连多路（Many/Many）；端口数量可编辑。
- **持久化**：`values."Listening host"`、`"Listening port"`、`"Target host"`、`"Target port"`、`Value`、`Format`（注意键名**带空格**且首字母大写）；加载后恢复全部参数并重绑监听。

## 6. 使用说明与典型接法

1. 配置 **Listening** 以接收 UDP 包；配置 **Target** 以指定发送目的地。
2. 收到数据时四个输出端口同时更新。
3. 点 **Send** 或从 **VALUE** / **TRIGGER** 触发发送。
4. 监听地址或端口变化后，节点会重新绑定监听套接字。

## 7. 示例

- **Art-Net 类控制**：监听端口收设备反馈，**Target** 指向控台 IP 发 DMX 类 UDP 包（HEX 格式）。
- **心跳广播**：定时 **TRIGGER**，**value** 固定为 `PING`，发往 `255.255.255.255` 的广播端口。
- **分路输出**：**HEX** 连十六进制显示，**HOST** 连日志节点记录来源设备。

