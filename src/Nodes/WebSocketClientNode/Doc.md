# WebSocketClientNode 帮助文档

## 节点功能
WebSocket客户端节点实现了WebSocket客户端通信功能，支持以下特性：

- 连接远程WebSocket服务器（ws/wss协议）
- 发送文本或二进制消息到服务器
- 接收服务器返回的数据（文本/二进制自动识别）
- 自动重连机制（每5秒尝试重连）
- 支持通过节点端口或界面控制通信参数
- 消息格式支持HEX、UTF-8、ASCII

## 输入端口
节点提供了4个输入端口：

1. URL ：服务器WebSocket地址（字符串类型，如"ws://127.0.0.1:2003"）
2. VALUE ：要发送的消息内容（字符串类型）
3. TYPE ：消息类型（整数类型，0=文本，1=二进制）
4. FORMAT ：消息格式（整数类型，0=HEX，1=UTF-8，2=ASCII）
5. TRIGGER ：触发消息发送的信号（任意类型，收到数据即触发）

## 输出端口
节点提供了4个输出端口：

1. RESULT ：接收到的完整数据（VariableData类型，包含url、hex、utf-8、ascii、default、type字段）
2. URL ：连接的服务器地址（字符串类型）
3. VALUE ：接收到的原始数据（字符串类型）
4. HEX ：接收到的数据的十六进制表示（字符串类型）

## 节点界面
节点界面包含以下控件：

- URL ：输入框，用于设置WebSocket服务器地址，默认为"ws://127.0.0.1:2003"
- Value ：输入框，用于输入要发送的消息内容
- Type ：下拉框，选择消息类型（文本/二进制）
- Format ：下拉框，选择消息格式（HEX/UTF-8/ASCII）
- Send ：按钮，点击发送消息到服务器

## 信号与状态
- 连接成功时，Send按钮将被启用；连接失败或断开时，Send按钮将被禁用
- 节点自动处理连接状态，无需手动管理
- 支持信号：connected、disconnected、errorOccurred、messageReceived

## 使用示例
### 示例1：基本连接和发送消息
1. 添加WebSocket客户端节点到流程图
2. 在节点界面中设置目标服务器的URL
3. 在Value输入框中输入要发送的消息
4. 选择Type和Format
5. 点击Send按钮发送消息
6. 从输出端口获取服务器响应

### 示例2：通过其他节点控制WebSocket客户端
1. 添加WebSocket客户端节点和字符串源节点到流程图
2. 将字符串源节点的输出连接到WebSocket客户端节点的URL输入端口，设置服务器地址
3. 添加另一个字符串源节点，将其输出连接到WebSocket客户端节点的VALUE输入端口，设置要发送的消息
4. 添加整数源节点，将其输出连接到WebSocket客户端节点的TYPE和FORMAT输入端口，设置消息类型和格式
5. 添加按钮节点，将其输出连接到WebSocket客户端节点的TRIGGER输入端口，用于触发消息发送
6. 将WebSocket客户端节点的RESULT输出端口连接到数据信息节点，以查看接收到的数据

### 示例3：自动重连机制
WebSocket客户端节点内置了自动重连机制，当连接断开时，节点会每隔5秒自动尝试重新连接服务器，直到连接成功。

## 注意事项
1. 支持ws://和wss://协议
2. 发送消息时可选择文本或二进制类型，并指定格式（HEX/UTF-8/ASCII）
3. 接收到的数据会通过RESULT输出端口以VariableData格式输出，包含以下字段：
   - url：服务器地址
   - hex：数据的十六进制表示
   - utf-8：UTF-8解码内容
   - ascii：ASCII解码内容
   - default：原始数据
   - type：消息类型（TextMessage/BinaryMessage）
4. 可以通过OSC协议远程控制节点的参数（/url, /value, /type, /format, /send）

## 技术细节
- 节点使用QWebSocket实现WebSocket通信
- 通信过程在单独的线程中运行，不会阻塞主UI线程
- 支持UTF-8编码的文本消息发送和接收
- 支持二进制消息收发