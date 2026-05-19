# MqttClientNode 使用说明

## 用途
MqttClientNode 用于连接 MQTT Broker，发布消息到指定 Topic，并接收订阅 Topic 的消息输出到下游节点。

## 端口
- 输入（5）
  - HOST（端口 0）：Broker 地址（string）
  - PORT（端口 1）：端口（int，默认 1883）
  - TOPIC（端口 2）：订阅/发布的 Topic（string）
  - VALUE（端口 3）：要发布的消息内容（string）；写入后会立刻发布一次
  - TRIGGER（端口 4）：触发发布（任意值/true 触发一次）
- 输出（3）
  - RESULT（端口 0）：VariableData（收到的消息）
  - TOPIC（端口 1）：VariableData（string，当前消息 topic）
  - VALUE（端口 2）：VariableData（string，当前消息 payload）

RESULT 字段：
- `topic`：消息 topic（string）
- `default`：payload（UTF-8 string）
- `hex`：payload 的十六进制字符串（便于查看二进制数据）

## 节点界面
- host/port/username/password：连接参数
- topic：当前订阅 topic
- payload：发布内容
- QoS：订阅/发布 QoS
- Publish：手动发布按钮
- 状态按钮：显示连接状态（Connected/Disconnect）

## 外部控制（可选）
- `/host`：设置 host（string）
- `/port`：设置 port（int）
- `/topic`：设置 topic（string）
- `/payload`：设置 payload（string）
- `/publish`：触发发布一次（任意值/true 触发）

反馈：
- `/connect`：连接状态（bool，true=已连接）

## 使用步骤
1. 设置 host/port（需要认证则填写 username/password）。
2. 设置 topic，连接成功后会自动订阅该 topic。
3. 发布：
   - 在 payload 输入内容，点 Publish
   - 或给 VALUE 输入端口写入内容（会立刻发布）
4. 接收：当收到订阅消息时，RESULT/TOPIC/VALUE 输出会更新。

## 注意事项
- topic 改变会重新订阅新 topic。
- payload 默认为 UTF-8 字符串；若发送/接收的是二进制内容，可用 hex 字段查看。
