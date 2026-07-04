#pragma once

#include "Common/DataTypes/GpuTextureHandle.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QSize>
#include <QSurfaceFormat>

#include <memory>

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @file ImageTextureView.hpp
 * @brief 嵌入式 GPU 纹理显示控件（QOpenGLWidget）
 *
 * ## 在图像管线中的位置
 *
 * ```
 * ImageTimestampRingQueue / ImageGpuUpload
 *         ↓ GpuTextureHandle（share group 内 textureId）
 * ImageTextureView::setTexture()
 *         ↓ glBindTexture + 全屏四边形 shader
 * 节点面板 / QWidget 内显示
 * ```
 *
 * ## 核心职责
 *
 * - **零拷贝显示**：仅 bind 已有 textureId，不读回 CPU、不再次上传
 * - **contain 缩放**：保持宽高比，图像完整可见，空余区域显示背景色
 * - **V 轴翻转校正**：glTexImage2D 首行在纹理底部，OpenCV 首行在顶部，绘制时翻转 texCoord
 *
 * ## 使用场景
 *
 * | 场景                         | 用法                                      |
 * |------------------------------|-------------------------------------------|
 * | 普通 QWidget 子控件          | 直接 new ImageTextureView，setTexture     |
 * | QtNodes 节点 embeddedWidget  | 须用 ImageTextureViewHost 包裹（见 Host） |
 *
 * ## 前置条件
 *
 * - 应用启用 `Qt::AA_ShareOpenGLContexts`（main.cpp）
 * - 纹理由同一 OpenGL share group 内上下文创建（ImageGpuUpload 等）
 * - 使用 CompatibilityProfile + GLSL 1.x，与 legacy 固定管线兼容
 *
 * @see ImageTextureViewHost  QGraphicsView 嵌入时的刷新桥接
 * @see ImageTextureWindow    独立全屏窗口输出
 */
class GUI_ELEMENTS_EXPORT ImageTextureView final : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit ImageTextureView(QWidget* parent = nullptr);

    /**
     * @brief 设置待显示纹理
     * @param texture  共享组内有效 GpuTextureHandle（width/height/textureId）
     * @param repaint  true：立即对本控件 update()，触发 paintGL
     *                 false：仅更新 m_texture，由外层 ImageTextureViewHost 统一调度重绘
     *
     * 纹理 lifetime 由 GpuTextureHandle::lifetime 管理；本控件不持有所有权延长。
     */
    void setTexture(const NodeDataTypes::GpuTextureHandle& texture, bool repaint = true);

    /**
     * @brief 清空当前纹理，paintGL 仅绘制背景色
     * @param repaint  语义同 setTexture
     */
    void clearTexture(bool repaint = true);

    /**
     * @brief 嵌入节点时不强制占位尺寸
     *
     * 返回 (0,0) 让父布局（QGraphicsProxyWidget / QVBoxLayout）按节点框拉伸。
     */
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    /** @brief 创建 GL 资源：initializeOpenGLFunctions、链接着色器 */
    void initializeGL() override;

    /**
     * @brief 每帧绘制
     *
     * 1. glClear 背景（深灰 #1F1F1F 近似）
     * 2. 按 contain 计算 scaleX/scaleY
     * 3. TRIANGLE_STRIP 四边形 + texture2D 采样
     */
    void paintGL() override;

private:
    NodeDataTypes::GpuTextureHandle m_texture;              ///< 当前帧待 bind 的纹理句柄
    std::unique_ptr<QOpenGLShaderProgram> m_program;        ///< 内联 GLSL 全屏四边形 program
};
