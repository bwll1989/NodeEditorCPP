# Scatter Series

**DataVisualNode 节点**

## 1. 节点说明

DataVisualNode 插件提供 **3D 散点**可视化节点，将输入向量映射为空间坐标并在节点面板内实时显示。无输出端口，仅作监视与调试。

| 变体 | 节点库名称 | 行为 |
|------|------------|------|
| ScatterSingle | ScatterSingle | 每次更新后**移动单个点**（`updatePoint`） |
| ScatterSeries | ScatterSeries | 每次更新后**追加一个新点**（`appendPoint`），形成轨迹 |

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | XYZ | VariableData | 位置向量；从 `default` 按需截取/补 0 得到 `[x, y, z]`（短向量缺维补 0，超长截断） |

### 输出

无。

## 3. 界面说明

节点主区域为内嵌 **3D 散点 QML 视图**，随输入实时旋转/缩放查看（具体交互以界面为准）。无额外按钮；图数据当前**不写入**工程保存（`save`/`load` 为空结构）。

## 4. 使用说明

1. 选择 ScatterSingle（单点跟随）或 ScatterSeries（留轨迹）。
2. 将三维向量源（如 Vec Source Size=3、LocationProto POS）接到 **XYZ**。
3. 用于观察 LFO、传感器或脚本输出的三维关系。

ScatterSeries 长时间运行会累积大量点，注意性能与清晰度。

## 5. 示例

**单点跟踪：** ScatterSingle，接陀螺仪或模拟向量，观察当前姿态点。  
**轨迹记录：** ScatterSeries，接无人机或鼠标三维坐标，回放运动路径。

---

# Scatter Single

与 `Scatter Series` 同属本插件；端口与用法见上文。
