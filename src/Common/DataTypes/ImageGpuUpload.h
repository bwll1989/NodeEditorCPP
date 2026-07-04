#pragma once

#include "GpuTextureHandle.h"

#include <QMutex>

#include <functional>
#include <type_traits>
#include <utility>

class QOpenGLFunctions;
class QOpenGLContext;
class QOffscreenSurface;

namespace NodeDataTypes
{
/**
 * @brief 进程内统一的离屏 OpenGL 纹理上传服务（单例）
 *
 * 设计目的：
 *   - 各节点/源（如 ImageLoader、legacy Mat 路径）只需传入 cv::Mat，无需各自创建 GL 上下文。
 *   - 绑定 QOpenGLContext::globalShareContext()，上传的纹理可在任意共享组内的
 *     QOpenGLWidget / QOpenGLWindow 中直接 bind，实现零拷贝显示。
 *
 * 线程约束：
 *   - warmup() / 首次 ensureContext() 须在 GUI 主线程调用（创建 QOffscreenSurface）。
 *   - upload() 须在 GUI 主线程调用；内部加锁串行化 GL 操作。
 *
 * 前置条件：应用须设置 Qt::AA_ShareOpenGLContexts（见 main.cpp）。
 */
class DATATYPES_EXPORT ImageGpuUpload
{
public:
    static ImageGpuUpload& instance();

    /** 在 GUI 主线程预创建离屏 GL 上下文与 QOffscreenSurface */
    bool warmup();

    /** 将 BGR/BGRA/灰度 Mat 上传为 GL_TEXTURE_2D，失败返回 invalid handle（须 GUI 线程） */
    GpuTextureHandle upload(const cv::Mat& mat);

    /** 连续 CV_8UC4 BGRA Mat 直传，跳过色彩转换（须 GUI 线程） */
    GpuTextureHandle uploadBgra8(const cv::Mat& mat);

    /** 离屏上下文是否已成功创建 */
    bool isReady() const;

    /** 在离屏 GL 上下文中删除纹理（纹理 lifetime 析构时调用，线程安全） */
    void destroyTexture(unsigned int textureId);

    /** 从共享组纹理读回 BGR Mat（须 GUI 线程或经 mutex 串行化） */
    cv::Mat readbackToBgrMat(const GpuTextureHandle& handle, bool flipY = true);

    /**
     * @brief 在离屏 GL 上下文中执行 func（mutex 串行化，须 GUI 线程）
     *
     * ImageGpuPass 等 GPU 渲染通过此接口执行 fragment pass，与 upload 共享同一上下文。
     */
    template<typename Func>
    auto runGl(Func&& func) -> std::invoke_result_t<Func, QOpenGLFunctions*>
    {
        using Result = std::invoke_result_t<Func, QOpenGLFunctions*>;
        if constexpr (std::is_void_v<Result>) {
            runGlImpl([&](QOpenGLFunctions* f) { func(f); });
        } else {
            Result result{};
            const bool ok = runGlImpl([&](QOpenGLFunctions* f) { result = func(f); });
            return ok ? result : Result{};
        }
    }

private:
    /** runGl 底层：持锁 makeCurrent → task → doneCurrent */
    bool runGlImpl(const std::function<void(QOpenGLFunctions*)>& task);

    ImageGpuUpload();
    ~ImageGpuUpload();

    ImageGpuUpload(const ImageGpuUpload&) = delete;
    ImageGpuUpload& operator=(const ImageGpuUpload&) = delete;

    /** 懒创建离屏 QOpenGLContext + QOffscreenSurface，并与 globalShareContext 共享 */
    bool ensureContext();
    void releaseContext();

    mutable QMutex m_mutex;
    QOpenGLContext* m_context = nullptr;
    QOffscreenSurface* m_surface = nullptr;
    bool m_ready = false;
};

} // namespace NodeDataTypes
