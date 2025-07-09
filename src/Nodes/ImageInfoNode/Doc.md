# ImageInfoNode 帮助文档
## 节点功能
ImageInfoNode 是一个用于显示和分析图像信息的节点。它提供了以下功能：

1. 图像属性可视化 ：以树状结构显示图像的各种属性，如尺寸、格式、是否有Alpha通道等。
2. 图像格式识别 ：自动识别并显示图像的格式（如RGB32、ARGB32、RGB888等）。
3. 图像数据透传 ：将输入的图像数据原样传递到输出端口，便于在图像处理流程中插入检查点。
4. 属性数据输出 ：将图像属性作为VariableData输出，可用于后续节点的条件判断或数据处理。
5. 实时更新 ：当输入图像变化时，属性显示会自动更新。
## 输入端口
- INPUT (PortIndex 0) ：接收类型为 ImageData 的图像数据。
## 输出端口
- OUTPUT 0 ：类型为 ImageData ，输出与输入相同的图像数据。
- OUTPUT 1 ：类型为 VariableData ，包含图像的各种属性信息，具体包括：
  - isNull ：图像是否为空
  - isGrayScale ：图像是否为灰度图
  - hasAlpha ：图像是否有Alpha通道
  - Format ：图像格式（如RGB32、ARGB32等）
  - Width ：图像宽度
  - Height ：图像高度
  - Size ：图像尺寸（QSize类型）
  - Rect ：图像矩形区域（QRect类型）
## 节点界面
节点嵌入了一个基于QPropertyBrowser的属性树视图，用于显示图像的各种属性。属性以只读方式显示，用户可以查看但不能修改这些值。界面会根据输入图像的变化自动更新显示的属性值。

## 使用示例
### 基本图像信息查看
1. 将一个图像源节点（如ImageLoadNode）连接到ImageInfoNode的输入端口。
2. ImageInfoNode会自动显示图像的各种属性，如尺寸、格式等。
3. 可以通过查看节点界面中的属性树来了解图像的详细信息。
### 图像处理流程中的检查点
1. 在图像处理流程的关键位置插入ImageInfoNode。
2. 通过查看节点显示的属性，可以验证图像处理的中间结果是否符合预期。
3. 利用OUTPUT 0端口将图像传递给下一个处理节点，不影响原有的处理流程。
### 基于图像属性的条件处理
1. 将ImageInfoNode的OUTPUT 1（VariableData）连接到条件处理节点（如SwitchNode）。
2. 根据图像的特定属性（如尺寸、是否有Alpha通道等）来决定后续的处理流程。
3. 例如，可以根据图像是否为灰度图来选择不同的处理算法。
### 图像格式分析
1. 使用ImageInfoNode检查图像的格式。
2. 根据Format属性的值，了解图像的像素格式（如RGB32、ARGB32等）。
3. 这对于需要特定格式的图像处理算法非常有用。
## 注意事项
1. 输入数据类型 ：节点只接受ImageData类型的输入，其他类型的数据将被忽略。
2. 空图像处理 ：当输入为空或无效图像时，节点会显示isNull=true，其他属性将不可用。
3. 性能考虑 ：对于非常大的图像，属性计算可能会消耗一定的性能，但通常影响很小。
4. 实时更新 ：节点会在输入图像变化时自动更新属性显示，无需手动刷新。
5. 数据透传 ：节点不会修改原始图像数据，OUTPUT 0端口输出的图像与输入完全相同。
6. 界面调整 ：节点界面的大小可以通过拖动节点边缘来调整，以便更好地查看属性树。
## 技术细节
- QPropertyBrowser ：节点使用QPropertyBrowser来显示图像属性，支持多种数据类型的显示。
- QVariantMap ：图像属性以QVariantMap的形式存储和传递，便于扩展和访问。
- buildPropertiesFromMap ：节点使用此方法将图像属性转换为属性树显示。
- formatToString ：节点内部实现了一个将QImage::Format枚举值转换为可读字符串的方法。
- dataUpdated信号 ：当属性更新时，节点会发出dataUpdated信号通知连接的下游节点。
- Resizable ：节点支持调整大小，便于查看更多属性信息。
- WidgetEmbeddable ：节点支持嵌入自定义控件（QPropertyBrowser）。