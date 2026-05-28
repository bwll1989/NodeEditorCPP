# Spout In 节点

## 1. 节点说明

从本机 Spout 发送端接收实时画面，并输出为图像流。适用于与 Resolume、TouchDesigner 等软件互通。

## 2. 端口说明

### 输入

- **端口 0**（VariableData）：发送端名称（字符串），可远程指定源。
- **端口 1**（VariableData）：启用/停止接收（布尔）。

### 输出

- **输出 0**（ImageData）：当前 Spout 帧。

## 3. 界面说明

- 发送端下拉列表：选择 Spout 源。
- 开始/停止按钮：控制是否接收。
- 连接状态指示。

## 4. 使用说明

1. 确保发送端软件已开启 Spout 输出。
2. 在节点中选择发送端并点击开始。
3. 将图像输出接到处理链或 Spout/NDI 转发节点。

外部控制：`/source`（名称）、`/enable`（布尔）。

## 5. 示例

TouchDesigner Spout Out → Spout In → 特效链 → Spout Out → 投影软件。
