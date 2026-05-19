# WebSocketClientNode 使用说明

## 用途
连接 WebSocket 服务器（ws/wss），发送消息并接收返回数据。

## 端口
### 输入（VariableData）
- URL：服务器地址（string，例如 ws://127.0.0.1:2003）
- VALUE：要发送的内容（string）。更新时会立即发送一次
- TRIGGER：触发发送（任意类型）。收到输入即发送一次（发送当前 VALUE）

### 输出（VariableData）
- RESULT：收到的数据（键值表）
- URL：当前连接 URL（string）
- STRING：收到的默认内容（等同 RESULT.default）

## 参数/界面
- URL：服务器地址
- Value：要发送的内容
- Type：消息类型（0=文本，1=二进制）
- Format：发送编码（0=HEX，1=UTF-8，2=ASCII）
- Connected：连接状态显示（只读）
- Send：发送一次

## 外部控制（可选）
### 属性（写入）
- /url（string）
- /host（string）：/url 的别名
- /value（string）

### 命令（触发）
- /send（bool 或任意值）：触发发送一次

### 反馈（只读）
- /connected：连接状态

## 输出字段（RESULT）
RESULT 常用字段：
- url：对端地址（string）
- hex：收到数据的 hex（string 或 bytearray，取决于消息类型）
- utf-8：按 UTF-8 解码后的文本（string）
- ascii：按 ASCII/Latin1 解码后的文本（string，可能为空）
- default：默认内容（通常为文本，string）
- type：消息类型标记（string）

## 使用步骤
1. 设置 URL，等待 Connected 显示为已连接。
2. 设置 VALUE 并发送（点 Send，或写入 VALUE 输入端口）。
3. 从 RESULT/STRING 获取收到的数据，并按需要选择 utf-8/ascii/hex 字段使用。 
