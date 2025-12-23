#include "ImageShowModel.hpp"


#include <QtNodes/NodeDelegateModelRegistry>

#include <QtCore/QDir>
#include <QtCore/QEvent>
using namespace Nodes;
using namespace NodeDataTypes;
/**
 * @brief 构造函数，初始化图像显示标签
 */
ImageShowModel::ImageShowModel()
    : _label(new QLabel("Image will appear here")) {
    InPortCount =1;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Image Display";
    WidgetEmbeddable=true;
    Resizable=false;
    _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    _label->setScaledContents(true);
    _label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    QFont f = _label->font();
    f.setBold(true);
    f.setItalic(true);
    _label->setFont(f);
    _label->setMinimumSize(200, 200);
}

/**
 * @brief 返回端口数据类型
 */
QtNodes::NodeDataType ImageShowModel::dataType(QtNodes::PortType const, QtNodes::PortIndex const) const {
    return ImageData().type();
}

/**
 * @brief 输出当前缓存的图像数据
 */
std::shared_ptr<QtNodes::NodeData> ImageShowModel::outData(QtNodes::PortIndex) {
    return m_outData;
}

/**
 * @brief 设置输入图像数据并刷新显示
 * 使用 QLabel 的缩放功能，避免每帧创建缩放后的 QPixmap 导致内存增长
 */
void ImageShowModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const) {
    m_outData = std::dynamic_pointer_cast<ImageData>(nodeData);
    if (m_outData) {
        const QImage img = m_outData->image();
        if (!img.isNull()) {
            _label->setPixmap(QPixmap::fromImage(img));
        }
    } else {
        _label->setPixmap(QPixmap());
    }

    Q_EMIT dataUpdated(0);
}
