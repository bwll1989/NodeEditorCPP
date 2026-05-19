# UDPSocketNode 使用说明

## 用途
同时支持 UDP 发送与接收：本地监听一个端口接收数据报，并按指定目标地址发送数据。

## 端口
### 输入（VariableData）
- TARGET HOST：目标地址（string）
- TARGET PORT：目标端口（int）
- VALUE：要发送的内容（string）。更新时会立即发送一次
- TRIGGER：触发发送（任意类型）。收到输入即发送一次（发送当前 VALUE）

### 输出（VariableData）
- RESULT：收到的数据（键值表）
- HOST：发送方地址（string，等同 RESULT.host）
- VALUE：收到的原始字节（QByteArray，等同 RESULT.default）
- HEX：收到数据的十六进制（string，等同 RESULT.hex）

## 参数/界面
- listening host / listening port：本地监听（默认 127.0.0.1:6000）
- target host / target port：目标地址（默认 127.0.0.1:6011）
- value：发送内容
- format：发送编码（0=HEX，1=UTF-8，2=ASCII）
- Send：发送一次

## 外部控制（可选）
- /listeningHost（string）
- /listeningPort（int）
- /targetHost（string）
- /targetPort（int）
- /value（string）
- /send（bool 或任意值）：触发发送一次

## 输出字段（RESULT）
RESULT 常用字段：
- host：发送方地址（string）
- hex：收到数据的 hex（string）
- utf-8：按 UTF-8 解码后的文本（string）
- ascii：按 ASCII/Latin1 解码后的文本（string）
- default：原始字节（QByteArray）

## 使用步骤
1. 设置 listening host/port 作为本地接收端口。
2. 设置 target host/port 作为发送目标。
3. 写入 VALUE（或点 Send）发送；从 RESULT/HEX 获取接收数据。 
