#include "videoplayerwidget.hpp"
#include <QOpenGLBuffer>
#include <QPainter>
#include <QFont>

// 顶点着色器
static const char* vertexShaderSource =
    "attribute vec4 position;\n"
    "attribute vec2 texCoord;\n"
    "varying vec2 texCoordVarying;\n"
    "uniform mat4 matrix;\n"
    "void main() {\n"
    "    gl_Position = matrix * position;\n"
    "    texCoordVarying = texCoord;\n"
    "}\n";

// 片段着色器
static const char* fragmentShaderSource =
    "varying vec2 texCoordVarying;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(texture, texCoordVarying);\n"
    "}\n";

VideoPlayerWidget::VideoPlayerWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_program(nullptr)
    , m_texture(nullptr)
    , m_frameUpdated(false)
    , m_keepAspectRatio(true)
    , m_aspectRatio(1.0f)
{
    // 设置背景色为黑色
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);
}

VideoPlayerWidget::~VideoPlayerWidget()
{
    makeCurrent();
    delete m_texture;
    delete m_program;
    doneCurrent();
}

void VideoPlayerWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // 设置清除色为黑色

    setupShaders();
    setupVertexBuffers();

    // 创建纹理对象
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    // 加载并显示默认图片
    loadDefaultImage();
}

void VideoPlayerWidget::setupShaders()
{
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();

    m_posAttr = m_program->attributeLocation("position");
    m_texAttr = m_program->attributeLocation("texCoord");
    m_matrixUniform = m_program->uniformLocation("matrix");
}

void VideoPlayerWidget::setupVertexBuffers()
{
    // 设置顶点坐标
    m_vertices = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    // 设置纹理坐标
    m_texCoords = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };
}

void VideoPlayerWidget::updateFrame(const QImage& frame)
{
    if (frame.isNull()) return;

    m_currentFrame = frame.convertToFormat(QImage::Format_RGBA8888);
    m_frameUpdated = true;
    m_aspectRatio = static_cast<float>(frame.width()) / frame.height();
    update();
}

void VideoPlayerWidget::clearFrame()
{
    m_currentFrame = QImage();
    m_frameUpdated = true;
    update();
}

void VideoPlayerWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_currentFrame.isNull()) return;

    if (m_frameUpdated) {
        // 更新纹理
        m_texture->destroy();
        m_texture->create();
        m_texture->setData(m_currentFrame);
        m_frameUpdated = false;
    }

    m_program->bind();
    m_texture->bind();

    // 设置变换矩阵
    m_program->setUniformValue(m_matrixUniform, m_projection);

    // 设置顶点属性
    m_program->enableAttributeArray(m_posAttr);
    m_program->enableAttributeArray(m_texAttr);
    m_program->setAttributeArray(m_posAttr, m_vertices.constData(), 2);
    m_program->setAttributeArray(m_texAttr, m_texCoords.constData(), 2);

    // 绘制矩形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->disableAttributeArray(m_posAttr);
    m_program->disableAttributeArray(m_texAttr);
    m_program->release();
}

void VideoPlayerWidget::resizeGL(int w, int h)
{
    // 更新投影矩阵
    m_projection.setToIdentity();
    
    if (m_keepAspectRatio && !m_currentFrame.isNull()) {
        float windowRatio = static_cast<float>(w) / h;
        float imageRatio = static_cast<float>(m_currentFrame.width()) / m_currentFrame.height();
        
        if (windowRatio > imageRatio) {
            // 窗口比图片更宽，垂直填充
            float scale = imageRatio / windowRatio;
            m_projection.scale(scale, 1.0f, 1.0f);
        } else {
            // 窗口比图片更高，水平填充
            float scale = windowRatio / imageRatio;
            m_projection.scale(1.0f, scale, 1.0f);
        }
    }
}

void VideoPlayerWidget::setKeepAspectRatio(bool keep)
{
    if (m_keepAspectRatio != keep) {
        m_keepAspectRatio = keep;
        update();
    }
}

void VideoPlayerWidget::loadDefaultImage()
{

    // 如果加载失败，创建一个默认的纯色图片
    QImage *m_defaultImage = new QImage(640, 360, QImage::Format_RGB32);
    m_defaultImage->fill(Qt::black);  // 使用黑色背景
    
    QPainter painter(m_defaultImage);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 20));
    painter.drawText(m_defaultImage->rect(), Qt::AlignCenter, "No Video");

    // 更新显示
    updateFrame(*m_defaultImage);
} 