#include "ImageShowModel.hpp"

#include <QtNodes/NodeDelegateModelRegistry>

#include <QLabel>
#include <QPainter>
#include <QStyleOption>
#include <QtCore/QDir>
#include <QtCore/QEvent>

namespace
{
    class ImageFitWidget final : public QWidget
    {
    public:
        /**
         * @brief 构造一个用于显示图像的控件，绘制时自动按控件大小缩放
         */
        explicit ImageFitWidget(QWidget* parent = nullptr)
            : QWidget(parent)
        {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumSize(0, 0);
        }

        /**
         * @brief 设置当前显示的图像（触发重绘）
         */
        void setImage(const QImage& img)
        {
            m_image = img;
            update();
        }

        /**
         * @brief 返回更小的 sizeHint，避免图像原始尺寸撑大父布局
         */
        QSize sizeHint() const override { return QSize(0, 0); }

        /**
         * @brief 返回更小的 minimumSizeHint，避免图像原始尺寸撑大父布局
         */
        QSize minimumSizeHint() const override { return QSize(0, 0); }

    protected:
        /**
         * @brief 按控件大小绘制图像（默认保持宽高比居中）
         */
        void paintEvent(QPaintEvent* e) override
        {
            Q_UNUSED(e);

            QStyleOption opt;
            opt.initFrom(this);
            QPainter p(this);
            style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

            if (m_image.isNull()) {
                p.setPen(palette().text().color());
                p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Image will appear here"));
                return;
            }

            const QSize targetSize = rect().size();
            if (targetSize.width() <= 0 || targetSize.height() <= 0) {
                return;
            }

            QSize scaledSize = m_image.size();
            scaledSize.scale(targetSize, Qt::KeepAspectRatio);
            const QRect drawRect(QPoint((targetSize.width() - scaledSize.width()) / 2,
                                        (targetSize.height() - scaledSize.height()) / 2),
                                 scaledSize);

            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            p.drawImage(drawRect, m_image);
        }

    private:
        QImage m_image;
    };
}

using namespace Nodes;
using namespace NodeDataTypes;
/**
 * @brief 构造函数，初始化图像显示标签
 */
ImageShowModel::ImageShowModel()
    : _view(new ImageFitWidget()) {
    InPortCount =1;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Image Display";
    WidgetEmbeddable=false;
    Resizable=false;
    QFont f = _view->font();
    f.setBold(true);
    f.setItalic(true);
    _view->setFont(f);
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
    auto* view = static_cast<ImageFitWidget*>(_view);
    if (m_outData) {
        const QImage img = m_outData->image();
        view->setImage(img);
    } else {
        view->setImage(QImage());
    }

    Q_EMIT dataUpdated(0);
}
