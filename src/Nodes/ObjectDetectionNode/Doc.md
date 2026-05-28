# Object Detection 节点

## 1. 节点说明

使用 YOLO11n 检测图像中的物体（COCO 80 类），可过滤类别与置信度，输出标注图与检测列表。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入图像。

### 输出

- **IMAGE 0**（ImageData）：带检测框的图像。
- **RESULT**（VariableData）：检测结果。

## 3. 界面说明

- **Enable**：开关检测。
- **Confidence**：置信度阈值。
- **Class**：类别筛选（下拉，含 person、car 等 COCO 类）。

## 4. 使用说明

1. 勾选 Enable 并连接 IMAGE。
2. 调整置信度与类别过滤。
3. 外部控制：`/enable`、`/confidence`、`/filter`。

## 5. 示例

监控画面 → Object Detection（仅 person）→ Condition → 触发报警节点。
