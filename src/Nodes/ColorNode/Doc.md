# ImageConstNode 帮助文档

## 节点功能
ImageConstNode 是一个用于生成纯色图像常量的节点。它提供以下功能：

1. 通过输入端口或界面设置图像的宽度、高度和颜色（RGBA）。
2. 实时预览当前颜色（display 区域显示纯色块）。
3. 输出指定尺寸和颜色的 QImage，可用于下游图像处理节点。
4. 支持保存和加载节点参数（宽、高、RGBA）。
5. 支持端口输入和界面输入联动，任意方式修改都会实时更新输出。
6. 图像生成过程异步执行，保证界面流畅。

## 输入端口
- WIDTH (PortIndex 0)：图像宽度（整数类型）。
- HEIGHT (PortIndex 1)：图像高度（整数类型）。
- RED (PortIndex 2)：红色分量（0-255，整数类型）。
- GREEN (PortIndex 3)：绿色分量（0-255，整数类型）。
- BLUE (PortIndex 4)：蓝色分量（0-255，整数类型）。
- ALPHA (PortIndex 5)：透明度分量（0-255，整数类型）。

## 输出端口
- OUTPUT (PortIndex 0)：类型为 ImageData，输出指定尺寸和颜色的 QImage。

## 节点界面
- widthEdit：输入框，设置图像宽度，默认100。
- heightEdit：输入框，设置图像高度，默认100。
- colorRedEdit/colorGreenEdit/colorBlueEdit/colorAlphaEdit：输入框，设置 RGBA 分量，默认255。
- display：颜色预览区域，实时显示当前颜色。

## 保存与加载
- 节点支持保存和加载参数（宽、高、RGBA），关闭后再次打开会恢复上次设置。

## 使用示例
### 生成纯色图像
1. 添加 ImageConstNode 到流程图。
2. 在界面输入框设置所需的宽度、高度和颜色分量。
3. 颜色预览区会实时显示当前颜色。
4. 输出端口可连接到下游图像处理节点（如 ImageInfoNode、ImageShowNode 等）。

### 通过端口动态控制
1. 用整数源节点连接到 WIDTH/HEIGHT/RED/GREEN/BLUE/ALPHA 输入端口，实现动态调整尺寸和颜色。
2. 任意输入变化都会实时更新输出图像和颜色预览。

## 注意事项
1. 所有输入参数必须为整数，颜色分量范围0-255。
2. 预览区仅显示当前颜色，不显示完整图像内容。
3. 图像生成为异步执行，保证界面流畅。
4. 节点参数支持保存和加载。
5. WidgetEmbeddable=false，节点界面不可嵌入其它控件。

## 技术细节
- 节点使用 QImage 生成纯色图像，输出为 ImageData。
- 颜色预览使用 QLabel+QPixmap 实现。
- 图像生成采用 QtConcurrent 异步执行，主线程仅负责界面更新。
- 支持端口输入和界面输入联动。
- 支持 QJsonObject 保存/加载参数。