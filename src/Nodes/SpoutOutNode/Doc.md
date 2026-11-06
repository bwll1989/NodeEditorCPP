# Spout Out

## 1. 节点说明

将节点图中的图像通过 Spout 发送到其他应用程序。需先启用发送，再持续送入图像帧。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 0 | 否 | 0 | 0 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

- **IMAGE**（ImageData）：要发送的画面。
- **ENABLE**（VariableData）：是否发送（true 开始，false 停止）。

### 输出

无输出端口。

## 3. 界面与属性

节点控件面板（`WidgetEmbeddable` 未显式声明，取框架默认值 `true`，控件**嵌入节点体**显示）包含：

| 控件 | 说明 |
|------|------|
| 发送名称输入框 | Spout 发送端名称，接收端软件按此名选择源；留空时回落为默认名 `NodeEditor Spout` |
| 开始/停止发送按钮 | 控制发送线程启停 |
| 连接状态指示 | 显示当前是否正在发送 |

本节点未声明任何 `Q_PROPERTY`，控件通过信号槽直接驱动内部逻辑，不走属性系统。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未注册任何专属 OSC 地址（源码无 `Q_PROPERTY` 外部属性、无 `registerExternalBinding` 调用），外部系统无法通过 OSC 直接控制；启停与改名只能通过界面控件或 ENABLE 输入端口完成。

## 5. 行为细节

- **端口语义**：输入 0（IMAGE）仅缓存 ImageData 共享指针并重置去重时间戳；输入 1（ENABLE）布尔值统一走 `setSendingEnabled`（`true` 启动发送线程并立即尝试推一帧，`false` 停止）。
- **帧时钟驱动发送（与 NDI Out 同模式）**：节点就绪后连接全局 `TimestampGenerator` 的 `frameCountUpdated` 信号，每个系统 tick 检查一次——正在发送且输入环形缓冲有**比上次已发更新的帧**（按时间戳去重）时，才读回最新帧交给发送线程，静止画面不重复发送。
- **发送线程**：独立 `SpoutSenderThread`——主线程创建离屏表面（QOffscreenSurface，CompatibilityProfile），工作线程自建 OpenGL 上下文并 makeCurrent，经 SpoutLibrary 发送；循环以约 16ms 间隔轮询，收到新帧时合并（只留最新）。
- **格式转换**：发送前统一转为 BGRA（3 通道 BGR→BGRA、4 通道转 CV_8UC4、单通道灰度→BGRA），`SendImage(GL_BGRA)`。
- **运行中改名**：发送线程检测到名称变更后 `ReleaseSender` 再以新名重建发送器；空名回落默认名 `NodeEditor Spout`。
- **停止与析构**：停止时线程循环退出并释放 Spout 发送器；析构等待线程退出最多 3 秒，防止残留。
- **持久化**：`save()` 使用**顶层键** `senderName`（发送名）、`isSending`（是否发送），无 `values` 包裹、不调用基类 save；`load()` 恢复发送名（转发给线程）并按 `isSending` 恢复启停状态。

## 6. 使用说明与典型接法

1. 设置 Spout 名称（其他软件用此名接收）。
2. 将 IMAGE 连接图像源，ENABLE 置 true 或点「开始」。
3. 在接收端选择对应 Spout 源。

## 7. 示例

NDI In → 调色节点 → Spout Out（名称「MainStage」）→ Resolume Arena。

