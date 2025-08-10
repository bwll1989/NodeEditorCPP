# ArtnetInNode 帮助文档

## 节点功能
Artnet 输入节点（ArtnetInNode）用于接收 Artnet 协议的 DMX 数据，支持以下特性：

- 监听指定 Universe 的 Artnet 数据包
- 支持通道过滤，可只输出指定通道的数据
- 支持通过节点端口或界面设置 Universe、通道列表和过滤开关
- 输出原始和过滤后的 DMX 数据
- 运行在独立线程，不阻塞主 UI 线程

## 输入端口
节点提供如下输入端口：

1. UNIVERSE ：监听的 Universe 编号（整数类型）
2. CHANNELS ：需要过滤的通道列表（字符串类型，如 "1,2,5-10"）
3. FILTER ：过滤开关（布尔类型，true=启用过滤，false=不过滤）

## 输出端口
节点提供如下输出端口：

1. OUTPUT ：接收到的 Artnet 数据（VariableData 类型，包含 universe、host、各通道值等字段）

## 节点界面
节点界面包含以下控件：

- Universe ：输入框，用于设置监听的 Universe 编号
- Channels ：输入框，用于设置需要过滤的通道列表，支持单个和范围（如 "1,2,5-10"）
- 过滤器开关 ：GroupBox 勾选启用过滤

## 信号与状态
- 支持信号：UniverseChanged、ChannelsFilterChanged
- 过滤开关启用时，仅输出指定 Universe 和通道的数据
- 关闭过滤时，输出所有接收到的 Artnet 数据

## 使用示例
### 示例1：基本接收
1. 添加 Artnet 输入节点到流程图
2. 在界面中设置 Universe 编号
3. 关闭过滤器，节点将输出所有接收到的 Artnet 数据

### 示例2：通道过滤
1. 启用过滤器开关
2. 在 Channels 输入框中输入 "1,2,5-10"，仅输出这些通道的数据
3. 连接 OUTPUT 输出端口到下游节点，获取过滤后的数据

### 示例3：通过其他节点控制
1. 用整数源节点控制 UNIVERSE 输入端口，实现动态切换监听 Universe
2. 用字符串源节点控制 CHANNELS 输入端口，实现动态切换过滤通道
3. 用布尔源节点控制 FILTER 输入端口，实现远程开关过滤

## 注意事项
1. 通道列表支持单个通道和范围（如 "1,2,5-10"），通道编号从 0 开始
2. 启用过滤时，只有 Universe 匹配且通道在列表中的数据才会输出
3. 关闭过滤时，所有接收到的 Artnet 数据包都会输出
4. 输出数据为 VariableData 类型，包含 universe、host、各通道值等字段
5. 节点自动管理 Artnet 接收线程，无需手动干预

## 技术细节
- 节点使用 ArtnetReceiver 实现 Artnet 协议数据接收
- 支持多 Universe 监听和通道过滤
- 通信过程在独立线程中运行，确保 UI 响应性
- 支持通过界面和端口动态配置过滤参数