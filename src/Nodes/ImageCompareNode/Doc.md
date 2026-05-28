# Image Compare 节点

## 1. 节点说明

比较两张图像的相似程度，输出一个数值（VariableData）。尺寸不一致时会缩放到较小的一边再比较。

## 2. 端口说明

### 输入

- **IMAGE 0**（ImageData）：参考图或 A 图。
- **IMAGE 1**（ImageData）：对比图或 B 图。

### 输出

- **OUTPUT 0**（VariableData）：相似度/差异指标（浮点数，含义取决于所选方法）。

## 3. 界面说明

下拉框选择比较方法：

- **(均方差) MSE**
- **(结构相似性指数) SSIM**
- **(峰值信噪比) PSNR**
- **(图片相似度) 直方图**

## 4. 使用说明

1. 将两路图像接到 IMAGE 0、IMAGE 1。
2. 选择比较算法。
3. 用输出数值驱动 Condition、Switch 或显示。

可通过 `/method` 外部切换算法。

## 5. 示例

设计稿与现场画面 → Image Compare（SSIM）→ Condition（低于 0.9 报警）。
