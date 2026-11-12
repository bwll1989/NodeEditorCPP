# Object Detection

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

基于 **YOLO11n** ONNX 模型做通用目标检测，覆盖 **COCO 80 类**（person、car 等）。可按类别与置信度过滤，输出标注图与检测列表。

模型路径：`./plugins/Models/yolo11n-Detection.onnx`

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 末端口为 ENABLE（启停） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE | ImageData | 输入图像 |
| ENABLE | VariableData | 布尔启停；`true` 开始检测，`false` 停止（与界面「启动检测」按钮、OSC `/enable` 同步） |

### 输出

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE 0 | ImageData | 带检测框与类别标签的图像（关闭「绘制检测框」后不刷新以降低 CPU） |
| RESULT | VariableData | 检测结果，结构见下 |

**RESULT 数据结构**：

| 字段 | 说明 |
|------|------|
| `default` | 当前帧检测到的目标数量 |
| `result` | 检测列表（数组） |

`result` 中每一项包含：

| 字段 | 说明 |
|------|------|
| `class_id` | 类别 ID（0–79，与文末类别表一致） |
| `class_name` | 类别名称（如 `person`） |
| `confidence` | 置信度（0–1） |

> 检测框坐标（bbox）字段在源码中已注释，RESULT 不包含框坐标。

## 3. 界面与属性

控件面板（`WidgetEmbeddable=false`，显示在右侧属性面板，不嵌入节点体）：

| 控件 | 含义 |
|------|------|
| 置信度阈值 | 低于该值的检测被丢弃，默认 `0.4` |
| 检测对象下拉 | 按 COCO 类别过滤（80 类之一） |
| 最大帧率 (FPS) | 限制推理频率，默认 `15`，自动收敛到 1–30；越低 CPU 占用越低 |
| 绘制检测框 | 关闭后仍更新 RESULT，但不绘制标注、不刷新 IMAGE 输出 |
| 启动检测按钮 | 与 ENABLE 端口联动 |

可外部控制的属性（Q_PROPERTY）：`confidence`、`filterClassIndex`、`enabled`、`maxFps`、`drawOverlay`。

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/confidence` | 读写 | 置信度阈值（绑定置信度滑块） |
| `/enable` | 读写 | 启停检测（bool，绑定启动按钮） |
| `/filter` | 读写 | 类别索引（int，对应「检测对象」下拉；-1 或越界表示不过滤） |

> `maxFps`、`drawOverlay` 两个属性无 OSC 绑定，只能在界面或 `.flow` 工程中设置。

## 5. 行为细节

- **帧时钟驱动**：订阅 TimestampGenerator 帧时钟调度推理，按 `maxFps` 节流（间隔 = 1000/maxFps ms）；推理进行中到达的新帧会被合并（只保留最新），不积压不丢帧。
- **输入预处理**：从输入环形缓冲取最新帧读回 Mat，长边超过 1280 时等比缩放（INTER_AREA），再缩放为 640×640 blob（1/255 归一化、BGR→RGB）。
- **类别过滤语义**：`filter` 在有效类别范围内时只检查该类别得分；设为 -1 或越界值时取全部类别最高分。
- **NMS**：置信度阈值过滤后做非极大值抑制（NMS 阈值固定 0.45），框坐标按原图比例回映射并裁剪到图像边界。
- **输出去重**：检测结果与上帧相同则不重复推送 RESULT；仅「绘制检测框」开启时才把标注图推入输出环形缓冲（时间戳去重），并在图上叠加检测数、置信度与类别信息。
- **停止清理**：`enabled=false` 时取消挂起推理、清空输出图像缓冲、RESULT 复位为空并刷新；IMAGE 输入变成非图像数据时同样清理。
- **参数即时生效**：修改置信度或类别会立即重置时间戳去重并重新推理当前帧。
- **推理后端**：ONNX Runtime 会话懒初始化，优先 CUDA EP（device 0），不可用时回退 CPU；推理异常时复位会话、下次重建。
- **持久化**：`save()` 将 `confidence`、`classIndex`、`enabled`、`maxFps`、`drawOverlay` 写入 `values.*`；`load()` 恢复后若 enabled 为 true 自动开始检测。
- **资源清理**：析构时取消异步推理、断开帧时钟、反注册事件总线并释放 ONNX 资源。

## 6. 使用说明与典型接法

1. 连接图像源到 IMAGE。
2. 通过 ENABLE 端口、界面按钮或 OSC `/enable` 开启检测。
3. 按需要调整置信度与检测对象。
4. 用 RESULT 做条件判断，或接 IMAGE 0 做预览。

## 7. 示例

监控画面 → **Object Detection**（检测对象选 `person`）→ Condition → 触发报警节点。

## 8. COCO 80 类

类别 ID（`class_id`）从 0 开始，与 YOLO / Ultralytics 标准顺序一致。界面「检测对象」与下表对应。

| ID | 类别 | ID | 类别 | ID | 类别 | ID | 类别 |
|----|------|----|------|----|------|----|------|
| 0 | person | 20 | elephant | 40 | wine glass | 60 | dining table |
| 1 | bicycle | 21 | bear | 41 | cup | 61 | toilet |
| 2 | car | 22 | zebra | 42 | fork | 62 | tv |
| 3 | motorcycle | 23 | giraffe | 43 | knife | 63 | laptop |
| 4 | airplane | 24 | backpack | 44 | spoon | 64 | mouse |
| 5 | bus | 25 | umbrella | 45 | bowl | 65 | remote |
| 6 | train | 26 | handbag | 46 | banana | 66 | keyboard |
| 7 | truck | 27 | tie | 47 | apple | 67 | cell phone |
| 8 | boat | 28 | suitcase | 48 | sandwich | 68 | microwave |
| 9 | traffic light | 29 | frisbee | 49 | orange | 69 | oven |
| 10 | fire hydrant | 30 | skis | 50 | broccoli | 70 | toaster |
| 11 | stop sign | 31 | snowboard | 51 | carrot | 71 | sink |
| 12 | parking meter | 32 | sports ball | 52 | hot dog | 72 | refrigerator |
| 13 | bench | 33 | kite | 53 | pizza | 73 | book |
| 14 | bird | 34 | baseball bat | 54 | donut | 74 | clock |
| 15 | cat | 35 | baseball glove | 55 | cake | 75 | vase |
| 16 | dog | 36 | skateboard | 56 | chair | 76 | scissors |
| 17 | horse | 37 | surfboard | 57 | couch | 77 | teddy bear |
| 18 | sheep | 38 | tennis racket | 58 | potted plant | 78 | hair drier |
| 19 | cow | 39 | bottle | 59 | bed | 79 | toothbrush |

---

# Object Detection (Hytch)

## 1. 节点说明

与 **Object Detection** 共用同一套实现（为其子类），仅换用 **Hytch 定制 YOLO** 模型，只检测两类目标：`xgpl_boat`、`gdtlj_truck`。适用于特定场景的船只 / 车辆识别，而非通用 COCO 80 类。

模型路径：`./plugins/Models/yolo26n-dection-hytch.onnx`

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 末端口为 ENABLE（启停） |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。

### 输入

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE | ImageData | 输入图像 |
| ENABLE | VariableData | 布尔启停；与界面「启动检测」、OSC `/enable` 同步 |

### 输出

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE 0 | ImageData | 带检测框与类别标签的图像（可关闭绘制以降 CPU） |
| RESULT | VariableData | 检测结果：`default` 为目标数量，`result` 数组每项含 `class_id`（0 或 1）、`class_name`、`confidence` |

## 3. 界面与属性

与 Object Detection 相同：

- **置信度阈值**：默认 `0.4`
- **检测对象**：仅 `xgpl_boat`、`gdtlj_truck` 两项
- **最大帧率 (FPS)**：默认 `15`，范围 1–30
- **绘制检测框**：关闭后只更新 RESULT
- **启动检测**：与 ENABLE 端口联动

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

| 地址 | 方向 | 说明 |
|------|------|------|
| `/enable` | 读写 | 启停检测（bool） |
| `/confidence` | 读写 | 置信度阈值 |
| `/filter` | 读写 | 类别索引（0 = `xgpl_boat`，1 = `gdtlj_truck`） |

> `maxFps`、`drawOverlay` 无 OSC 绑定，只能在界面或 `.flow` 工程中设置。

## 5. 行为细节

与 Object Detection 完全一致（帧时钟驱动 + maxFps 节流、QtConcurrent 异步推理、输入长边 1280 缩放、640×640 blob、NMS 阈值 0.45、结果去重、停止时清空输出），差异仅在：

- 模型文件为 `yolo26n-dection-hytch.onnx`，输出类别数为 2；
- 类别表为 `xgpl_boat`（ID 0）、`gdtlj_truck`（ID 1）；
- 持久化键位相同（`values.confidence/classIndex/enabled/maxFps/drawOverlay`）。

## 6. 使用说明与典型接法

1. 连接图像源到 IMAGE。
2. 开启检测（ENABLE / 按钮 / OSC）。
3. 选择要跟踪的 Hytch 类别并调整置信度。
4. 用 RESULT 驱动后续逻辑（计数、条件、报警等）。

## 7. 示例

码头监控 → **Object Detection (Hytch)**（检测对象选 `xgpl_boat`）→ Count / Condition → 记录或报警。

## 8. Hytch 类别

| ID | 类别 |
|----|------|
| 0 | xgpl_boat |
| 1 | gdtlj_truck |
