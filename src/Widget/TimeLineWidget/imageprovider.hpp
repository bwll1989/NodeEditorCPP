#ifndef IMAGEPROVIDER_HPP
#define IMAGEPROVIDER_HPP

#include <QQuickImageProvider>
#include <QImage>
#include <QDateTime>

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

    void updateImage(const QImage &image) {
        m_currentImage = image;
    }

private:
    QImage m_currentImage;
}; 
#endif // IMAGEPROVIDER_HPP