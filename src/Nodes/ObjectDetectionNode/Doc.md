# Object Detection 节点

## 1. 节点说明

本插件包含两个子节点：

| 节点名称 | 模型 | 检测类别 |
|----------|------|----------|
| **Object Detection** | `plugins/Models/yolo11n-Detection.onnx` | COCO 80 类 |
| **Object Detection (Hytch)** | `plugins/Models/yolo26n-dection-hytch.onnx` | `xgpl_boat`、`gdtlj_truck` |

两者端口与界面一致：可过滤类别与置信度，输出标注图与检测列表。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入图像。
- **ENABLE**（VariableData）：布尔启停；`true` 开始检测，`false` 停止（与界面 Enable 按钮、OSC `/enable` 同步）。

### 输出

- **IMAGE 0**（ImageData）：带检测框的图像。
- **RESULT**（VariableData）：检测结果。

## 3. 界面说明

- **Enable**：开关检测。
- **Confidence**：置信度阈值。
- **Class**：类别筛选。COCO 节点含 person、car 等；Hytch 节点为 `xgpl_boat`、`gdtlj_truck`。
- **最大帧率 (FPS)**：限制推理频率，默认 15，越低 CPU 占用越低。
- **绘制检测框**：关闭后只更新 RESULT 端口，不做图像标注与 IMAGE 输出刷新，可明显降 CPU。

## 4. 使用说明

1. 连接 IMAGE；通过 ENABLE 端口、界面按钮或 OSC 将检测设为开启。
2. 调整置信度与类别过滤。
3. 外部控制：`/enable`、`/confidence`、`/filter`。

## 5. 示例

监控画面 → Object Detection（仅 person）→ Condition → 触发报警节点。

## 6. COCO 80 类（Object Detection / YOLO 标准）

`Object Detection` 节点使用 COCO 数据集 80 类，**类别 ID（`class_id`）从 0 开始**，与 YOLO / Ultralytics 标准顺序一致。界面 **Class** 下拉框与下表对应；RESULT 端口中每条检测含 `class_id`、`class_name`。

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

**Hytch 自定义类别（Object Detection (Hytch)）**

| ID | 类别 |
|----|------|
| 0 | xgpl_boat |
| 1 | gdtlj_truck |
