#pragma once

#include "Common/DataTypes/GpuTextureHandle.h"

#include <QEvent>
#include <QKeyEvent>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWindow>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QSurfaceFormat>

#include <functional>
#include <memory>

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @file ImageTextureWindow.hpp
 * @brief 独立 GPU 纹理输出窗口（QOpenGLWindow）
 *
 * GL 上下文与着色器仅在首次 show/expose 时由 Qt 创建并进入 initializeGL。
 * 构造阶段不主动 create()/makeCurrent()，避免与节点实例化竞态。
 *
 * @see WindowDisplayModel
 */
class GUI_ELEMENTS_EXPORT ImageTextureWindow final : public QOpenGLWindow, protected QOpenGLFunctions
{
public:
    enum class FitMode {
        Contain = 0,
        Cover = 1,
        Stretch = 2
    };

    /** 窗口相对目标屏幕区域的对齐。offset 在此基础上再平移。 */
    enum class Anchor {
        Center = 0,  ///< 自定义尺寸在区域内居中；填满时即整块区域
        TopLeft = 1  ///< 左上角对齐区域原点，再用 offset 平移
    };

    /**
     * @brief 构造窗口对象（不创建 native surface / GL context）
     *
     * 依赖应用级 Qt::AA_ShareOpenGLContexts 自动加入共享组（勿手动传 globalShareContext）。
     * 真正的 GL 初始化发生在 openOnScreen() → show() → initializeGL()。
     */
    ImageTextureWindow();

    void setTexture(const NodeDataTypes::GpuTextureHandle& texture);
    void clearTexture();

    void setFitMode(FitMode mode);
    FitMode fitMode() const { return m_fitMode; }

    void setTitle(const QString& title);
    QString title() const { return m_title; }

    void setBordersEnabled(bool enabled);
    bool bordersEnabled() const { return m_borders; }

    void setAlwaysOnTop(bool enabled);
    bool alwaysOnTop() const { return m_alwaysOnTop; }

    void setCloseOnEscape(bool enabled);
    bool closeOnEscape() const { return m_closeOnEscape; }

    /** @brief 放置并显示；首次 show 时触发 GL 初始化 */
    void openOnScreen(QScreen* screen,
                      const QSize& customSize = QSize(),
                      bool ignoreTaskbar = true,
                      QPoint offset = QPoint(),
                      Anchor anchor = Anchor::Center);

    /** @brief 隐藏窗口（不销毁；由节点侧决定 deleteLater） */
    void closeWindow();

    /** @brief GL 是否已完成 initializeGL（可用于避免过早 update） */
    bool isGlReady() const { return m_glReady; }

    std::function<void()> onClosed;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override;

private:
    void applyWindowFlags();
    void notifyClosed();
    void computeQuadScale(float& scaleX, float& scaleY) const;
    void requestRedraw();

    NodeDataTypes::GpuTextureHandle m_texture;
    std::unique_ptr<QOpenGLShaderProgram> m_program;

    FitMode m_fitMode = FitMode::Contain;
    QString m_title = QStringLiteral("Window Display");
    bool m_borders = false;
    bool m_alwaysOnTop = true;
    bool m_closeOnEscape = true;
    bool m_closedNotified = false;
    bool m_closing = false;
    bool m_glReady = false;
};
