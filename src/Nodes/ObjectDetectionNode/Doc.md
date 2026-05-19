# ObjectDetectionNode 使用说明

## 用途
对输入图像进行目标检测，并输出：
- 叠加检测框/标签后的图像
- 检测结果列表（VariableData）

## 端口
### 输入
- IMAGE（ImageData）：输入图像

### 输出
- IMAGE 0（ImageData）：检测结果图（带框与标签）
- RESULT（VariableData）：检测结果数据

## 参数/界面
- Enable：启用/禁用检测
- Confidence：置信度阈值（数值越高越严格）
- Class Filter：类别过滤（选择后只检测该类别；若选择“全部/无过滤”，则检测所有类别）

## 外部控制（可选）
- /enable（bool）：启用/禁用
- /confidence（double）：设置置信度阈值
- /filter（int）：设置类别索引（与界面下拉一致）

## 输出字段（RESULT）
RESULT 为一个键值表，常用字段：
- default：数组，每项为一个检测结果（QVariantMap）
  - class_id（int）
  - class_name（string）
  - confidence（double，0~1）
- total_detections（int）：检测数量

## 使用步骤
1. 将图像源连接到 IMAGE。
2. 打开 Enable，调整 Confidence（必要时选择 Class Filter）。
3. 从 IMAGE 0 获取叠加框的图像；从 RESULT 获取结构化检测结果，供下游逻辑/显示使用。

## 注意事项
- 未启用或无输入时，RESULT 会输出空结果（total_detections=0）。
- 类别索引与模型/界面下拉保持一致；不同模型可能会导致类别列表变化。 
