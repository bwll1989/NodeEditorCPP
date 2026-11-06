# Mqtt Client

## 1. 节点说明

MQTT 客户端节点，连接 Broker 后订阅主题、接收消息，并可向主题发布内容。适合物联网设备、智能家居、分布式状态同步等场景。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 5 | 否 | 5 | 5 | — | 末端口为 TRIGGER（发布触发） |
| 输出 | 3 | 否 | 3 | 3 | — | 末端口为 VALUE（消息文本） |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | HOST | 设置 Broker 地址 |
| 1 | PORT | 设置 Broker 端口 |
| 2 | TOPIC | 设置主题（并重新订阅） |
| 3 | VALUE | 设置发布内容并立即发布 |
| 4 | TRIGGER | 收到数据即按当前 **payload** 发布 |

### 输出

| 端口 | 名称 | 说明 |
|------|------|------|
| 0 | RESULT | 完整消息（含 topic、default、hex 等） |
| 1 | TOPIC | 收到消息的主题 |
| 2 | VALUE | 收到消息的文本内容 |

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 含义 |
|------|------|
| Host | Broker 地址，默认 `127.0.0.1`（支持 `mqtt://`/`mqtts://`/`ssl://`/`tcp://` 前缀与路径，连接时自动剥离） |
| Port | Broker 端口，默认 1883 |
| Username / Password | 可选认证信息 |
| Topic | 订阅与发布使用的主题 |
| Payload | 要发布的消息体 |
| QoS | 服务质量下拉框 0/1/2（订阅与发布共用） |
| 状态按钮 | 连接状态显示（绿色 Connected / 红色 Disconnect） |
| Publish | 手动按当前 Topic/Payload 发布 |

可外部控制的属性（Q_PROPERTY）：`host`、`port`、`username`、`password`、`topic`、`payload`。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点注册了 6 个专属 OSC 地址：

| 地址 | 方向 | 说明 |
|------|------|------|
| `/host` | 读写 | Broker 地址（写入后防抖重连，setter 即时回读） |
| `/port` | 读写 | Broker 端口（同上） |
| `/topic` | 读写 | 订阅主题（写入后若已连接立即重新订阅） |
| `/payload` | 读写 | 发布内容（仅更新缓存，不自动发布） |
| `/publish` | 命令 | 触发按当前 Topic/Payload 发布，成功后 `/publish` 回读 true 脉冲 |
| `/connect` | 只读状态 | 连接状态（true/false），状态变化时自动反馈；未订阅 Command，不可外部写 |

`username` / `password` 属性无 OSC 地址，只能在界面或经 `.flow` 工程文件配置。

## 5. 行为细节

- **连接与防抖**：修改 Host / Port / Username / Password 任一项都清空当前连接键，启动 400 ms 单次定时器防抖重连；连接键（归一化 host、端口、账号、密码）未变化时不重复连接。
- **Host 归一化**：连接时自动剥离 `mqtt://`/`mqtts://`/`ssl://`/`tcp://` 前缀与路径部分。
- **自动订阅**：连接成功后自动订阅当前 Topic（QoS 取下拉当前值）；修改 Topic 且已连接时立即重新订阅；Topic 为空或未连接时不订阅。
- **发布**：VALUE 输入 = 设置 Payload 并立即发布；TRIGGER 输入 = 收到任意数据即按当前 Payload 发布；界面 Publish 与 OSC `/publish` 同效。每次发布后 `/publish` 地址回读 true。
- **消息接收**：收到消息后组装键值对象 `{topic, default（UTF-8 文本）, hex（空格分隔十六进制）}`，从三个输出端口同时刷新。
- **端口行为**：输入 0/1/2 分别等价于写 `/host` `/port` `/topic` 属性；断开输入不触发任何动作（端口数据不缓存）。
- **连接策略**：所有端口 ConnectionPolicy 均为 Many，可多对多连接。
- **持久化**：`save()` 将 `host`、`port`、`username`、`password`、`topic`、`payload`、`qos` 写入 `values.*`；`load()` 恢复后调度防抖连接。
- **资源清理**：析构时主动断开 Broker 连接并释放客户端。

## 6. 使用说明与典型接法

1. 填写 Broker **Host**、**Port** 及可选账号密码；节点会自动连接。
2. 设置 **Topic** 后，连接成功即订阅该主题；收到消息从 **RESULT** 等端口输出。
3. 填写 **Payload** 后点 **Publish**，或从 **VALUE** / **TRIGGER** 触发发布。
4. 修改 Host、Port、用户名或密码会触发防抖重连；工程保存全部连接参数与 QoS。

## 7. 示例

- **传感器上报**：主题 `home/temp`，上游数值连 **VALUE**，定时 **TRIGGER** 发布 JSON。
- **灯控订阅**：订阅 `building/floor1/light/#`，**TOPIC** 连路由节点，**VALUE** 解析开关状态。
- **远程改 Broker**：OSC 更新 `/host` 为 `mqtt.example.com`，`/port` 为 `8883`，节点自动重连并重新订阅。

