# DataVisualNode 使用说明

DataVisualNode 插件包含两个可视化节点：ScatterSingle 与 ScatterSeries。它们用于把输入的三维坐标（X/Y/Z）显示为 3D 散点。

## ScatterSingle
### 用途
显示“单个点”。当 X/Y/Z 任意输入变化时，更新该点的位置。

### 端口
- 输入（3）
  - VALUE_X：X 坐标（VariableData，可转为数值）
  - VALUE_Y：Y 坐标（VariableData，可转为数值）
  - VALUE_Z：Z 坐标（VariableData，可转为数值）
- 输出：无

### 使用步骤
1. 将三个数值源分别连接到 VALUE_X / VALUE_Y / VALUE_Z。
2. 运行流程后在界面中观察点位置变化。

## ScatterSeries
### 用途
显示“点序列”。当 X/Y/Z 任意输入变化时，会向序列追加一个新点（形成轨迹/散点云）。

### 端口
- 输入（3）
  - VALUE_X：X 坐标（VariableData，可转为数值）
  - VALUE_Y：Y 坐标（VariableData，可转为数值）
  - VALUE_Z：Z 坐标（VariableData，可转为数值）
- 输出：无

### 使用步骤
1. 将三个数值源分别连接到 VALUE_X / VALUE_Y / VALUE_Z（可用 LFO/时间轴/传感器数据等）。
2. 运行后会持续累积点，形成轨迹/散点分布。

## 注意事项
- 该插件节点没有输出端口，主要用于可视化观察与调试。
- 输入为空时不会更新点。
