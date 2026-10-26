/**
 * @file ImageTextureWindow.cpp
 * @brief QOpenGLWindow 独立纹理显示；GL 延迟到首次 show/expose
 */

#include "ImageTextureWindow.hpp"

#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QOpenGLShader>

ImageTextureWindow::ImageTextureWindow()
    : QOpenGLWindow(NoPartialUpdate)
{
    // 重要：禁止把 QOpenGLContext::globalShareContext() 传给 QOpenGLWindow 构造。
    // Qt 文档明确警告：不得用 globalShareContext 作为 QOpenGLWindow/QOpenGLWidget 的 shareContext，
    // 否则多窗口时第二个实例极易崩溃。AA_ShareOpenGLContexts 已会自动加入共享组。
    //
    // 此处只设 format/flags/title；native window + GL context 在首次 show() 时创建。
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    setFormat(fmt);
    applyWindowFlags();
    QOpenGLWindow::setTitle(m_title);
    // 退出清理由 WindowDisplayModel 负责：transient parent 避免挡住 quitOnLastWindowClosed，
    // aboutToQuit / 析构里同步 destroy。此处不要再连 aboutToQuit，多窗口 DirectConnection 会竞态。
}

void ImageTextureWindow::requestRedraw()
{
    // 未 expose 前 update() 可能提前拉起 context，与其它 GL 路径竞态
    if (m_glReady && isExposed()) {
        update();
    }
}

void ImageTextureWindow::setTexture(const NodeDataTypes::GpuTextureHandle& texture)
{
    m_texture = texture;
    requestRedraw();
}

void ImageTextureWindow::clearTexture()
{
    m_texture = NodeDataTypes::GpuTextureHandle();
    requestRedraw();
}

void ImageTextureWindow::setFitMode(FitMode mode)
{
    if (m_fitMode == mode) {
        return;
    }
    m_fitMode = mode;
    requestRedraw();
}

void ImageTextureWindow::setTitle(const QString& title)
{
    m_title = title.isEmpty() ? QStringLiteral("Window Display") : title;
    QOpenGLWindow::setTitle(m_title);
}

void ImageTextureWindow::setBordersEnabled(bool enabled)
{
    if (m_borders == enabled) {
        return;
    }
    m_borders = enabled;
    const bool wasVisible = isVisible();
    applyWindowFlags();
    if (wasVisible) {
        show();
    }
}

void ImageTextureWindow::setAlwaysOnTop(bool enabled)
{
    if (m_alwaysOnTop == enabled) {
        return;
    }
    m_alwaysOnTop = enabled;
    const bool wasVisible = isVisible();
    applyWindowFlags();
    if (wasVisible) {
        show();
    }
}

void ImageTextureWindow::setCloseOnEscape(bool enabled)
{
    m_closeOnEscape = enabled;
}

void ImageTextureWindow::applyWindowFlags()
{
    Qt::WindowFlags f = Qt::Window;
    if (!m_borders) {
        f |= Qt::FramelessWindowHint;
    }
    if (m_alwaysOnTop) {
        f |= Qt::WindowStaysOnTopHint;
    }
    setFlags(f);
}

void ImageTextureWindow::openOnScreen(QScreen* screen,
                                      const QSize& customSize,
                                      bool ignoreTaskbar,
                                      QPoint offset,
                                      Anchor anchor)
{
    if (!screen) {
        screen = this->screen();
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
    }
    if (!screen) {
        return;
    }

    m_closedNotified = false;
    m_closing = false;

    applyWindowFlags();
    setScreen(screen);

    const QRect area = ignoreTaskbar ? screen->geometry() : screen->availableGeometry();
    const bool hasCustomSize = customSize.isValid() && customSize.width() > 0 && customSize.height() > 0;
    QRect geo;
    if (anchor == Anchor::TopLeft) {
        const QSize size = hasCustomSize ? customSize : area.size();
        geo = QRect(area.topLeft() + offset, size);
    } else if (hasCustomSize) {
        geo = QRect(QPoint(0, 0), customSize);
        geo.moveCenter(area.center());
        geo.translate(offset);
    } else {
        geo = area.translated(offset);
    }

    // show() 才会创建 native window + GL context，并回调 initializeGL
    setGeometry(geo);
    show();
    raise();
    requestActivate();

    if (m_glReady) {
        update();
    }
}

void ImageTextureWindow::closeWindow()
{
    if (!isVisible() && !m_closing) {
        return;
    }
    m_closing = true;
    hide();
    m_closing = false;
    notifyClosed();
}

void ImageTextureWindow::notifyClosed()
{
    if (m_closedNotified) {
        return;
    }
    m_closedNotified = true;
    if (onClosed) {
        onClosed();
    }
}

void ImageTextureWindow::computeQuadScale(float& scaleX, float& scaleY) const
{
    scaleX = 1.f;
    scaleY = 1.f;

    if (!m_texture.valid() || m_texture.width <= 0 || m_texture.height <= 0) {
        return;
    }

    if (m_fitMode == FitMode::Stretch) {
        return;
    }

    const float imgRatio = static_cast<float>(m_texture.width) / static_cast<float>(m_texture.height);
    const float wndRatio = width() > 0 && height() > 0
        ? static_cast<float>(width()) / static_cast<float>(height())
        : imgRatio;

    if (m_fitMode == FitMode::Contain) {
        if (imgRatio > wndRatio) {
            scaleY = wndRatio / imgRatio;
        } else {
            scaleX = imgRatio / wndRatio;
        }
    } else {
        if (imgRatio > wndRatio) {
            scaleX = imgRatio / wndRatio;
        } else {
            scaleY = wndRatio / imgRatio;
        }
    }
}

void ImageTextureWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    m_program = std::make_unique<QOpenGLShaderProgram>();
    const bool vsOk = m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
        attribute vec2 vertex;
        attribute vec2 texCoord;
        varying vec2 vTexCoord;
        void main() {
            gl_Position = vec4(vertex, 0.0, 1.0);
            vTexCoord = texCoord;
        }
    )");
    const bool fsOk = m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
        uniform sampler2D uTexture;
        varying vec2 vTexCoord;
        void main() {
            gl_FragColor = texture2D(uTexture, vTexCoord);
        }
    )");
    if (!vsOk || !fsOk || !m_program->link()) {
        m_program.reset();
        m_glReady = false;
        return;
    }

    m_glReady = true;
}

void ImageTextureWindow::resizeGL(int w, int h)
{
    if (!m_glReady) {
        return;
    }
    glViewport(0, 0, w, h);
}

void ImageTextureWindow::paintGL()
{
    if (!m_glReady) {
        return;
    }

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_texture.valid() || !m_program || !m_program->isLinked()) {
        return;
    }

    float scaleX = 1.f;
    float scaleY = 1.f;
    computeQuadScale(scaleX, scaleY);

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
    if (e && e->key() == Qt::Key_Escape
        && (m_closeOnEscape || (e->modifiers() & Qt::ShiftModifier))) {
        closeWindow();
        e->accept();
        return;
    }
    QOpenGLWindow::keyPressEvent(e);
}

bool ImageTextureWindow::event(QEvent* e)
{
    if (!e) {
        return QOpenGLWindow::event(e);
    }

    if (e->type() == QEvent::Close || e->type() == QEvent::Hide) {
        if (!m_closing) {
            notifyClosed();
        }
    }
    return QOpenGLWindow::event(e);
}
