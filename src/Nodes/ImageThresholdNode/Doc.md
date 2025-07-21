# ImageThresholdNode 帮助文档

## 节点功能
实现基于OpenCV的图像阈值处理，支持以下操作：
1. 自动将彩色图像转换为灰度图像
2. 支持5种阈值处理类型：
  - 二进制阈值化 (THRESH_BINARY)
  - 反二进制阈值化 (THRESH_BINARY_INV)
  - 截断阈值化 (THRESH_TRUNC)
  - 阈值化为0 (THRESH_TOZERO)
  - 反阈值化为0 (THRESH_TOZERO_INV)
3. 支持动态调整阈值和最大值参数
4. 实时预览处理结果

## 输入端口
- IMAGE (PortIndex 0)：输入图像，支持图像类型数据
- THRESHOLD (PortIndex 1)：阈值参数，支持整数数据
- MAX_VALUE (PortIndex 2)：最大值参数，支持整数数据
- THRESHOLD_TYPE (PortIndex 3)：阈值处理类型参数，支持整数数据

## 输出端口
- IMAGE (PortIndex 0)：输出图像，支持图像类型数据

## 节点界面
- 阈值参数输入框
- 最大值参数输入框
- 阈值处理类型下拉列表

## 保存与加载
- 节点支持保存和加载参数。

## 使用示例
### 基本消息配置



## 技术细节


## 注意事项
