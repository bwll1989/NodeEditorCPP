/**
 * @file ImageTextureView.cpp
 * @brief QOpenGLWidget 纹理显示实现
 *
 * 绘制管线：
 *   initializeGL() → 链接着色器
 *   setTexture()   → 更新 m_texture，可选 update()
 *   paintGL()      → clear → contain 缩放 → bind 纹理 → draw TRIANGLE_STRIP
 */

#include "ImageTextureView.hpp"

#include <QOpenGLShader>
#include <QSizePolicy>

ImageTextureView::ImageTextureView(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // CompatibilityProfile：与 ImageGpuUpload 离屏上下文及固定管线上传路径一致
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    setFormat(fmt);
    // 与 ImageTextureWindow 一致；PartialUpdate 在 QGraphicsProxyWidget 内易漏绘 GPU 纹理
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    setMinimumSize(0, 0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ImageTextureView::setTexture(const NodeDataTypes::GpuTextureHandle& texture, bool repaint)
{
    m_texture = texture;
    if (repaint) {
        update();
    }
}

void ImageTextureView::clearTexture(bool repaint)
{
    m_texture = NodeDataTypes::GpuTextureHandle();
    if (repaint) {
        update();
    }
}

QSize ImageTextureView::sizeHint() const
{
    return QSize(0, 0);
}

QSize ImageTextureView::minimumSizeHint() const
{
    return QSize(0, 0);
}

void ImageTextureView::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // GLSL 1.x + attribute/varying，兼容 QOpenGLWidget 默认 Compatibility 上下文
    m_program = std::make_unique<QOpenGLShaderProgram>();
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        attribute vec2 vertex;
        attribute vec2 texCoord;
        varying vec2 vTexCoord;
        void main() {
            gl_Position = vec4(vertex, 0.0, 1.0);
            vTexCoord = texCoord;
        }
    )");
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        uniform sampler2D uTexture;
        varying vec2 vTexCoord;
        void main() {
            gl_FragColor = texture2D(uTexture, vTexCoord);
        }
    )");
    m_program->link();
}

void ImageTextureView::paintGL()
{
    // 深灰背景，与节点编辑器画布风格接近；无纹理时用户看到空面板而非闪白
    glClearColor(0.12f, 0.12f, 0.12f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_texture.valid() || !m_program || !m_program->isLinked()) {
        return;
    }

    // ── contain 模式：整图可见，等比缩放，居中（通过 NDC 顶点缩放实现）──
    const float imgRatio = static_cast<float>(m_texture.width) / static_cast<float>(m_texture.height);
    const float wndRatio = width() > 0 && height() > 0
        ? static_cast<float>(width()) / static_cast<float>(height())
        : imgRatio;

    float scaleX = 1.f;
    float scaleY = 1.f;
    if (imgRatio > wndRatio) {
        // 图像更宽：宽度贴满 NDC ±1，高度缩小 → 上下留边
        scaleY = wndRatio / imgRatio;
    } else {
        // 图像更高：高度贴满，宽度缩小 → 左右留边
        scaleX = imgRatio / wndRatio;
    }

    // 顶点格式：[x, y, u, v] × 4
    // OpenGL 纹理坐标 v=0 在底部；OpenCV/glTexImage2D 上传时首行在纹理底部，
    // 屏幕坐标 y 向上为正，故 v 与 y 反向映射（下方顶点 v=1，上方 v=0）
    const float vertices[] = {
        -scaleX, -scaleY, 0.f, 1.f,
         scaleX, -scaleY, 1.f, 1.f,
        -scaleX,  scaleY, 0.f, 0.f,
         scaleX,  scaleY, 1.f, 0.f,
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture.textureId);

    m_program->bind();
    m_program->setUniformValue("uTexture", 0);
    m_program->enableAttributeArray("vertex");
    m_program->enableAttributeArray("texCoord");
    m_program->setAttributeArray("vertex", GL_FLOAT, vertices, 2, sizeof(float) * 4);
    m_program->setAttributeArray("texCoord", GL_FLOAT, vertices + 2, 2, sizeof(float) * 4);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_program->disableAttributeArray("vertex");
    m_program->disableAttributeArray("texCoord");
    m_program->release();

    glBindTexture(GL_TEXTURE_2D, 0);
}
