# TCPClientNode 使用说明

## 用途
连接到远端 TCP 服务器，发送数据并接收返回数据。

## 端口
### 输入（VariableData）
- HOST：服务器地址（string）
- PORT：服务器端口（int）
- VALUE：要发送的内容（string）。更新时会立即发送一次
- TRIGGER：触发发送（bool）。为 true 时发送一次（发送当前 VALUE）

### 输出（VariableData）
- RESULT：收到的数据（键值表）
- HOST：当前连接目标 host（string）
- VALUE：当前待发送 value（string）
- HEX：收到数据的十六进制（string，等同 RESULT.hex）

## 参数/界面
- Host / Port：连接目标
- Value：要发送的内容
- Format：发送编码（0=HEX，1=UTF-8，2=ASCII）
- Connected：连接状态显示（只读）
- Send：发送一次

## 外部控制（可选）
### 属性（写入）
- /host（string）
- /port（int）
- /value（string）

### 命令（触发）
- /send（bool 或任意值）：触发发送一次

### 反馈（只读）
- /connected：连接状态

## 输出字段（RESULT）
RESULT 常用字段：
- host：对端地址（string）
- hex：收到数据的 hex（string）
- utf-8：按 UTF-8 解码后的文本（string）
- ascii：按 ASCII/Latin1 解码后的文本（string）
- default：原始字节（QByteArray）

## 使用步骤
1. 设置 HOST/PORT，等待连接成功（Connected 变为已连接）。
2. 设置 VALUE 并发送（点 Send，或写入 VALUE 输入端口）。
3. 从 RESULT/HEX 获取接收数据，并按需要选择 utf-8/ascii/hex 字段使用。 
