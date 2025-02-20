#ifndef VIDEOPLAYERWIDGET_HPP
#define VIDEOPLAYERWIDGET_HPP

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QImage>
#include <QMatrix4x4>
#include <QVector>

// 如果使用 Qt6
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLFunctions_3_3_Core>
#endif

class VideoPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit VideoPlayerWidget(QWidget* parent = nullptr);
    ~VideoPlayerWidget() override;

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

private:
    void setupShaders();
    void setupVertexBuffers();
    void updateTextureCoords();
    void loadDefaultImage();

private:
    QOpenGLShaderProgram* m_program;
    QOpenGLTexture* m_texture;
    
    // 顶点和纹理坐标
    QVector<GLfloat> m_vertices;
    QVector<GLfloat> m_texCoords;
    
    // 着色器属性位置
    GLuint m_posAttr;
    GLuint m_texAttr;
    GLuint m_matrixUniform;
    
    // 变换矩阵
    QMatrix4x4 m_projection;
    
    // 当前帧
    QImage m_currentFrame;
    bool m_frameUpdated;
    
    // 保持宽高比
    bool m_keepAspectRatio;
    float m_aspectRatio;
};

#endif // VIDEOPLAYERWIDGET_HPP 