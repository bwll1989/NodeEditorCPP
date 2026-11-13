// ============================================================================
//  GlRenderer.hpp —— OpenGL 3.3 core 合成后端（IComposeRenderer 的 GPU 实现）
//
//  设计依据：《hw-decode-implementation-plan.md》②-3（GlRenderer）
//    * 与 SoftRenderer 消费同一 DrawItem/FrameView、实现同一接口——
//      RenderLoop/ScreenMapper 解算层零改动切换后端（后端可替换契约）；
//    * 窗口与 GL 上下文（GlContextWGL）在首次 beginFrame 懒初始化——
//      beginFrame 由渲染线程调用，天然满足 GL 上下文的线程亲和；
//      初始化失败（无桌面/驱动过旧）→ failed_ 置位，draw 全部跳过，
//      同步输出不受影响（headless 语义与 GDI 预览窗降级一致）。
//
//  【坐标系与防上下反】（本次实现的核心风险点，顶点数学以此为准）：
//    * 数据域：DrawItem.x/y/w/h 是本机输出像素、y 向下（top-down）；
//      FrameView.rgba 行主序 top-down（texel row0 = 画面顶）；
//      GL 窗口坐标 y 向上（gl_y=0 在底）。
//    * 换算：顶点 gl_y = H − y_topdown（H = 画布高，整数运算零误差）；
//      投影 uniform u_outSize=(W,H)：ndc = pos/(W,H)*2−1（ortho 等价）。
//    * 纹理：帧数据按行序直传（row0 先传 = GL 的 v=0），画面顶在 v=0；
//      顶点 uv：目标矩形顶边 v=0、底边 v=1 —— 链条"texel row0(画面顶)
//      = v0 → 目标顶边贴画面顶 → gl_y 大 = NDC +1 = 窗口视觉顶"闭环，
//      正立不翻转。
//    * 采样语义：顶点坐标取像素【边界】整数，uv 在顶点处取仿射映射值——
//      GL 像素中心线性插值恰等于 SoftRenderer 的 M(px+0.5, py+0.5)
//      （线性函数插值一致性），1:1 时与最近邻逐点重合。
//
//  合成语义复刻（对拍基准 SoftRenderer）：
//    * 混合：out = src*a + dst*(1-a)，a = tex_a × opacity —— GL 用
//      glBlendFuncSeparate(SRC_ALPHA, ONE_MINUS_SRC_ALPHA, ONE,
//      ONE_MINUS_SRC_ALPHA)（alpha 通道公式也与 SoftRenderer 一致）；
//    * 出界丢弃：fragment 中 uv×frameSize 落在 [-0.5, fw+0.5] 容差外
//      discard（旋转角外露底色，等价 SoftRenderer 仿射路径的出界判定）；
//    * 边缘采样：CLAMP_TO_EDGE（等价 SoftRenderer 的帧内 clamp）；
//    * 差异声明：GL 用双线性滤波（SoftRenderer 最近邻）——缩放时边缘
//      ±1 texel 级差异属预期；alpha 经浮点管线（±1/255 舍入差）。
//
//  性能注记（②阶段定位：合成管线正确性优先）：
//    * 帧纹理每次 draw 全量上传（glTexSubImage2D）——视频帧内容逐帧
//      变化本就必须重传；静态图片的重复上传由③阶段（CUDA interop +
//      帧代次脏标记）统一优化，此处不做半吊子缓存；
//    * vsync 关闭（GlContextWGL），呈现相位由 FramePacer 栅格控制。
// ============================================================================

#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "GlContextWGL.hpp"
#ifndef MS_GL_NO_CUDA
#include "GlCudaInterop.hpp" // CUDA-GL 互操作（③阶段 0 拷贝；含恒失败桩）
#endif
#include "engine/IComposeRenderer.hpp"

struct AVFrame; // FFmpeg 前置声明（完整类型仅 .cpp 可见）

namespace ms {

/// 【MS_GL_NO_CUDA 宏】编译掉 NV12/CUDA interop 全部分支——editor 目标
/// 专用（media_editor 复用 GlRenderer 做预览 GL 合成时不需要也不允许
/// 引入 CUDA 头：③阶段实证 CUDA 头在 Windows 直含 GL/gl.h，对宿主
/// TU 的 include 顺序敏感；editor 侧零 CUDA 依赖彻底规避）。
/// display 目标不定义该宏——NV12 0 拷贝路径原样保留，行为零变化。

class GlRenderer final : public IComposeRenderer {
public:
    using WindowGeom = GlContextWGL::WindowGeom;

    /// 【外部上下文模式标记】ExternalCtx 构造的宿主（QOpenGLWidget 等）
    /// 自行创建并 makeCurrent GL 上下文（Qt paintGL 内 context 恒当前），
    /// GlRenderer 不建窗口、不自管上下文生命周期。
    struct ExternalCtxTag {};

    /// \param geom 输出窗口几何（display 端沿用预览窗参数语义）
    /// \param gpu_delivery_ok CUDA 0 拷贝协商旗标（装配层创建，与解码端
    ///        共享同一 shared_ptr；nullptr = GPU 交付未启用——GpuCudaNv12
    ///        帧按跳过处理，等价旧②阶段行为）
    explicit GlRenderer(WindowGeom geom,
                        std::shared_ptr<std::atomic_bool> gpu_delivery_ok
                        = nullptr)
        : geom_(std::move(geom)) {
#ifndef MS_GL_NO_CUDA
        gpu_delivery_ok_ = std::move(gpu_delivery_ok); // CUDA 0 拷贝协商旗标
#else
        (void)gpu_delivery_ok; // editor 目标：无 NV12 路径，旗标无处可用
#endif
    }

    /// 【ExternalCtx 模式构造】不建窗口、不建上下文——假定每次 draw 系
    /// 方法调用前宿主已 makeCurrent（QOpenGLWidget::paintGL 内成立）。
    ///   * ensureInit 跳过 ctx_.create，改为经 gl::loadGl() 填充函数表
    ///     （当前上下文是 Qt 创建的 3.3+ context，wglGetProcAddress 可用）；
    ///   * beginFrame 的 (w,h) 默认即视口尺寸（不再查窗口客户区），可用
    ///     setExternalViewport 指定完整视口矩形（合成内容贴进 FBO
    ///     的局部区域，其余区域保持清屏色）；
    ///   * endFrame 只 glFlush 不 SwapBuffers（呈现由 Qt 合成器负责）。
    /// \param gpu_delivery_ok CUDA 0 拷贝协商旗标（editor 预览：与
    ///        PreviewRenderer 的解码源共享同一 shared_ptr——见
    ///        PreviewRenderer::gpuDeliveryFlag；nullptr = 未启用）
    explicit GlRenderer(ExternalCtxTag,
                        std::shared_ptr<std::atomic_bool> gpu_delivery_ok
                        = nullptr)
        : external_(true) {
#ifndef MS_GL_NO_CUDA
        gpu_delivery_ok_ = std::move(gpu_delivery_ok);
#else
        (void)gpu_delivery_ok; // editor 目标（若仍定义宏）：无处可用
#endif
    }

    ~GlRenderer() override; ///< GL 资源释放（须与使用同线程——渲染线程）

    // ---- IComposeRenderer ------------------------------------------------
    /// 启动即建窗：立即创建输出窗口+GL 上下文+程序（不等首帧）。
    /// 【2026-11-04 回归修复】旧惰性行为下无 Editor 接入（无场景 → 渲染
    /// 循环早退 → beginFrame 永不触发）时窗口永不出现，display 只剩终端。
    /// 幂等：inited_/failed_ 已就绪/已失败时直接短路。
    /// ExternalCtx 模式：无窗口可预建，恒 true（程序懒建在 beginFrame）。
    bool ensureReady() override;
    /// 消息泵转发（渲染线程每帧；无场景早退路径的窗口响应兜底）。
    /// ExternalCtx 模式：窗口归 Qt 管理，空操作。
    void pumpOsMessages() override;
    void beginFrame(int w, int h) override;
    bool draw(const DrawItem& di, const FrameView& frame) override;
    void endFrame() override;
    bool readPixels(std::vector<std::uint8_t>& out_rgba,
                    int& out_w, int& out_h,
                    std::uint32_t& out_stride) override;

    /// 输出变形网格（WATCHOUT 式手动网格变形；2026-11-09）。
    /// 【ExternalCtx 忽略】变形是 display 物理输出端能力——编辑器预览
    /// 画布恒 1:1 直显，宿主模式下静默丢弃。
    void setWarp(const WarpSpec& warp) override;

    /// 实际输出表面尺寸（显示分辨率模型 2026-11-10）：
    ///   * 窗口模式：GlContextWGL 客户区（WM_SIZE 实时维护——resize 下一
    ///     帧即跟随；全屏铺满 = 设备物理分辨率，输出恒点对点 1:1）；
    ///   * ExternalCtx 模式恒 0（宿主 QOpenGLWidget 自管视口，上层按
    ///     cfg 显式尺寸合成——预览端语义不受本模型影响）；
    ///   * 窗口未创建/已销毁时 0（上层走兜底，同旧行为）。
    int outputWidth()  const override;
    int outputHeight() const override;

    bool inited()  const noexcept { return inited_; }
    bool failed()  const noexcept { return failed_; }
    /// CUDA 0 拷贝 interop 是否就绪（诊断/测试断言用；未启用恒 false）
#ifndef MS_GL_NO_CUDA
    bool gpuInteropReady() const noexcept { return interop_.ready(); }
#else
    bool gpuInteropReady() const noexcept { return false; }
#endif

    /// 【ExternalCtx 专用】帧视口（GL 像素坐标，左下原点）：下次
    /// beginFrame 起视口 = (x, y, w, h)【完整矩形，2026-11-05 修复】——
    /// 合成内容贴进 FBO 子矩形（如预览画布的"内容域映射区域"），其余
    /// 区域保持清屏色。旧版只传偏移、尺寸沿用 beginFrame 参数（=comp
    /// 域尺寸），当宿主内容域映射矩形 ≠ comp 尺寸时（如 dom 含出界
    /// 素材外扩、或 dpr/fit 缩放）内容被错误压缩/偏移（联测实证
    /// 818/929=0.88 缩放伪影）。窗口模式无效（恒视口=客户区）。
    void setExternalViewport(int x, int y, int w, int h) noexcept {
        vp_x_ = x;
        vp_y_ = y;
        vp_w_ = w;
        vp_h_ = h;
    }

private:
    /// 懒初始化：窗口+上下文+程序+VAO/VBO+纹理（首次 beginFrame 调用）
    bool ensureInit();

    /// 编译/链接着色器程序（失败打日志返回 false）
    bool buildProgram();

#ifndef MS_GL_NO_CUDA
    /// NV12（YUV→RGB BT.601 limited）着色器程序（GPU 帧路径；惰性构建）
    bool ensureNv12Program();

    /// NV12 纹理对 ensure：按帧尺寸创建 R8/RG8 纹理并注册进 CUDA
    /// （尺寸变化 → 重建 + 重注册；失败返回 false 由调用方回退）
    bool ensureNv12Textures(int w, int h);

    /// 翻转 CUDA 0 拷贝协商旗标（首次失败打日志）：解码端下一帧起自动
    /// 回退 av_hwframe_transfer_data 下载路径；本地 gpu_dead_ 快速失败
    /// ——不再反复尝试 map/unmap（占位帧自愈窗口 <1s，全程无黑屏）
    void disableGpuDelivery(const char* why);
#endif

    /// 单帧绘制项的顶点构造（RGBA/NV12 双路径共用；失败 = 防御拒绝）
    /// \param verts 输出 {glx, gly, u, v}×4 条带序（左上→左下→右上→右下）
    bool buildVerts(const DrawItem& di, int frame_w, int frame_h,
                    float verts[4][4]);

    void drawQuad(const float verts[4][4]); ///< 顶点上传 + 条带绘制

    // ---- 输出变形（两段式渲染：FBO 离屏合成 + mesh 贴图；2026-11-09）-----
    /// 变形管线是否应激活（warp 有效 && 非 External && FBO 未失败）。
    /// 任一条件不满足即退回普通直出路径——变形失败绝不黑屏。
    bool warpActive() const noexcept;

    /// FBO（离屏画布）确保就绪：按需创建/重建 RGBA 纹理 + 帧缓冲对象。
    /// \return false = 创建/完整性校验失败（warp_failed_ 置位，本会话
    ///         永久降级直出——不逐帧重试避免日志刷屏）
    bool ensureFbo(int w, int h);

    /// 变形呈现：解绑 FBO → 按网格把 FBO 纹理贴到后备缓冲（mesh 三角形）。
    /// 调用点：endFrame（SwapBuffers 前）与 readPixels（诊断导帧须读到
    /// 变形后画面——RenderThread 的 maybeCapture 在 endFrame 之前）。
    /// 幂等（warped_this_frame_ 守卫）：一帧内多次调用只生效首次。
    void warpBlit();

    /// 构建变形 mesh 顶点数组（非索引三角形：每 quad 6 顶点）。
    /// 顶点 {glx, gly, u, v}：位置 = 节点归一化坐标 × 后备缓冲尺寸
    /// （gl_y 翻转）；uv = 规则网格（内容顶 = v=1，FBO 渲染产物惯例）。
    void buildWarpMesh(std::vector<float>& out_verts);

    // ---- 窗口与上下文（渲染线程独占）-------------------------------------
    WindowGeom   geom_;      ///< 窗口几何（构造传入）
    GlContextWGL ctx_;       ///< 窗口 + 3.3 core 上下文（懒创建）

    // ---- 宿主模式（WindowOwnCtx=display 现状 / ExternalCtx=Qt paintGL）---
    bool external_  = false; ///< ExternalCtx 模式（不建窗口/不自管上下文）
    bool gl_loaded_ = false; ///< 函数表已加载（External 模式懒加载一次）
    int  vp_x_      = 0;     ///< External 帧视口偏移 X（GL 像素，左下原点）
    int  vp_y_      = 0;     ///< External 帧视口偏移 Y
    int  vp_w_      = 0;     ///< External 帧视口宽（0 = 沿用 beginFrame 尺寸）
    int  vp_h_      = 0;     ///< External 帧视口高（0 = 沿用 beginFrame 尺寸）

    // ---- GL 对象（ensureInit 创建；析构销毁——同线程契约）---------------
    unsigned int program_ = 0; ///< 合成着色器程序（RGBA 域）
    unsigned int vao_ = 0;     ///< 顶点数组（quad 布局）
    unsigned int vbo_ = 0;     ///< 顶点缓冲（每 draw 动态更新 64B）
    unsigned int tex_ = 0;     ///< 帧纹理（单槽复用，每 draw 全量上传）
    int          tex_w_ = 0;   ///< 纹理当前分配宽（变化时重建）
    int          tex_h_ = 0;

    // ---- NV12 GPU 帧路径（③阶段 0 拷贝；惰性创建；MS_GL_NO_CUDA 编译掉）--
#ifndef MS_GL_NO_CUDA
    unsigned int program_nv12_ = 0; ///< NV12→RGB 着色器程序（BT.601 limited）
    // 【双缓冲纹理对 2026-11-05】两组 Y/UV 纹理按帧奇偶交替使用——单组时
    // 本帧 cudaGraphicsMapResources 隐式等待 GPU 画完上一帧对同组纹理的
    // 采样（GL↔CUDA 交接驱动语义），渲染管线逐帧串行化（实测 map 恒
    // ≈16ms ≈ 60Hz 节拍 → compose 顶破 16.6ms 帧预算 → 58-59fps）。
    // 交替后 map 落在"另一组"上，上一帧 draw 早已完成——map 回亚毫秒。
    unsigned int tex_y_[2]  = {0, 0}; ///< NV12 亮度纹理×2（R8；双组交替）
    unsigned int tex_uv_[2] = {0, 0}; ///< NV12 色度纹理×2（RG8；双组交替）
    int          nv_parity_ = 0;      ///< 本帧使用的纹理组索引（draw 后翻转）
    int          tex_y_w_ = 0;        ///< NV12 纹理分配宽（变化重建+重注册）
    int          tex_y_h_ = 0;
    GlCudaInterop interop_;         ///< CUDA-GL 互操作封装（探测/注册/D2D）
    std::shared_ptr<std::atomic_bool> gpu_delivery_ok_; ///< 协商旗标（共享）
    bool gpu_dead_ = false;         ///< 本地快速失败标记（旗标翻转后不再重试）
#endif
    bool swap_fail_logged_ = false; ///< SwapBuffers 失败已告警标记（一次性；
                                    ///  渲染线程独占——屏幕恒黑诊断 2026-11-04）

    // ---- 输出变形（两段式渲染资源；2026-11-09）---------------------------
    // 帧流程（warpActive 时）：beginFrame 绑 FBO（合成照旧画进离屏）→
    // endFrame/readPixels 解绑后按 mesh 把 FBO 纹理贴到后备缓冲 → 呈现。
    // 任何环节失败 → warp_failed_ 永久降级直出（变形绝不黑屏）。
    WarpSpec     warp_;             ///< 当前变形网格（setWarp 注入，渲染线程独占）
    bool         warp_dirty_ = false; ///< 网格变化标记（首帧激活/更新用）
    bool         warp_failed_ = false; ///< FBO 创建失败（本会话永久降级直出）
    unsigned int fbo_     = 0;     ///< 离屏帧缓冲对象
    unsigned int fbo_tex_ = 0;     ///< 离屏颜色纹理（合成画布 1:1 内容）
    int          fbo_w_   = 0;     ///< FBO 当前分配宽（变化时重建）
    int          fbo_h_   = 0;     ///< FBO 当前分配高
    bool warped_this_frame_ = false; ///< 本帧已执行变形呈现（幂等守卫——
                                    ///  readPixels 先到 blit 后 endFrame 不重复）
    std::vector<float> warp_mesh_;  ///< mesh 顶点缓存（复用容量，逐帧重填）

    // ---- 实际呈现帧率统计（MS_CANVAS_DBG=1 门控打印；渲染线程独占）----
    // 【2026-11-05 新增】player 诊断心跳的 frame 字段是播放头换算值
    // （t/16.6ms）而非实际渲染帧数——量化"display 卡顿/预览流畅"必须有
    // SwapBuffers 真实计数。5s 窗口计数打印（与 player 诊断心跳同节拍）。
    long long  present_count_ = 0;   ///< 窗口内 SwapBuffers 计数
    long long  present_total_ = 0;   ///< 累计呈现帧数（进程生命周期）
    std::int64_t present_win_t0_ = 0;///< 窗口起点（steady ms；0=未初始化）

    // ---- 呈现层病态自适应（2026-11-05）----------------------------------
    // 语义：SwapBuffers 是同步阻塞调用，vsync(1) 下正常 ≤16.7ms。远程桌面 /
    // 虚拟显示器 / Optimus 跨 GPU 呈现链路上实测单帧阻塞 250ms（整机 5fps，
    // 无论合成多快都上不去）——渲染循环被呈现层反压，用户观感即"editor 改
    // 了、display 半天才动"。这里对 swap 耗时做 EMA：稳态超阈值（>60ms）
    // 判定病态，一次性降级 vsync(0)，把节拍交还 FramePacer（Pacer 的栅格
    // 对齐语义不依赖 vsync）；只降不升，避免健康/病态间反复抖动。
    // 阈值取 60ms 而非 30ms：30Hz 输出的正常 vsync 单帧就有 33ms，阈值过低
    // 会把"低刷新率屏幕"误判为病态；实测病态值在 125~250ms，60ms 足够区分。
    // MS_GL_SWAP_INTERVAL 显式指定时尊重用户意图，不自动降级。
    std::int64_t swap_ema_us_   = 0; ///< swap 耗时 EMA（α=1/16；0=无样本）
    long long    swap_samples_  = 0; ///< 累计样本数（<30 不判定，避开启动抖动）
    bool         vsync_downgraded_ = false; ///< 已自动降级（一次性）

    // ---- uniform location 缓存（程序链接成功时查询一次）-----------------
    // 【性能】glGetUniformLocation 是字符串查找，draw 热路径逐帧逐项调用
    // 纯属浪费（4K@60 帧每秒数千次无效查找）——location 在程序生命周期
    // 内不变，构建期缓存后 draw/beginFrame 零查找。
    int loc_out_size_      = -1; ///< program_ 投影（u_outSize）
    int loc_tex_           = -1; ///< program_ 采样器（u_tex）
    int loc_frame_size_    = -1; ///< program_ 帧尺寸（u_frameSize）
    int loc_opacity_       = -1; ///< program_ 不透明度（u_opacity）
#ifndef MS_GL_NO_CUDA
    int loc_nv_out_size_   = -1; ///< program_nv12_ 投影（u_outSize）
    int loc_nv_frame_size_ = -1; ///< program_nv12_ 帧尺寸（u_frameSize）
    int loc_nv_opacity_    = -1; ///< program_nv12_ 不透明度（u_opacity）
#endif

    // ---- 帧状态（beginFrame → endFrame 之间有效）------------------------
    int canvas_w_ = 0;         ///< 本帧画布宽（beginFrame 参数；投影域）
    int canvas_h_ = 0;         ///< 本帧画布高（顶点 y 翻转用）
    bool inited_ = false;      ///< 懒初始化完成
    bool failed_ = false;      ///< 初始化失败（此后全部跳过，headless 兜底）
};

} // namespace ms
