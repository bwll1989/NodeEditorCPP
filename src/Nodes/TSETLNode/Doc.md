# TSETL

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

作为 **TSETL** 触发事件系统的 TCP 客户端：连接服务器、解析二进制帧中的 JSON，在收到 `SignalID` 消息时输出信号 ID 与完整 JSON；并每 5 秒发送心跳维持连接。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 3 | 否 | 3 | 3 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| HOST | VariableData | 服务器 IP 或主机名 |
| PORT | VariableData | 端口号（默认 11001） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| SIGNAL_ID | VariableData | 解析出的 SignalID 字符串 |
| JSON_DATA | VariableData | 完整 JSON 消息（键值表） |
| CONNECTION | VariableData | TCP 是否已连接（布尔） |

## 3. 界面与属性

节点控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 说明 |
|------|------|
| 主机（行编辑框） | 服务器地址，默认 `127.0.0.1`；编辑完成即重连 |
| 端口（数字拖动框） | 服务器端口，默认 `11001`；变更即重连 |
| 连接状态 | 是否在线 |
| 最近信号 | 显示最后一次 SignalID 与时间 |

节点公开 2 个 `Q_PROPERTY`：

| 属性 | 类型 | 说明 |
|------|------|------|
| `host` | QString | 服务器地址（变更即重连） |
| `port` | int | 服务器端口（变更即重连） |

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/host` | 可写（命令） | 字符串，改服务器地址并立即重连 |
| `/port` | 可写（命令） | 整型，改端口并立即重连 |
| `/connect` | 只读 | 连接状态反馈（布尔）——仅注册 member 绑定未订阅 Command，**外部写无效** |

`host`/`port` 通过 `registerExternalBinding` 注册（绑定界面控件）并订阅 Command 事件；连接状态由 socket 回调驱动，经 `stateFeedBack` 推送到 `/connect`。

## 5. 行为细节

- **端口语义**：HOST/PORT 输入等效于改属性——变更即重连。构造后**自动连接**一次。
- **线程模型**：复用 `TcpClient` 设备层，socket 在独立工作线程收发（含 1s 周期自动重连），模型层走队列信号。
- **协议解析**：TSETL 二进制帧 = 包头 `0xFBFBFBFB`（4B 小端）+ 数据长度（2B）+ JSON 数据 + CRC16（Modbus，2B）；长度不足、包头不符、CRC 校验失败的帧直接丢弃。
- **JSON 分发**：按 `MsgID` 分发——`SignalID` 消息提取 `Datas.SignalID` / `Datas.DateTime`，更新 SIGNAL_ID（字符串）与 JSON_DATA（完整 JSON 键值表）输出并刷新界面；`HeartBeat`/`HeartBeatReply` 仅内部消化；其他 MsgID 忽略。
- **心跳**：连接成功后每 **5 秒**发送一帧心跳（`{MsgID:"HeartBeat", FromObject:"QSC", ToObject:"TSETL", datas:{DateTime}}`，按 TSETL 帧封装后以 HEX 文本发出）；断开即停。
- **连接状态**：连接/断开时更新 CONNECTION 输出、界面状态，并反馈 `/connect`。
- **端口连接策略**：所有输入、输出端口均可连多路（Many/Many）。
- **持久化**：`values.Port`、`values.IP`（键名首字母大写）；加载后恢复并触发重连。

## 6. 使用说明与典型接法

1. 填写 TSETL 服务器地址与端口。
2. 连接成功后等待 `SignalID` 类消息。
3. SIGNAL_ID 可驱动场景切换；JSON_DATA 可接解析或记录。
4. CONNECTION 可用于界面或联锁（未连接时不执行）。

## 7. 示例

TSETL 服务器推送 SignalID `Scene_A` → SIGNAL_ID 接 Switch 选路 → JSON_DATA 接日志节点存档。

