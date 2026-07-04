/**
 * @file ImageTextureWindow.cpp
 * @brief QOpenGLWindow 独立纹理全屏显示实现
 *
 * 与 ImageTextureView 共享相同的：
 *   - CompatibilityProfile + GLSL 1.x shader
 *   - contain 缩放算法
 *   - V 轴 texCoord 翻转（OpenCV / glTexImage2D 约定）
 *
 * 差异：
 *   - 背景色纯黑（投影场景）
 *   - resizeGL 显式 glViewport
 *   - 默认无边框 + 置顶；Esc / onClosed 生命周期管理
 */

#include "ImageTextureWindow.hpp"

#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QOpenGLShader>

ImageTextureWindow::ImageTextureWindow()
    : QOpenGLWindow(NoPartialUpdate)
{
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    setFormat(fmt);
    // 舞台辅屏 / 第二显示器：无边框全屏，置顶避免被主界面遮挡
    setFlags(flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // QWindow 不参与 QWidget 的 quit-on-last-window-closed，需在应用退出时主动销毁原生窗口
    if (QGuiApplication* app = qobject_cast<QGuiApplication*>(QGuiApplication::instance())) {
        QObject::connect(app, &QGuiApplication::aboutToQuit, this, [this]() {
            onClosed = nullptr;
            if (isVisible()) {
                hide();
            }
            close();
            destroy();
        }, Qt::DirectConnection);
    }
}

void ImageTextureWindow::setTexture(const NodeDataTypes::GpuTextureHandle& texture)
{
    m_texture = texture;
    update();
}

void ImageTextureWindow::clearTexture()
{
    m_texture = NodeDataTypes::GpuTextureHandle();
    update();
}

void ImageTextureWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

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

void ImageTextureWindow::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void ImageTextureWindow::paintGL()
{
    // 全屏输出使用纯黑底，与嵌入式 ImageTextureView 的深灰区分
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_texture.valid() || !m_program || !m_program->isLinked()) {
        return;
    }

    // contain 缩放（逻辑与 ImageTextureView::paintGL 一致）
    const float imgRatio = static_cast<float>(m_texture.width) / static_cast<float>(m_texture.height);
    const float wndRatio = width() > 0 && height() > 0
        ? static_cast<float>(width()) / static_cast<float>(height())
        : imgRatio;

    float scaleX = 1.f;
    float scaleY = 1.f;
    if (imgRatio > wndRatio) {
        scaleY = wndRatio / imgRatio;
    } else {
        scaleX = imgRatio / wndRatio;
    }

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

void ImageTextureWindow::keyPressEvent(QKeyEvent* e)
{
    if (e && e->key() == Qt::Key_Escape) {
        close();
        e->accept();
        return;
    }
    QOpenGLWindow::keyPressEvent(e);
}

bool ImageTextureWindow::event(QEvent* e)
{
    // 用户 Alt+F4、系统关闭、hide() 等路径统一通知节点
    if (e && (e->type() == QEvent::Hide || e->type() == QEvent::Close)) {
        if (onClosed) {
            onClosed();
        }
    }
    return QOpenGLWindow::event(e);
}
