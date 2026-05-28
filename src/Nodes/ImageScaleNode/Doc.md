# Scale Image 节点

## 1. 节点说明

将输入图像缩放到指定宽高。可通过节点属性、输入端口或外部命令设置目标尺寸。

## 2. 端口说明

### 输入

- **Image**（ImageData）：原始图像。
- **SIZE**（VariableData）：一次性设置宽高（QSize）。
- **Width**（VariableData）：目标宽度（像素）。
- **Height**（VariableData）：目标高度（像素）。

### 输出

- **Image**（ImageData）：缩放后的图像。宽高无效或输入为空时无输出。

## 3. 界面说明

无内嵌面板；通过属性面板设置 `width`、`height`，或通过 `/Width`、`/Height` 外部地址控制。

## 4. 使用说明

1. 连接图像到 Image 端口。
2. 在属性中填写目标宽高，或从 Size Source / Rect 等节点接入 Width、Height。
3. 也可同时用 SIZE 端口传入 QSize 覆盖宽高。

## 5. 示例

视频帧 → Scale Image（width=1920, height=1080）→ NDI Out，统一输出分辨率。
