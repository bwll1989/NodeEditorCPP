# Threshold 节点

## 1. 节点说明

对图像做二值化（阈值）处理。彩色输入会先转为灰度。支持多种 OpenCV 阈值模式。

## 2. 端口说明

### 输入

- **IMAGE**（ImageData）：输入图像。
- **THRESHOLD**（VariableData）：阈值 `thresh`（0～255）。
- **MAXVAL**（VariableData）：二值化后的最大值 `maxval`。
- **METHOD**（VariableData）：方法索引（见下表）。

### 输出

- **OUTPUT 0**（ImageData）：二值化后的图像。

**METHOD 索引：**

| 索引 | 含义 |
|------|------|
| 0 | Binary |
| 1 | BinaryInv |
| 2 | Trunc |
| 3 | ToZero |
| 4 | ToZeroInv |

## 3. 界面说明

无内嵌面板；通过属性 `thresh`、`maxval`、`method` 或外部地址 `/thresh`、`/maxval`、`/method` 调节。

## 4. 使用说明

1. 连接 IMAGE。
2. 设置阈值与模式（属性或端口）。
3. 将输出接到后续识别或显示节点。

## 5. 示例

灰度摄像头 → Threshold（thresh=128）→ 轮廓检测预处理。
