# OSCInNode 帮助文档
## 节点功能
OSCInNode 是一个用于接收和处理 OSC (Open Sound Control) 协议消息的节点，提供以下功能：

- 实时监听 OSC 消息 ：通过 UDP 协议接收 OSC 格式的网络消息
- 可配置监听端口 ：支持自定义 UDP 监听端口（默认 6000）
- 自动解析 OSC 消息 ：自动解析 OSC 地址路径和参数值
- 多种数据类型支持 ：支持整数、浮点数、字符串等多种 OSC 数据类型
- 独立线程运行 ：网络操作在独立线程中执行，不会阻塞主界面
- OSC 远程控制 ：支持通过 OSC 协议远程控制节点参数
- 实时数据显示 ：在节点界面实时显示接收到的 OSC 地址和值
## 输入端口
此节点没有输入端口，仅作为数据源节点使用。

## 输出端口
- RESULT ：完整消息数据，输出 VariableData 类型，包含接收到的完整 OSC 消息信息（包括地址路径、参数值和数据类型）
- ADDRESS ：OSC 地址路径，输出字符串类型数据，表示 OSC 消息的地址路径（如 "/led/color"）
- VALUE ：参数值，输出接收到的 OSC 消息参数值
## 节点界面
节点界面包含以下控件：

- port ：监听端口输入框，默认为 6000，范围 0-65536
- address ：显示接收到的 OSC 地址路径
- value ：显示接收到的 OSC 参数值
## 使用示例
### 基本设置和接收
1. 添加 OSCInNode 到编辑器
2. 设置监听端口（例如 6000）
3. 使用 OSC 客户端（如 TouchOSC、OSC 控制器应用等）向指定端口发送 OSC 消息
4. 接收到的 OSC 消息将通过输出端口输出，并在节点界面显示
### 创建 OSC 控制系统
1. 添加 OSCInNode 到编辑器
2. 添加 DisplayNode 并连接到 RESULT、ADDRESS 或 VALUE 输出端口，用于显示接收到的数据
3. 添加 StringCompareNode 并连接到 ADDRESS 端口，用于根据 OSC 地址路径过滤消息
4. 添加 SwitchNode 并连接到 VALUE 端口，用于根据参数值执行不同操作
### 与其他节点集成
1. 添加 OSCInNode 到编辑器
2. 添加 TCPClientNode 或 UDPSocketNode
3. 将 OSCInNode 的 VALUE 输出连接到 TCPClientNode 或 UDPSocketNode 的 VALUE 输入
4. 实现通过 OSC 控制网络通信的功能
## 注意事项
- 端口冲突 ：确保选择的监听端口未被其他应用占用
- 防火墙设置 ：可能需要在防火墙中允许 UDP 通信
- 数据类型转换 ：OSC 消息的参数值会自动转换为适当的数据类型（整数、浮点数、字符串）
- VariableData 输出 ：RESULT 端口输出的是 VariableData 类型，包含多个字段，可通过其他端口获取特定字段
- OSC 远程控制 ：可以通过发送 OSC 消息到 "/value" 地址来远程控制节点的端口设置
- 独立线程 ：OSC 接收操作在独立线程中执行，不会阻塞主界面
## 技术细节
- 使用 QUdpSocket 实现 UDP 通信
- 使用 tinyosc 库解析 OSC 消息格式
- 支持 OSC 1.0 规范的基本数据类型（int32、float、string）
- 使用 QThread 创建独立线程处理网络操作
- 支持共享地址模式（ShareAddress）和地址重用（ReuseAddressHint）
- 自动解析接收到的 OSC 消息，提取地址路径和参数值
- 支持通过 NodeDelegateModel::registerOSCControl 注册 OSC 控制接口