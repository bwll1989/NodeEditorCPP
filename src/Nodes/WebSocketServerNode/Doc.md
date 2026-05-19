# WebSocketServerNode 使用说明

## 用途
启动一个 WebSocket 服务器（监听指定端口），接收客户端消息，并可向所有客户端广播消息。

## 端口
### 输入（VariableData）
- PORT：监听端口（int）
- VALUE：要广播的内容（string）。更新时会立即广播一次
- TRIGGER：触发广播（任意类型）。收到输入即广播一次（广播当前 VALUE）

### 输出（VariableData）
- RESULT：收到的数据（键值表）
- HOST：发送方地址（string，等同 RESULT.host）
- VALUE：收到的原始字节（QByteArray，等同 RESULT.default）
- HEX：收到数据的十六进制（string，等同 RESULT.hex）

## 参数/界面
- Port：监听端口
- Value：要广播的内容
- Message Type：消息类型（0=文本，1=二进制）
- Format：发送编码（0=HEX，1=UTF-8，2=ASCII）
- Send：广播一次

## 外部控制（可选）
- /port（int）
- /value（string）
- /send（bool 或任意值）：触发广播一次

## 输出字段（RESULT）
RESULT 常用字段：
- host：发送方地址（string）
- hex：收到数据的 hex（string）
- utf-8：按 UTF-8 解码后的文本（string）
- ascii：按 ASCII/Latin1 解码后的文本（string）
- default：原始字节（QByteArray）

## 使用步骤
1. 设置 PORT 启动监听（端口变更会自动重启服务）。
2. 客户端发送的数据会从 RESULT 输出。
3. 要广播时：设置 VALUE（或点 Send）即可向所有已连接客户端广播。 
