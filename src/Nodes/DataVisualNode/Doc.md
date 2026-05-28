# DataVisualNode 节点

## 1. 节点说明

DataVisualNode 插件提供 **3D 散点**可视化节点，将三路数值映射为空间坐标并在节点面板内实时显示。无输出端口，仅作监视与调试。

| 变体 | 节点库名称 | 行为 |
|------|------------|------|
| ScatterSingle | ScatterSingle | 每次更新 X/Y/Z 后**移动单个点**（`updatePoint`） |
| ScatterSeries | ScatterSeries | 每次更新 X/Y/Z 后**追加一个新点**（`appendPoint`），形成轨迹 |

## 2. 端口说明

### 输入

| 端口 | 名称 | 数据类型 | 说明 |
|------|------|----------|------|
| 0 | VALUE_X | VariableData | X 坐标（按 double 解析） |
| 1 | VALUE_Y | VariableData | Y 坐标 |
| 2 | VALUE_Z | VariableData | Z 坐标 |

任一端口更新都会用当前缓存的 X/Y/Z 刷新图形（未更新的轴保留上次值）。

### 输出

无。

## 3. 界面说明

节点主区域为内嵌 **3D 散点 QML 视图**，随输入实时旋转/缩放查看（具体交互以界面为准）。无额外按钮；图数据当前**不写入**工程保存（`save`/`load` 为空结构）。

## 4. 使用说明

1. 选择 ScatterSingle（单点跟随）或 ScatterSeries（留轨迹）。
2. 将三个数值源接到 VALUE_X、VALUE_Y、VALUE_Z。
3. 用于观察 LFO、传感器或脚本输出的三维关系。

ScatterSeries 长时间运行会累积大量点，注意性能与清晰度。

## 5. 示例

**单点跟踪：** ScatterSingle，三路接陀螺仪或模拟数据，观察当前姿态点。  
**轨迹记录：** ScatterSeries，接无人机或鼠标三维坐标，回放运动路径。
