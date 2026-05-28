# Spout Out 节点

## 1. 节点说明

将节点图中的图像通过 Spout 发送到其他应用程序。需先启用发送，再持续送入图像帧。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：要发送的画面。
- **ENABLE**（VariableData）：是否发送（true 开始，false 停止）。

### 输出

无输出端口。

## 3. 界面说明

- Spout 发送名称设置。
- 开始/停止发送按钮及状态显示。

## 4. 使用说明

1. 设置 Spout 名称（其他软件用此名接收）。
2. 将 IMAGE 连接图像源，ENABLE 置 true 或点「开始」。
3. 在接收端选择对应 Spout 源。

## 5. 示例

NDI In → 调色节点 → Spout Out（名称「MainStage」）→ Resolume Arena。
