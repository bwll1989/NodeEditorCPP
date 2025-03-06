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
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

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
        
        // 确保OpenGL上下文是当前的
        m_glContext->makeCurrent(&m_surface);

        // 初始化着色器程序
        initShaders();
        // 释放上下文
        m_glContext->doneCurrent();
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
            // QMutexLocker locker(&m_mutex);
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
        QSize m_size(maxWidth, maxHeight);
        m_currentPos = QPoint(minPosX, minPosY);
       
        // 确保OpenGL上下文是当前的
        m_glContext->makeCurrent(&m_surface);

        // 创建或调整FBO大小
        if (!m_fbo || m_fbo->size() != m_size) {
            delete m_fbo;
            m_fbo = new QOpenGLFramebufferObject(m_size);
        }

        // 绑定FBO准备渲染
        m_fbo->bind();
        
        QOpenGLFunctions *f = m_glContext->functions();
        f->glViewport(0, 0, m_size.width(), m_size.height());
        f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        f->glClear(GL_COLOR_BUFFER_BIT);

        // 启用混合
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // 从后向前渲染所有图层
        QList<QOpenGLTexture*> textures;
        for (auto it = data.rbegin(); it != data.rend(); ++it) {
            // 检查是否有纹理对象
            QOpenGLTexture* texture = it->value("texture").value<QOpenGLTexture*>();
            if (!texture) {
 
                // 如果没有纹理，尝试获取图像
                QImage image = it->value("image").value<QImage>();
                if (!image.isNull()) {
                    // 创建并设置纹理
                    texture = new QOpenGLTexture(image);
                    texture->setMinificationFilter(QOpenGLTexture::Linear);
                    texture->setMagnificationFilter(QOpenGLTexture::Linear);
                }
            }

            if (texture) {
                // 计算绘制位置
                int posX = it->value("posX").toInt() - m_currentPos.x();
                int posY = it->value("posY").toInt() - m_currentPos.y();
                
                // 使用着色器程序
                if (!m_shaderProgram.bind()) {
                    qDebug() << "Failed to bind shader program";
                    continue;
                }

                // 设置投影矩阵
                QMatrix4x4 projection;
                projection.ortho(0, m_size.width(), m_size.height(), 0, -1, 1);
                m_shaderProgram.setUniformValue("projection", projection);

                // 设置模型视图矩阵
                QMatrix4x4 modelView;
                modelView.translate(posX, posY);
                modelView.scale(texture->width(), texture->height());
                m_shaderProgram.setUniformValue("modelView", modelView);

                // 绑定纹理
                texture->bind();
                m_shaderProgram.setUniformValue("texture", 0);

                // 设置顶点数据
                static const GLfloat vertices[] = {
                    0.0f, 0.0f,  // 左上
                    1.0f, 0.0f,  // 右上
                    0.0f, 1.0f,  // 左下
                    1.0f, 1.0f   // 右下
                };

                static const GLfloat texCoords[] = {
                    0.0f, 0.0f,  // 左上
                    1.0f, 0.0f,  // 右上
                    0.0f, 1.0f,  // 左下
                    1.0f, 1.0f   // 右下
                };

                // 设置顶点属性
                m_shaderProgram.enableAttributeArray("vertex");
                m_shaderProgram.enableAttributeArray("texCoord");
                m_shaderProgram.setAttributeArray("vertex", vertices, 2);
                m_shaderProgram.setAttributeArray("texCoord", texCoords, 2);

                // 绘制四边形
                f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                // 清理
                m_shaderProgram.disableAttributeArray("vertex");
                m_shaderProgram.disableAttributeArray("texCoord");
                texture->release();
                m_shaderProgram.release();

                // 如果是从图像创建的临时纹理，需要清理
                if (!it->contains("texture")) {
                    texture->destroy();
                    delete texture;
                }
            }
        }

        // 从FBO读取结果
        QImage result = m_fbo->toImage();

        // 清理纹理
        for (auto texture : textures) {
            texture->destroy();
            delete texture;
        }

        m_fbo->release();
        m_glContext->doneCurrent();

        // 更新当前图像
        {
            QMutexLocker locker(&m_mutex);
            m_currentImage = result;
            emit imageChanged();
        }
    }

signals:
    void imageChanged();

private:
    void initShaders()
    {
        // 顶点着色器
        const char *vertexShaderSource = R"(
            attribute vec2 vertex;
            attribute vec2 texCoord;
            uniform mat4 projection;
            uniform mat4 modelView;
            varying vec2 texCoordVarying;
            void main() {
                gl_Position = projection * modelView * vec4(vertex, 0.0, 1.0);
                texCoordVarying = texCoord;
            }
        )";

        // 片段着色器
        const char *fragmentShaderSource = R"(
            uniform sampler2D texture;
            varying vec2 texCoordVarying;
            void main() {
                gl_FragColor = texture2D(texture, texCoordVarying);
            }
        )";

        // 编译和链接着色器
        if (!m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
            qDebug() << "Failed to compile vertex shader";
            return;
        }

        if (!m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
            qDebug() << "Failed to compile fragment shader";
            return;
        }

        if (!m_shaderProgram.link()) {
            qDebug() << "Failed to link shader program";
            return;
        }
    }

    mutable QMutex m_mutex;
    QImage m_currentImage;
    QPoint m_currentPos;
    
    // OpenGL相关
    QOpenGLContext* m_glContext;
    QOffscreenSurface m_surface;
    QOpenGLFramebufferObject* m_fbo;
    QOpenGLShaderProgram m_shaderProgram;
    // 线程池
    // QThreadPool m_threadPool;
};

#endif // IMAGEPROVIDER_HPP