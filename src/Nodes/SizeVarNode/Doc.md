# Size Source 节点

## 1. 节点说明

输出一个尺寸（QSize），用于驱动缩放、布局等需要宽高的节点。

## 2. 端口说明

### 输入

- **Width**（VariableData）：宽度。
- **Height**（VariableData）：高度。

### 输出

- **SIZE**（VariableData）：合并后的 QSize。

## 3. 界面说明

无内嵌面板；通过属性 `width`、`height` 或 `/width`、`/height` 设置。

## 4. 使用说明

1. 设置或通过端口传入宽高。
2. 将 SIZE 接到 Scale Image 的 SIZE 端口，或供其他逻辑读取。

## 5. 示例

Size Source（1920×1080）→ Scale Image / SIZE，统一下游分辨率。
