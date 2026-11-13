// ============================================================================
//  GlCudaInterop.hpp —— CUDA-GL 互操作封装（③阶段 0 拷贝的核心件）
//
//  设计依据：《hw-decode-implementation-plan.md》③-2
//    * 职责：把 FFmpeg CUDA 硬解帧（NV12，data 指向显存 CUdeviceptr）
//      拷进已注册的 GL 纹理对（R8 亮度 + RG8 色度）——GPU 内 D2D 拷贝
//      （cudaMemcpy2DToArray），像素全程不落系统内存，即 0 拷贝目标；
//    * 设备一致性：interop 要求 CUDA 上下文与 GL 上下文在同一物理 GPU。
//      init() 经 cudaGLGetDevices 探测"正在服务当前 GL 上下文"的 CUDA
//      设备——0 台即失败（双显卡笔记本 GL 落在核显的常态）→ 调用方
//      （GlRenderer）翻转协商旗标，解码端自动回退下载路径（无黑屏）；
//    * 生命周期：GL 上下文 current 的线程上 init/shutdown（渲染线程）；
//      GL 纹理注册（cudaGraphicsGLRegisterImage）在纹理创建后通过
//      registerTextures() 一次性完成，纹理销毁前 unregisterTextures()；
//    * 依赖策略：CUDA Toolkit（cuda.h + cudart_static）——运行机只需
//      显卡驱动（cudart_static 静态编入，无 cudart DLL 部署负担）。
//      无 CUDA Toolkit 的构建环境：MS_HAS_CUDA 未定义，本类退化为
//      恒 false 桩（协商旗标自动回退，功能无损）。
// ============================================================================

#pragma once

#include <cstdint>

#ifdef MS_HAS_CUDA

struct AVFrame; // FFmpeg 前置声明（完整类型仅 .cpp 可见）

namespace ms {

class GlCudaInterop {
public:
    GlCudaInterop() = default;
    ~GlCudaInterop() { shutdown(); }
    GlCudaInterop(const GlCudaInterop&) = delete;
    GlCudaInterop& operator=(const GlCudaInterop&) = delete;

    /// 初始化：探测服务当前 GL 上下文的 CUDA 设备并建立运行时上下文。
    /// 【前提】GL 上下文必须在调用线程 current（渲染线程契约）。
    /// \return false = 无可用设备（GL 在非 N 卡上）——调用方回退下载路径
    bool init();

    bool ready() const noexcept { return ready_; }
    int  deviceId() const noexcept { return device_id_; }

    /// 注册 GL 纹理【双组】（纹理创建后调用；把 GL 纹理句柄登记进 CUDA——
    /// 此后 map/unmap 期间 CUDA 可把纹理论证为 cudaArray 写入）。
    /// 【双缓冲必要性】单组纹理下，本帧 cudaGraphicsMapResources 会隐式
    /// 同步等待 GPU 完成"上一帧对这些纹理的采样绘制"（GL↔CUDA 交接点
    /// 的驱动语义）——每帧 map 撞上上一帧 draw，渲染管线被串行化
    /// （实测 3050Ti：map 恒 ≈16ms ≈ 60Hz 节拍，compose 顶破帧预算，
    /// 帧率掉到 58-59fps）。双组纹理按帧奇偶交替：map 本帧组时上一帧
    /// draw 用的是另一组，早已画完——隐式同步链被打断，map 回到亚毫秒。
    /// 尺寸变化重建纹理后需重新调用（内部先注销旧注册，幂等）。
    /// \return false = 注册失败（任一纹理）——调用方回退下载路径
    bool registerTextures(std::uint32_t luma_tex0, std::uint32_t chroma_tex0,
                          std::uint32_t luma_tex1, std::uint32_t chroma_tex1);

    /// 注销纹理注册（纹理对象销毁前调用；幂等——未注册时为空操作）
    void unregisterTextures();

    /// 把 CUDA NV12 硬解帧拷进【指定组】已注册 GL 纹理对
    /// （map → cudaMemcpy2DToArray（D2D，双平面）→ unmap）。
    /// \param hwframe AV_PIX_FMT_CUDA 硬解帧（data[0]=Y 平面显存指针、
    ///        data[1]=UV 交错平面；linesize 为行距字节）
    /// \param group 纹理组索引（0/1；调用方按帧奇偶交替——双缓冲防串行）
    /// \return false = 拷贝失败（帧丢弃；调用方降级回退）
    bool uploadNv12(const AVFrame* hwframe, int group);

    /// 全部 CUDA 资源释放（幂等；渲染线程调用）
    void shutdown();

private:
    bool ready_ = false;      ///< init 成功标记
    int  device_id_ = -1;     ///< 服务 GL 的 CUDA 设备号（诊断）
    void* luma_res_[2] = {nullptr, nullptr};   ///< 亮度纹理注册句柄（双组）
    void* chroma_res_[2] = {nullptr, nullptr}; ///< 色度纹理注册句柄（双组）
};

} // namespace ms

#else // !MS_HAS_CUDA —— 无 CUDA Toolkit 构建：恒失败桩（保持接口完整）

struct AVFrame;

namespace ms {

/// 无 CUDA 工具链的桩实现：init 恒 false → GlRenderer 翻转协商旗标 →
/// 解码端自动走 av_hwframe_transfer_data 下载路径（功能无损）。
class GlCudaInterop {
public:
    bool init() { return false; }          ///< 无工具链：恒不可用
    bool ready() const noexcept { return false; }
    int  deviceId() const noexcept { return -1; }
    bool registerTextures(std::uint32_t, std::uint32_t,
                          std::uint32_t, std::uint32_t) { return false; }
    void unregisterTextures() {}
    bool uploadNv12(const AVFrame*, int) {
        return false;
    }
    void shutdown() {}
};

} // namespace ms

#endif // MS_HAS_CUDA
