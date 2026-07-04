#pragma once

#include "ImageTextureView.hpp"

#include <QWidget>

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @file ImageTextureViewHost.hpp
 * @brief ImageTextureView 的外层 QWidget 宿主（QtNodes 节点嵌入专用）
 *
 * ## 问题背景
 *
 * QOpenGLWidget 作为 `NodeDelegateModel::embeddedWidget()` 被 QGraphicsProxyWidget
 * 嵌入 QGraphicsView 时，**直接对 GL 子控件调用 update() 常常无法及时触发 paintGL**。
 * 这是 Qt 在场景图 + 独立 GL FBO 合成路径下的已知行为。
 *
 * ## 解决方案：双层 update 链
 *
 * ```
 * ImageShowModel::onSystemFrameTick
 *       → host->setTexture(tex)     // m_view 仅更新数据，repaint=false
 *       → ImageTextureViewHost::update()
 *       → paintEvent (Host)
 *       → m_view->update()
 *       → paintGL (ImageTextureView)
 * ```
 *
 * ## 典型用法
 *
 * @code
 * // ImageShowModel / 类似显示节点
 * auto* host = new ImageTextureViewHost();
 * embeddedWidget() 返回 host;
 *
 * // tick 驱动（从 ring buffer 取帧）
 * host->setTexture(frame.texture);
 *
 * // legacy 路径需在 Host 内 GL 控件上 makeCurrent 时：
 * host->textureView()->makeCurrent();
 * @endcode
 *
 * @see ImageShowModel  节点内预览
 */
class GUI_ELEMENTS_EXPORT ImageTextureViewHost final : public QWidget
{
    Q_OBJECT

public:
    explicit ImageTextureViewHost(QWidget* parent = nullptr);

    /**
     * @brief 更新纹理并由 Host 发起重绘链
     *
     * 内部调用 m_view->setTexture(texture, false)，再 Host::update()。
     */
    void setTexture(const NodeDataTypes::GpuTextureHandle& texture);

    /** @brief 清空纹理并触发重绘链 */
    void clearTexture();

    /**
     * @brief 访问内部 QOpenGLWidget
     *
     * 用于需要绑定 GL 上下文的 legacy 操作（如 Mat 直传上传前的 makeCurrent）。
     */
    ImageTextureView* textureView() const { return m_view; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    /**
     * @brief Host 收到重绘请求后，转发给内部 ImageTextureView
     *
     * 这是 QGraphicsProxyWidget 场景下 GL 刷新可靠触发的关键步骤。
     */
    void paintEvent(QPaintEvent* event) override;

    /** @brief 节点缩放时请求重绘，避免 proxy 尺寸变化后 GL 区域不同步 */
    void resizeEvent(QResizeEvent* event) override;

private:
    ImageTextureView* m_view = nullptr;  ///< 内嵌 GL 显示控件，填满 Host 布局
};
