# InternalCommandsNode 帮助文档

## 节点功能
InternalControlNode 是用于管理OSC消息列表的内部控制节点，主要功能包括：

1. 可视化管理多个OSC消息模板
2. 支持消息的添加、删除和排序操作
3. 提供消息批量发送和测试功能
4. 支持拖拽操作调整消息顺序
5. 消息配置的持久化保存与加载
6. 与OSCSender模块深度集成实现消息发送

## 输入端口
- TRIGGER (PortIndex 0)：触发信号，接收任意类型数据触发消息发送

## 输出端口
- 无

## 节点界面
- OSC消息列表（OSCMessageListWidget）：
  - 显示所有已配置的OSC消息模板
  - 支持右键菜单操作（添加/删除消息）
  - 可拖拽调整消息顺序
  - "Add Message"：添加新消息模板
  - "Delete Message"：删除选中消息
  - "Clear All Message"：清空所有消息
- 操作按钮区域：
  - "Trigger"：测试触发选中的消息

## 保存与加载
- 节点支持保存和加载参数（宽、高、RGBA），关闭后再次打开会恢复上次设置。

## 使用示例
### 基本消息配置
1. 添加InternalControlNode到流程图
2. 右键点击消息列表选择"Add Message"
3. 在弹出对话框中配置OSC地址和参数
4. 连接触发信号到外部控制节点（如ButtonNode）


## 技术细节
消息存储：使用QList<OSCMessage>存储消息模板
序列化：通过QJsonArray实现配置保存/加载
线程安全：发送操作在独立线程执行
性能优化：使用模型-视图架构管理大型消息列表

## 注意事项
OSC地址需符合规范（以/开头的路径格式）