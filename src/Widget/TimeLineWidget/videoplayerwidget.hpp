#ifndef VIDEOPLAYERWIDGET_HPP
#define VIDEOPLAYERWIDGET_HPP

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QImage>
#include <QMatrix4x4>
#include <QVector>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QMouseEvent>
#include <QFileDialog>

// 如果使用 Qt6
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLFunctions_3_3_Core>
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

class VideoPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VideoPlayerWidget(QWidget* parent = nullptr);
    ~VideoPlayerWidget() override;

    void openFile(const QString& filePath);
    void play();
    void pause();
    void stop();

    // 更新视频帧
    void updateFrame(const QImage& frame);
    // 清除当前帧
    void clearFrame();
    // 设置保持宽高比
    void setKeepAspectRatio(bool keep);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupShaders();
    void setupVertexBuffers();
    void updateTexture(const QImage& frame);
    void decodeVideo();

    QOpenGLShaderProgram* m_program;
    QOpenGLTexture* m_texture;
    QVector<GLfloat> m_vertices;
    QVector<GLfloat> m_texCoords;
    GLuint m_posAttr;
    GLuint m_texAttr;
    GLuint m_matrixUniform;
    QMatrix4x4 m_projection;

    AVFormatContext* m_formatCtx;
    AVCodecContext* m_codecCtx;
    AVFrame* m_frame;
    AVPacket* m_packet;
    SwsContext* m_swsCtx;
    int m_videoStreamIndex;
    QImage m_currentFrame;
    QTimer* m_timer;
    QMutex m_mutex;
    bool m_isPlaying;
    
    // 保持宽高比
    bool m_keepAspectRatio;
    float m_aspectRatio;
};

#endif // VIDEOPLAYERWIDGET_HPP 