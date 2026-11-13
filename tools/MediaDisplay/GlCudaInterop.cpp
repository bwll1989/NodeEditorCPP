// ============================================================================
//  GlCudaInterop.cpp —— CUDA-GL 互操作实现（探测/注册/D2D 拷贝/注销）
//
//  关键实现点：
//    * cudaGLGetDevices(cudaGLDeviceListCurrentFrame)：查询"正在服务当前
//      GL 上下文帧栅"的 CUDA 设备——这是 interop 同 GPU 硬约束的官方探测
//      口径；0 台 = GL 在核显/其它厂卡上，interop 不可用；
//    * 纹理注册：cudaGraphicsGLRegisterImage 把 GL 纹理对象登记为 CUDA
//      图形资源（GL 上下文 current 时调用——渲染线程契约）；
//    * 每帧拷贝：map → cudaMemcpy2DToArray(DeviceToDevice) → unmap。
//      【宽度单位陷阱】cudaMemcpy2DToArray 的 width 参数按【字节】计——
//      RG8 色度纹理单 texel 2 字节，w/2 texel 的行宽 = w 字节；传 w/2
//      会只拷半行（画面右半色度错乱），必须传帧宽 w；
//    * GL_TEXTURE_2D 常量：本项目 GL 3.3 函数/常量经手写加载器
//      （GlLoader.hpp，ms::gl 命名空间）——不经 gl/glcorearb 头，
//      与 display 层其余 GL 代码同一套常量来源。
// ============================================================================

#include "GlCudaInterop.hpp"

#ifdef MS_HAS_CUDA

#include <chrono>
#include <cstdio>
#include <cstdlib>

// 【include 顺序陷阱】cuda_gl_interop.h 在 Windows 上直接 #include <GL/gl.h>，
// 而 GL/gl.h 依赖 windows.h 先行定义 WINGDIAPI/APIENTRY——必须先引入
// GlLoader.hpp（内部含 windows.h），否则 GL/gl.h 的全部声明报错。
#include "GlLoader.hpp" // ms::gl 常量 + windows.h（WINGDIAPI/APIENTRY 就位）

#include <cuda.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

namespace ms {

// ---------------------------------------------------------------------------
// 内部辅助：CUDA 调用检查（失败打一次性日志）
// ---------------------------------------------------------------------------
namespace {

/// 检查 CUDA 运行时调用返回值（失败打日志返回 false）
bool checkCuda(cudaError_t err, const char* what) {
    if (err == cudaSuccess) return true;
    std::fprintf(stderr, "[gl-cuda] %s 失败: %s\n", what,
                 cudaGetErrorString(err));
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// init —— 探测服务当前 GL 上下文的 CUDA 设备
// ---------------------------------------------------------------------------
bool GlCudaInterop::init() {
    if (ready_) return true;

    // ---- 1. 找到正在服务当前 GL 上下文的 CUDA 设备 -----------------------
    // 【关键】interop 的硬约束：GL 上下文与 CUDA 上下文必须同 GPU。
    // 双显卡（Optimus）笔记本上 GL 常落在核显——此处探测到 0 台设备即
    // 判失败，由 GlRenderer 翻协商旗标回退下载路径（NvOptimusEnablement
    // 导出可让 GL 落到 N 卡，此时本探测通过）。
    unsigned int count = 0;
    int gl_devices[1] = {-1};
    if (!checkCuda(cudaGLGetDevices(&count, gl_devices, 1,
                                    cudaGLDeviceListCurrentFrame),
                   "cudaGLGetDevices"))
        return false;
    if (count == 0) {
        std::fprintf(stderr,
                     "[gl-cuda] 当前 GL 上下文不在 NVIDIA GPU 上"
                     "（双显卡？）——CUDA 0 拷贝不可用，回退下载路径\n");
        return false;
    }
    device_id_ = gl_devices[0];

    // ---- 2. 激活该设备的运行时主上下文（惰性初始化）----------------------
    // cudaFree(nullptr) 是运行时文档钦定的主上下文初始化触发器（无实际释放）
    if (!checkCuda(cudaSetDevice(device_id_), "cudaSetDevice")) return false;
    if (!checkCuda(cudaFree(nullptr), "cudaFree(0)（主上下文初始化）"))
        return false;

    // ---- 3. 设备信息打印（诊断：确认 interop 落在预期 GPU）---------------
    cudaDeviceProp prop{};
    if (checkCuda(cudaGetDeviceProperties(&prop, device_id_),
                  "cudaGetDeviceProperties")) {
        std::printf("[gl-cuda] interop 就绪: device %d = %s（SM %d.%d）\n",
                    device_id_, prop.name, prop.major, prop.minor);
        std::fflush(stdout);
    }
    ready_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// registerTextures —— GL 纹理对登记进 CUDA（纹理创建后一次性调用）
// ---------------------------------------------------------------------------
namespace {

/// 单纹理注册辅助（成功时 *out_res 持 cudaGraphicsResource*；失败 nullptr）
bool registerOne(void** out_res, std::uint32_t gl_tex) {
    *out_res = nullptr;
    if (gl_tex == 0) return false;
    cudaGraphicsResource* res = nullptr;
    // 【宏污染规避】cuda_gl_interop.h 引入的 GL/gl.h 把 GL_TEXTURE_2D
    // 等定义成了无守卫宏——ms::gl 枚举在此 TU 被 macro 展开，用数值
    // 常量直传（GL_TEXTURE_2D = 0x0DE1，与 GlLoader.hpp 枚举同值）。
    constexpr unsigned int kTexture2D = 0x0DE1;
    // flags=None：纹理只被 CUDA 写（D2D 拷贝目标），无需 ReadOverwrite 语义
    if (!checkCuda(cudaGraphicsGLRegisterImage(&res, gl_tex, kTexture2D,
                                               cudaGraphicsRegisterFlagsNone),
                   "cudaGraphicsGLRegisterImage"))
        return false;
    *out_res = res;
    return true;
}

} // namespace

bool GlCudaInterop::registerTextures(std::uint32_t luma_tex0,
                                     std::uint32_t chroma_tex0,
                                     std::uint32_t luma_tex1,
                                     std::uint32_t chroma_tex1) {
    if (!ready_) return false;
    unregisterTextures(); // 幂等：清掉旧注册（换尺寸重建纹理的场景）

    // 双组逐对注册（任一失败即整体回滚——不留半注册状态）
    const std::uint32_t tex[4] = {luma_tex0, chroma_tex0,
                                  luma_tex1, chroma_tex1};
    void*               res[4] = {nullptr, nullptr, nullptr, nullptr};
    for (int i = 0; i < 4; ++i) {
        if (!registerOne(&res[i], tex[i])) {
            for (int j = 0; j < 4; ++j) { // 回滚已成功的注册
                if (res[j] != nullptr)
                    cudaGraphicsUnregisterResource(
                        static_cast<cudaGraphicsResource*>(res[j]));
            }
            return false;
        }
    }
    luma_res_[0]   = res[0];
    chroma_res_[0] = res[1];
    luma_res_[1]   = res[2];
    chroma_res_[1] = res[3];

    // 【注册物化】注册后立即空跑一轮 map→写入→unmap：强迫驱动完成注册
    // 句柄与纹理存储的首次绑定（仅 getMappedArray 不够——需要一次真实
    // 写入周期）。实测（RTX 3050 Ti / 572.61）缺这一步时，注册后首次
    // D2D 拷贝写入的存储与 GL 采样不同步——首帧黑、第二帧起正常。
    // 双组后对两组各空跑一轮（两组都要物化，交替首用无黑帧）。
    for (int g = 0; g < 2; ++g) {
        cudaGraphicsResource* prime[2] = {
            static_cast<cudaGraphicsResource*>(luma_res_[g]),
            static_cast<cudaGraphicsResource*>(chroma_res_[g])};
        if (checkCuda(cudaGraphicsMapResources(2, prime, 0),
                      "prime map")) {
            cudaArray_t a0 = nullptr, a1 = nullptr;
            const unsigned char z0 = 0;          // 1 像素占位写（内容无意义）
            const unsigned char z1[2] = {0, 0};
            if (cudaGraphicsSubResourceGetMappedArray(&a0, prime[0], 0, 0) ==
                cudaSuccess)
                cudaMemcpy2DToArray(a0, 0, 0, &z0, 1, 1, 1,
                                    cudaMemcpyHostToDevice);
            if (cudaGraphicsSubResourceGetMappedArray(&a1, prime[1], 0, 0) ==
                cudaSuccess)
                cudaMemcpy2DToArray(a1, 0, 0, z1, 2, 2, 1,
                                    cudaMemcpyHostToDevice);
            cudaGraphicsUnmapResources(2, prime, 0);
        }
    }
    return true;
}

void GlCudaInterop::unregisterTextures() {
    // cudaGraphicsUnregisterResource 对 nullptr 无害——由安全包装统一处理
    for (int g = 0; g < 2; ++g) {
        if (luma_res_[g] != nullptr) {
            cudaGraphicsUnregisterResource(
                static_cast<cudaGraphicsResource*>(luma_res_[g]));
            luma_res_[g] = nullptr;
        }
        if (chroma_res_[g] != nullptr) {
            cudaGraphicsUnregisterResource(
                static_cast<cudaGraphicsResource*>(chroma_res_[g]));
            chroma_res_[g] = nullptr;
        }
    }
}

// ---------------------------------------------------------------------------
// uploadNv12 —— NV12 显存帧 → 指定组已注册 GL 纹理对（map → D2D → unmap）
// ---------------------------------------------------------------------------
bool GlCudaInterop::uploadNv12(const AVFrame* hw, int group) {
    if (group < 0 || group > 1) return false; // 组索引防御
    if (!ready_ || hw == nullptr || luma_res_[group] == nullptr ||
        chroma_res_[group] == nullptr)
        return false;

    // 【瞬卡探针 2026-11-05】分段耗时：map / D2D 拷贝 / unmap——定位渲染
    // 线程丢帧的 GPU 层来源（map 的 GL↔CUDA 同步等待 / D2D 与 NVDEC 争
    // 带宽 / unmap 的 GL 命令流同步）。MS_CANVAS_DBG=1 门控；>8ms 分段
    // 打点（200ms 节流——60fps 下正常段 <2ms，超时即异常证据）。
    const auto t0 = std::chrono::steady_clock::now();

    // ---- map（把已注册 GL 纹理暴露为 CUDA 可写的 cudaArray）--------------
    // 【驱动同步】map 期间 GL 对这两张纹理的访问被暂停——unmap 返回后
    // GL 侧可见新内容（CUDA 运行时在 unmap 内与 GL 命令流同步）。
    // 【双缓冲】本帧 map 的是"另一组"纹理——上一帧 draw 采样的是本组
    // 之外的纹理，隐式同步（等 GL 排空）不再阻塞在上一帧绘制上。
    cudaGraphicsResource* res[2] = {
        static_cast<cudaGraphicsResource*>(luma_res_[group]),
        static_cast<cudaGraphicsResource*>(chroma_res_[group])};
    if (!checkCuda(cudaGraphicsMapResources(2, res, 0),
                   "cudaGraphicsMapResources"))
        return false;
    const auto t1 = std::chrono::steady_clock::now(); // map 完成

    bool ok = true;
    cudaArray_t arr_luma = nullptr, arr_chroma = nullptr;
    if (ok) {
        ok = checkCuda(cudaGraphicsSubResourceGetMappedArray(
                           &arr_luma, res[0], 0, 0),
                       "getMappedArray(luma)");
    }
    if (ok) {
        ok = checkCuda(cudaGraphicsSubResourceGetMappedArray(
                           &arr_chroma, res[1], 0, 0),
                       "getMappedArray(chroma)");
    }

    // ---- D2D 拷贝：Y 平面（w×h R8）+ UV 平面（w/2×h/2 RG8 交错）----------
    // 源是 FFmpeg CUDA 硬解帧的显存指针（data[0]/data[1]，CUdeviceptr），
    // cudaMemcpyKind = DeviceToDevice——像素全程不离开 GPU（0 拷贝本体）。
    // 【宽度单位】width 参数按字节计：Y 平面 1 字节/texel → w 字节；
    // UV 平面 RG8 2 字节/texel × (w/2) texel = w 字节（height 减半）。
    if (ok) {
        ok = checkCuda(
            cudaMemcpy2DToArray(arr_luma, 0, 0,
                                hw->data[0], static_cast<size_t>(hw->linesize[0]),
                                static_cast<size_t>(hw->width),
                                static_cast<size_t>(hw->height),
                                cudaMemcpyDeviceToDevice),
            "memcpy luma (D2D)");
    }
    if (ok) {
        ok = checkCuda(
            cudaMemcpy2DToArray(arr_chroma, 0, 0,
                                hw->data[1], static_cast<size_t>(hw->linesize[1]),
                                static_cast<size_t>(hw->width),
                                static_cast<size_t>(hw->height / 2),
                                cudaMemcpyDeviceToDevice),
            "memcpy chroma (D2D)");
    }

    // ---- unmap（GL 侧恢复纹理可见性；拷贝失败也要 unmap 防句柄泄漏）------
    const auto t2 = std::chrono::steady_clock::now(); // 拷贝完成（unmap 前）
    cudaGraphicsUnmapResources(2, res, 0);

    // 【探针】分段耗时打点（map / 拷贝 / unmap 任一 >8ms 才打印；200ms 节流）
    {
        const auto t3 = std::chrono::steady_clock::now(); // unmap 完成
        static const bool s_dbg =
            std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            const auto us = [](auto a, auto b) {
                return std::chrono::duration_cast<std::chrono::microseconds>(
                           b - a)
                    .count();
            };
            const std::int64_t map_us   = us(t0, t1);
            const std::int64_t copy_us  = us(t1, t2);
            const std::int64_t unmap_us = us(t2, t3);
            if (map_us > 8'000 || copy_us > 8'000 ||
                unmap_us > 8'000) {
                static std::int64_t s_last_log_ms = 0;
                const std::int64_t now_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        t3.time_since_epoch())
                        .count();
                if (now_ms - s_last_log_ms > 200) {
                    s_last_log_ms = now_ms;
                    std::fprintf(stderr,
                                 "[interop-probe] map=%lldus copy=%lldus "
                                 "unmap=%lldus（任一 >8ms）\n",
                                 static_cast<long long>(map_us),
                                 static_cast<long long>(copy_us),
                                 static_cast<long long>(unmap_us));
                    std::fflush(stderr);
                }
            }
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// shutdown —— 全资源释放（幂等；渲染线程调用）
// ---------------------------------------------------------------------------
void GlCudaInterop::shutdown() {
    unregisterTextures();
    ready_     = false;
    device_id_ = -1;
}

} // namespace ms

#endif // MS_HAS_CUDA
