#pragma once

#include "Common/DataTypes/GpuTextureHandle.h"

#include <QEvent>
#include <QKeyEvent>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWindow>
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
 * @brief 独立 GPU 纹理全屏/辅屏输出窗口（QOpenGLWindow）
 *
 * ## 与 ImageTextureView 的分工
 *
 * | 组件               | 基类            | 典型场景                          |
 * |--------------------|-----------------|-----------------------------------|
 * | ImageTextureView   | QOpenGLWidget   | 节点面板内嵌预览                  |
 * | ImageTextureWindow | QOpenGLWindow   | 多显示器输出、舞台辅屏、全屏投影    |
 *
 * ## 在管线中的位置
 *
 * ```
 * WindowDisplayModel::onSystemFrameTick
 *       → getLatestFrame() from ImageTimestampRingQueue
 *       → ImageTextureWindow::setTexture(frame.texture)
 *       → paintGL（独立 surface，不经过 QGraphicsView）
 * ```
 *
 * ## 特性
 *
 * - **contain 绘制 + V 轴翻转**：与 ImageTextureView 算法一致，保证节点预览与辅屏一致
 * - **无边框 + 置顶**：默认适合投影输出；可通过 setScreen / setGeometry 定位到指定显示器
 * - **Esc 关闭**：keyPressEvent 捕获 Qt::Key_Escape
 * - **onClosed 回调**：Hide / Close 事件时通知节点同步按钮状态（如"关闭窗口"）
 *
 * ## 前置条件
 *
 * 同 ImageTextureView：纹理须在 OpenGL share group 内，应用须 AA_ShareOpenGLContexts。
 *
 * @see WindowDisplayModel  节点侧驱动本窗口的 tick 刷新
 *
 * @code
 * auto* window = new ImageTextureWindow();
 * window->onClosed = []() { syncUiState(); };
 * window->setScreen(QGuiApplication::screens().at(1));
 * window->setGeometry(screen->geometry());
 * window->setTexture(frame.texture);
 * window->showFullScreen();
 * @endcode
 */
class GUI_ELEMENTS_EXPORT ImageTextureWindow final : public QOpenGLWindow, protected QOpenGLFunctions
{
public:
    ImageTextureWindow();

    /**
     * @brief 设置纹理并 requestUpdate
     *
     * QOpenGLWindow 拥有独立 surface，不经过 QGraphicsProxyWidget，
     * 可直接 update()，无需 ImageTextureViewHost 双层桥接。
     */
    void setTexture(const NodeDataTypes::GpuTextureHandle& texture);

    /** @brief 清空纹理，paintGL 仅绘制黑色背景 */
    void clearTexture();

    /**
     * @brief 窗口 Hide 或 Close 时调用的可选回调
     *
     * 用于 WindowDisplayModel 等将节点 UI（如"打开窗口"按钮）与窗口生命周期同步。
     */
    std::function<void()> onClosed;

protected:
    /** @brief 初始化 GL：着色器与 ImageTextureView 相同 */
    void initializeGL() override;

    /** @brief 窗口尺寸变化时设置 glViewport */
    void resizeGL(int w, int h) override;

    /** @brief contain 模式绘制当前纹理（背景纯黑，适合投影） */
    void paintGL() override;

    /** @brief Esc 键关闭窗口 */
    void keyPressEvent(QKeyEvent* e) override;

    /** @brief 拦截 Hide/Close，触发 onClosed */
    bool event(QEvent* e) override;

private:
    NodeDataTypes::GpuTextureHandle m_texture;              ///< 当前帧纹理
    std::unique_ptr<QOpenGLShaderProgram> m_program;        ///< 全屏四边形纹理 shader
};
