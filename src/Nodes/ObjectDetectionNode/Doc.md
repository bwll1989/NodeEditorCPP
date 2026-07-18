# Object Detection

## 1. 节点说明

基于 **YOLO11n** ONNX 模型做通用目标检测，覆盖 **COCO 80 类**（person、car 等）。可按类别与置信度过滤，输出标注图与检测列表。

模型路径：`./plugins/Models/yolo11n-Detection.onnx`

## 2. 端口说明

### 输入

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE | ImageData | 输入图像 |
| ENABLE | VariableData | 布尔启停；`true` 开始检测，`false` 停止（与界面「启动检测」按钮、OSC `/enable` 同步） |

### 输出

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE 0 | ImageData | 带检测框与类别标签的图像（关闭「绘制检测框」后可不刷新以降低 CPU） |
| RESULT | VariableData | 检测结果，见下文 |

## 3. RESULT 数据结构

| 字段 | 说明 |
|------|------|
| `default` | 当前帧检测到的目标数量 |
| `result` | 检测列表（数组） |

`result` 中每一项包含：

| 字段 | 说明 |
|------|------|
| `class_id` | 类别 ID（0–79，与下表一致） |
| `class_name` | 类别名称（如 `person`） |
| `confidence` | 置信度（0–1） |

## 4. 界面说明

- **置信度阈值**：低于该值的检测会被丢弃，默认 `0.4`
- **检测对象**：按 COCO 类别过滤（下拉为 80 类之一）
- **最大帧率 (FPS)**：限制推理频率，默认 `15`，范围 1–30；越低 CPU 占用越低
- **绘制检测框**：关闭后只更新 RESULT，不做图像标注与 IMAGE 输出刷新
- **启动检测**：与 ENABLE 端口联动

## 5. 外部控制

| 地址 | 说明 |
|------|------|
| `/enable` | 启停检测 |
| `/confidence` | 置信度阈值 |
| `/filter` | 类别索引（对应 Class 下拉） |

## 6. 使用说明

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

与 **Object Detection** 共用同一套端口与界面逻辑，但使用 **Hytch 定制 YOLO** 模型，仅检测两类目标：`xgpl_boat`、`gdtlj_truck`。适用于特定场景的船只 / 车辆识别，而非通用 COCO 80 类。

模型路径：`./plugins/Models/yolo26n-dection-hytch.onnx`

## 2. 端口说明

### 输入

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE | ImageData | 输入图像 |
| ENABLE | VariableData | 布尔启停；与界面「启动检测」、OSC `/enable` 同步 |

### 输出

| 端口 | 数据类型 | 说明 |
|------|----------|------|
| IMAGE 0 | ImageData | 带检测框与类别标签的图像（可关闭绘制以降 CPU） |
| RESULT | VariableData | 检测结果，结构同通用节点 |

## 3. RESULT 数据结构

| 字段 | 说明 |
|------|------|
| `default` | 当前帧检测到的目标数量 |
| `result` | 检测列表 |

`result` 中每一项包含：

| 字段 | 说明 |
|------|------|
| `class_id` | 类别 ID（0 或 1，见下表） |
| `class_name` | `xgpl_boat` 或 `gdtlj_truck` |
| `confidence` | 置信度（0–1） |

## 4. 界面说明

- **置信度阈值**：默认 `0.4`
- **检测对象**：仅 `xgpl_boat`、`gdtlj_truck` 两项
- **最大帧率 (FPS)**：默认 `15`，范围 1–30
- **绘制检测框**：关闭后只更新 RESULT
- **启动检测**：与 ENABLE 端口联动

## 5. 外部控制

| 地址 | 说明 |
|------|------|
| `/enable` | 启停检测 |
| `/confidence` | 置信度阈值 |
| `/filter` | 类别索引（0 = `xgpl_boat`，1 = `gdtlj_truck`） |

## 6. 使用说明

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
