# Human Segmentation Node
**PPHumanSeg 人物分割节点**

## 1. 节点说明

> ⚠️ **版本要求**：本节点为 **Advance（进阶版）** 功能，需有效授权后才能加载使用。

基于 [OpenCV Zoo PPHumanSeg](https://github.com/opencv/opencv_zoo/tree/main/models/human_segmentation_pphumanseg) 实现的人物分割节点。使用 `human_segmentation_pphumanseg_2023mar.onnx` 对输入画面进行语义分割，将人物与背景分离。

## 2. 端口说明

| 方向 | 默认数量 | 可编辑 | 最小 | 最大 | 新增端口类型 | 末端口差异 |
|------|----------|--------|------|------|--------------|------------|
| 输入 | 2 | 否 | 2 | 2 | — | 无 |
| 输出 | 2 | 否 | 2 | 2 | — | 无 |

> 本表对应 .flow 中该节点的 `input-count` / `output-count` / `port-editable` 三个字段：
> AI 搭图时连接索引不能超过当前端口数，可编辑节点的取值范围由"最小/最大"给出。



### 输入

- **IMAGE**（ImageData）：输入画面。
- **ENABLE**（VariableData）：启停分割。

### 输出

- **FOREGROUND**（ImageData）：前景图，保留人物区域像素，非人物区域为黑色。
- **BACKGROUND**（ImageData）：背景图，保留背景区域像素，人物区域为黑色。

## 3. 界面与属性

- 最大帧率（1–30 FPS）
- **使用 GPU (OpenCV CUDA DNN)**：默认开启；需 OpenCV 编译时启用 CUDA/cuDNN，否则自动回退 CPU
- 启停按钮

## 4. 外部控制（OSC 地址）

地址前缀：`/dataflow/{别名}/{节点ID}/…`

本节点未定义专属外部控制参数；框架会为公开属性自动生成默认 OSC 地址，外部系统可通过该地址读写对应属性。

## 5. 行为细节

将以下 ONNX 模型放入 `./plugins/Models/`：

- `human_segmentation_pphumanseg_2023mar.onnx`

下载地址：[human_segmentation_pphumanseg](https://github.com/opencv/opencv_zoo/tree/main/models/human_segmentation_pphumanseg)

## 6. 使用说明与典型接法

1. 连接视频或图像到 IMAGE，并通过 ENABLE 或面板启用分割。
2. 将 **FOREGROUND** 接到后续合成、抠像或特效节点。
3. 将 **BACKGROUND** 用于背景替换、模糊等处理。
4. 支持 OSC：`/enable`。

## 8. 示例

Camera → PPHumanSeg → Spout Out（前景），用于实时人物抠像与背景分离。

