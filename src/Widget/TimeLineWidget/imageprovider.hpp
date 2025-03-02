#ifndef IMAGEPROVIDER_HPP
#define IMAGEPROVIDER_HPP

#include <QQuickImageProvider>
#include <QImage>
#include <QDateTime>
#include <QDebug>

class ImageProvider : public QQuickImageProvider
{
public:
    static ImageProvider* instance() {
        static ImageProvider* provider = new ImageProvider();
        return provider;
    }

    ImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {}

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
       
        if (m_currentImage.isNull()) {
            return QImage();
        }

        if (size) {
            *size = m_currentImage.size();
        }

        if (requestedSize.isValid()) {
            return m_currentImage.scaled(requestedSize, Qt::KeepAspectRatio);
        }

        return m_currentImage;
    }

    QImage getCurrentImage() const {
        return m_currentImage;
    }

    void updateImage(const QList<QVariantMap> &data) {
        if (data.isEmpty()) {
            m_currentImage = QImage();  // 清空图像
            return;
        }
        
        // 计算所有图像混合后的最大尺寸
        int maxWidth = 0;
        int maxHeight = 0;
        
        for (const QVariantMap &item : data) {
            int posX = item["posX"].toInt();
            int width = item["width"].toInt();
            int posY = item["posY"].toInt(); 
            int height = item["height"].toInt();
            maxWidth = qMax(maxWidth, posX + width);
            maxHeight = qMax(maxHeight, posY + height);
        }
        
        // 如果没有有效尺寸，清空图像并返回
        if (maxWidth <= 0 || maxHeight <= 0) {
            m_currentImage = QImage();
            return;
        }
        
        // 创建新的空白图像
        m_currentImage = QImage(maxWidth, maxHeight, QImage::Format_ARGB32);
        m_currentImage.fill(Qt::transparent);
        
        // 将所有图像按层级顺序绘制到新图像上
        QPainter painter(&m_currentImage);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        
        // 按层级排序
        QMap<int, QVariantMap> layeredImages;
        for (const QVariantMap &item : data) {
            int layer = -item["layer"].toInt(); // 取负值使得layer越大的排在前面（底层）
            layeredImages.insert(layer, item);
        }
        
        // 从底层到顶层依次绘制
        for (auto it = layeredImages.begin(); it != layeredImages.end(); ++it) {
            const QVariantMap &item = it.value();
            QImage image = item["image"].value<QImage>();
            if (!image.isNull()) {
                int posX = item["posX"].toInt();
                int posY = item["posY"].toInt();
                painter.drawImage(posX, posY, image);
            }
        }
        
        painter.end();
    }

private:
    QImage m_currentImage;
}; 
#endif // IMAGEPROVIDER_HPP