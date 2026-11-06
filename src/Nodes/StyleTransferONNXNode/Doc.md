# StyleTransfer

## 1. 节点说明

使用 ONNX 模型对图像做风格迁移（默认 AnimeGAN 模型）。输入一帧即推理一帧，输出风格化图像及结果变量。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 1 | 否 | 1 | 1 | — | 无 |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

- **IMAGE**（ImageData）：原始图像。

### 输出

- **IMAGE 0**（ImageData）：风格化后的图像。
- **RESULT**（VariableData）：推理相关结果数据。

## 3. 界面与属性

本节点不提供自定义控件；`WidgetEmbeddable=false`，框架基于 Q_PROPERTY 自动生成的属性树显示在右侧属性面板，不嵌入节点体，仅含一个 Enable 开关。

节点公开 1 个 `Q_PROPERTY`：

| 属性 | 类型 | 说明 |
|------|------|------|
| `enable` | bool | 是否执行推理（默认 `false`，需外部或 OSC 打开） |

模型路径在源码中固定为 `./plugins/Models/AnimeGANv3_Hayao_36.onnx`（模型输入 720×720），界面不可更换；内部节流参数 `maxFps`（默认 15）未暴露为属性。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/enable` | 可写（命令） | 布尔，开启/关闭推理；关闭时取消排队中的推理并清空输出 |

`enable` 通过 `registerExternalBinding` 注册并订阅 Command 事件，外部写入实时生效；`maxFps` 为内部私有变量，未注册绑定，OSC 不可调。

## 5. 行为细节

- **端口语义**：输入 0 仅缓存 ImageData 共享指针并重置去重时间戳；连接断开时取消挂起推理并清空输出图像缓冲。
- **推理调度（帧时钟驱动）**：节点就绪后连接全局 `TimestampGenerator` 帧 tick，同时输入端口更新也会触发；用 `QFutureWatcher` 保证同时只有一次推理在飞（推理中新请求跳过，结束后自动补一次最新帧），配合输入时间戳去重与 `maxFps=15` 节流（约 67ms 最小间隔），静帧不重复推理。
- **会话管理**：ONNX Runtime 会话懒初始化并缓存，模型路径未变不重建；CUDA EP 可用则启用（device 0），否则回退 CPU；会话线程数取 CPU 核数。
- **预处理**：输入帧缩放至 720×720 → BGR 转 RGB → 归一化到 [0,1] 浮点。
- **后处理**：模型输出（CHW 布局）转 HWC → ×255 → 截断至 [0,255] → 8 位图 → 缩放回原图尺寸 → RGB 转 BGR，推入输出环形缓冲（按时间戳去重）。
- **RESULT 内容**：`status`（success/error）、`input_size`、`output_size`、`model_input_size`、`timestamp`（毫秒）；推理异常时 `status=error` 并附 `error_message`，图像端口输出原图。
- **启停清理**：`setEnable(false)` 取消挂起推理、清空输出图像缓冲并重置 RESULT；重新开启时立即调度一次推理。
- **持久化**：`values.enabled`（布尔，注意键名是 `enabled`）；加载后按值恢复启停。
- **析构**：取消挂起推理、断开帧时钟、退订事件总线、释放 ONNX 会话资源。

## 6. 使用说明与典型接法

1. 确认 ONNX 模型已部署。
2. 连接图像源到 IMAGE。
3. 使用 IMAGE 0 显示或继续处理；RESULT 可供调试或逻辑判断。

## 7. 示例

摄像头 → StyleTransfer → NDI Out，实时漫画风直播画面。

