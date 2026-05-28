# Capture 节点

## 1. 节点说明

在触发时「抓拍」当前输入图像的一帧并输出，适合手动截图或由外部信号触发存图。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：待抓拍的视频/图像流。
- **TRIGGER**（VariableData）：为 `true` 时执行一次抓拍。

### 输出

- **IMAGE**（ImageData）：最近一次抓拍到的图像（保持到下次触发）。

## 3. 界面说明

内嵌 **Capture** 按钮：点击即抓拍一帧。也可通过外部命令 `/capture` 触发。

## 4. 使用说明

1. 将连续图像源接到 IMAGE。
2. 点击按钮，或向 TRIGGER 发送 true，或发送 `/capture` 命令。
3. 从 IMAGE 输出取静态帧做保存、分析或切换。

## 5. 示例

NDI In → Capture；热键节点 → TRIGGER → 抓拍帧 → File Load 下游处理。
