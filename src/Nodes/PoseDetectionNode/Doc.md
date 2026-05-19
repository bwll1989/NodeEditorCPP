# PoseDetectionNode 使用说明

## 用途
对输入图像进行人体姿态检测，并输出：
- 叠加关键点/骨架后的图像
- 结构化检测结果（VariableData）

## 端口
### 输入
- IMAGE（ImageData）：输入图像

### 输出
- IMAGE 0（ImageData）：姿态标注后的图像
- RESULT（VariableData）：检测结果

## 输出字段（RESULT）
RESULT 为一个键值表，常用字段：
- count（int）：检测到的人数
- width（int）：输入图像宽
- height（int）：输入图像高
- detections（数组）：每项为一个人体检测结果
  - score（double）：该人的置信度
  - box（数组）：[x1, y1, x2, y2]
  - keypoints（数组）：17 个关键点，每项包含
    - x（double）
    - y（double）
    - conf（double）

## 使用步骤
1. 将图像源连接到 IMAGE。
2. 从 IMAGE 0 获取叠加了关键点/骨架的画面。
3. 从 RESULT 读取 keypoints，用于下游逻辑/可视化/控制。

## 注意事项
- 当输入为空时不会产生有效输出。
- 输出 keypoints 数组固定为 17 项；低置信度关键点可能不可用（坐标可能为负值）。 
