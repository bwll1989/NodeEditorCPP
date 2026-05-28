# NDI In 节点

## 1. 节点说明

从局域网接收 NDI 视频流并输出为图像。可扫描并选择 NDI 源，支持远程控制启停与切源。

## 2. 端口说明

### 输入

- **SOURCE**（VariableData）：NDI 源名称（字符串）。
- **ENABLE**（VariableData）：开始/停止接收（布尔）。

### 输出

- **IMAGE**（ImageData）：当前 NDI 视频帧。

## 3. 界面说明

- NDI 源列表与刷新按钮。
- 开始/停止接收按钮。
- 连接状态显示。

## 4. 使用说明

1. 点击刷新，从列表选择 NDI 源。
2. 开始接收后连接下游图像节点。
3. 可用 SOURCE、ENABLE 端口或 `/sourceName`、`/enable`、`/refresh` 外部控制。

## 5. 示例

现场 NDI 摄像机 → NDI In → Pose Detection → 大屏预览。
