#ifndef IMAGEPROVIDER_HPP
#define IMAGEPROVIDER_HPP

#include <QQuickImageProvider>
#include <QImage>
#include <QDateTime>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QWaitCondition>

// 图像合成任务类
class ImageCompositionTask : public QRunnable 
{
public:
    ImageCompositionTask(const QList<QVariantMap>& data, 
                        QSize size, 
                        QPoint offset,
                        QMutex* mutex,
                        QWaitCondition* condition)
        : m_data(data)
        , m_size(size)
        , m_offset(offset)
        , m_mutex(mutex)
        , m_condition(condition)
    {}

    void run() override {
        // 创建目标图像
        QImage result(m_size, QImage::Format_ARGB32);
        result.fill(Qt::transparent);

        // 在CPU上进行图像合成
        QPainter painter(&result);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        for (auto it = m_data.rbegin(); it != m_data.rend(); ++it) {
            QImage image = it->value("image").value<QImage>();
            if (!image.isNull()) {
                int posX = it->value("posX").toInt() - m_offset.x();
                int posY = it->value("posY").toInt() - m_offset.y();
                painter.drawImage(posX, posY, image);
            }
        }

        // 存储结果
        QMutexLocker locker(m_mutex);
        m_result = result;
        m_condition->wakeOne();
    }

    QImage getResult() const { return m_result; }

private:
    QList<QVariantMap> m_data;
    QSize m_size;
    QPoint m_offset;
    QImage m_result;
    QMutex* m_mutex;
    QWaitCondition* m_condition;
};

class TimelineImageProducer : public QQuickImageProvider
{
    Q_OBJECT
public:
    static TimelineImageProducer* instance() {
        static TimelineImageProducer* provider = new TimelineImageProducer();
        return provider;
    }

    TimelineImageProducer()
        : QQuickImageProvider(QQuickImageProvider::Image)
        , m_glContext(nullptr)
        , m_fbo(nullptr)
    {
        // 初始化OpenGL上下文
        m_glContext = new QOpenGLContext();
        m_glContext->create();

        // 创建离屏surface
        m_surface.create();
        
        // 初始化线程池
        m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
    }

    ~TimelineImageProducer() {
        delete m_fbo;
        delete m_glContext;
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        QMutexLocker locker(&m_mutex);
        
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
        QMutexLocker locker(&m_mutex);
        return m_currentImage;
    }

    QPoint getCurrentPosition() const {
        QMutexLocker locker(&m_mutex);
        return m_currentPos;
    }

    void updateImage(const QList<QVariantMap> &data) {
        if (data.isEmpty()) {
            QMutexLocker locker(&m_mutex);
            m_currentImage = QImage();
            m_currentPos = QPoint(0, 0);
            // 确保通知所有观察者图像已被清除
            emit imageChanged();
            return;
        }

        // 计算图像尺寸和偏移
        int maxWidth = 0;
        int maxHeight = 0;
        int minPosX = INT_MAX;
        int minPosY = INT_MAX;

        for (const QVariantMap &item : data) {
            int posX = item["posX"].toInt();
            int width = item["width"].toInt();
            int posY = item["posY"].toInt();
            int height = item["height"].toInt();

            if (width > 0 && height > 0) {
                maxWidth = qMax(maxWidth, posX + width);
                maxHeight = qMax(maxHeight, posY + height);
                minPosX = qMin(minPosX, posX);
                minPosY = qMin(minPosY, posY);
            }
        }

        if (maxWidth <= 0 || maxHeight <= 0) {
            QMutexLocker locker(&m_mutex);
            m_currentImage = QImage();
            m_currentPos = QPoint(0, 0);
            return;
        }

        // 调整尺寸
        maxWidth = maxWidth - minPosX;
        maxHeight = maxHeight - minPosY;

        // 创建合成任务
        QMutex mutex;
        QWaitCondition condition;
        auto task = new ImageCompositionTask(
            data, 
            QSize(maxWidth, maxHeight), 
            QPoint(minPosX, minPosY),
            &mutex,
            &condition
        );

        // 在线程池中执行合成任务
        mutex.lock();
        m_threadPool.start(task);
        condition.wait(&mutex);

        // 获取结果并更新
        {
            QMutexLocker locker(&m_mutex);
            m_currentImage = task->getResult();
            m_currentPos = QPoint(minPosX, minPosY);
            // 确保通知所有观察者图像已更新
            emit imageChanged();
        }

        mutex.unlock();
    }

signals:
    void imageChanged();

private:
    mutable QMutex m_mutex;
    QImage m_currentImage;
    QPoint m_currentPos;
    
    // OpenGL相关
    QOpenGLContext* m_glContext;
    QOffscreenSurface m_surface;
    QOpenGLFramebufferObject* m_fbo;
    
    // 线程池
    QThreadPool m_threadPool;
};

#endif // IMAGEPROVIDER_HPP