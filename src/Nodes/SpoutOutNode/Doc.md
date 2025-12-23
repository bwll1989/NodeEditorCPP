# SpoutOutNode 帮助文档

## 功能概述

**注意**: 当前版本的 `SpoutOutNode` 实现为 **Spout 接收器 (Receiver)**，其底层基于 `SpoutLibrary` API。虽然节点名称为 "Out"，但其目前的实际行为是从 Spout 接收图像并输出到节点网络（类似于 `SpoutInNode`，但实现方式不同）。

### 主要功能

- **实时图像接收**: 基于 `SpoutLibrary` 接收 Spout 图像数据。
- **独立线程**: 使用 `SpoutSendThread` (实际为接收逻辑) 在独立线程中处理 OpenGL 上下文和数据传输。
- **高性能**: 利用 OpenGL/DirectX 互操作性进行纹理共享。

## 输入输出端口

### 输入端口

- **无**: 当前版本没有输入端口。
  - *注意*: 标准的 Spout 输出节点通常应该有一个图像输入端口，但当前实现不接受输入。

### 输出端口

- **端口 0**: `ImageData`
  - **功能**: 输出图像数据。
  - **描述**: 输出从 Spout 接收到的图像帧。

## 节点界面

- **发送器选择**: 允许选择外部 Spout 发送源。
- **连接控制**: 开始/停止接收。
- **状态显示**: 显示连接状态。

## 技术细节

- **底层实现**: 直接使用 `SpoutLibrary.dll` 接口，而非 `SpoutReceiver` 封装类。
- **OpenGL 上下文**: 节点内部维护独立的 `QOpenGLContext` 和 `QOffscreenSurface` 以处理纹理共享。
- **图像格式**: 内部处理 BGRA 格式并转换为 RGB 输出。

## 差异说明 (vs SpoutInNode)

- **SpoutInNode**: 使用 `SpoutReceiver` 类，具有控制输入端口 (VariableData)。
- **SpoutOutNode**: 使用 `SpoutLibrary` API，无输入端口，完全独立的线程模型。

*文档更新日期: 2025-12-21*
