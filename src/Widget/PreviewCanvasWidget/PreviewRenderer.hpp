// ============================================================================
//  PreviewRenderer.hpp —— Editor 预览所见即所得渲染器（第七迭代：解码线程化）
//
//  职责：把 display 端的渲染管线（解码→调度→映射→合成）以【异步拉模式】
//  接入 Editor 预览画布——GUI 线程零解码零合成：
//    * buildPreviewDrawList（整画布视作一块虚拟屏，多屏布局拼合显示；
//      可见性/有效尺寸/z 序与正式渲染同源，两端同帧同结论）——在 GUI 线程
//      执行（纯几何解算，微秒级，读 Scene 无跨线程竞争）；
//    * MediaScheduler 逐项 produce（本地独立解码实例，与 display 端隔离）
//      + SoftRenderer 软件合成——在工作线程执行（重活全部离场）；
//    * paintEvent 只取"最新合成完成的帧"上屏（互斥快照，零阻塞）。
//
//  播放头状态机（本地独立时钟，不依赖 display 在线；语义对齐
//  EditorRuntime::cmdPlay：Idle → 从头播，Paused → 从暂停点续播）：
//    play()   Idle→从头 / Paused→从暂停点续播（幂等：Playing 时不重置）
//    pause()  冻结当前播放头
//    stop()   归零回 Idle
//    seekTo() 任意态跳转（播放态重置推进锚点，不中断播放）
//  【显示端解耦（2026-11-03）】Editor 集成采用"外部喂时"模式：不调
//  play/tick/pause，播放头由 EditorWindow::echoSessionClock 统一喂
//  seekTo(会话时钟)——预览与时间轴游标同源同值（单一时间权威），
//  无显示端也照常预览。play/tick/pause 保留给独立使用与状态机测试
//  （PreviewRendererTest 验证语义）。
//
//  线程模型（对齐 display 端 RenderThread 架构，2026-11-03 卡死终修）：
//    * GUI 线程：状态机 + 请求提交 + 结果取用。render() 检测输入变化
//      （内容版本 rev / 播放头 / 合成尺寸）→ 构建绘制列表快照（DrawItem
//      标量 + Asset 值拷贝，不借用 Scene 内部指针）→ 投递最新请求（单槽
//      覆盖，天然合并风暴）→ 返回最近交付帧（可能滞后一拍）。
//    * 工作线程：等待请求 → 快照拷贝 → 逐项解码+合成 → 深拷贝结果帧 →
//      经 frame_ready 回调（跨线程！）通知 GUI 排队重绘。worker 永远只
//      渲染最新请求：拖动播放头时旧请求作废，不堆积延迟。
//    * MediaScheduler/SoftRenderer 单线程契约由"只在 worker 使用"满足；
//      clearMediaCache 的 scheduler_.clear() 经标志位转发到 worker 执行。
//    * Scene 读取仅在 GUI 线程（buildPreviewDrawList）；worker 只接触
//      快照，与工作副本的并发编辑（拖拽/属性修改）完全解耦。
//
//  降级语义：素材类型未注册（Text/Audio）或解码失败 → 该片段区域保持
//  画布底色（与 display 端"素材缺失渲染黑帧"行为一致）；无场景/画布
//  非法时 render 返回 false，画布回退棋盘格占位画法。
// ============================================================================

#pragma once

#include <chrono>
#include <atomic>
#include <cstdint>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <QImage>
#include <QRect>

#include "engine/MediaFactory.hpp"
#include "engine/MediaScheduler.hpp"
#include "engine/ScreenMapper.hpp"   // buildPreviewDrawList + DrawItem
#include "engine/SoftRenderer.hpp"
#include "media/HwAccel.hpp"         // 硬解配置（第三步：cuda 硬解进预览）
#include "model/SceneModel.hpp"

class Scene; // ms::Scene 前置（ms 命名空间在 include 中展开）

namespace ms {

class PreviewRenderer {
public:
    /// 【交付模式】worker 产出的帧交付形态：
    ///   * SoftCompose —— 现状全保留：worker 端 SoftRenderer CPU 合成
    ///     完整画布帧，pull(QImage) 取整帧（测试基准路径 + 回退兜底）；
    ///   * GpuDeliver —— GL 直通（2026-11-04 预览管线 GL 化）：worker 只
    ///     逐项解码并快照各片段【源帧】（DrawItem 标量 + QImage 深拷贝），
    ///     合成挪到 GUI paintGL 内由 GlRenderer(ExternalCtx) 完成——
    ///     解码照旧离场 GUI，合成从 CPU 逐像素（4.6ms@1080p）变为 GPU
    ///     shader，且消除整帧深拷贝的 8MB 传递（逐片段源帧通常更小）。
    enum class DeliverMode : std::uint8_t {
        SoftCompose = 0, ///< CPU 合成整帧（缺省；与历史行为逐字节一致）
        GpuDeliver  = 1, ///< 逐项源帧交付，paintGL 内 GL 合成直呈
    };

    /// \param assets_root 素材根目录（与 display 端同源 IniConfig [common]；
    ///        空 = 不注册解码器，全部片段占位降级）
    /// \param clock_fn 时钟读法（对齐 ClockProvider.alignedNowUs 同域；
    ///        缺省用 steady 时钟——测试可注入虚拟时钟）
    /// \param mode 交付模式（缺省 SoftCompose=历史行为；装配点按 GL 探测
    ///        结果选择 GpuDeliver——见 EditorWindow 装配注释）
    explicit PreviewRenderer(
        std::string assets_root,
        std::function<std::int64_t()> clock_fn = {},
        DeliverMode mode = DeliverMode::SoftCompose);

    /// 析构：停工作线程（置退出标志 + join；join 前回调不会再触发，
    /// 保证持有回调引用的一方安全销毁）
    ~PreviewRenderer();

    PreviewRenderer(const PreviewRenderer&)            = delete;
    PreviewRenderer& operator=(const PreviewRenderer&) = delete;

    // ---- 场景绑定（工作副本外部拥有；指针须持续有效）----
    void setScene(const Scene* s);

    // ---- 播放状态机（语义对齐 EditorRuntime::cmdPlay/cmdPause/cmdStop）----
    void play();
    void pause();
    void stop();
    void seekTo(std::int64_t us);
    bool playing() const noexcept { return playing_; }
    std::int64_t playheadUs() const noexcept { return playhead_us_; }

    /// 播放中推进播放头（QTimer ~33ms 驱动；非播放态无操作）
    /// \return 播放头是否发生变化（调用方据此决定是否重绘画布）
    bool tick();

    // ---- 引擎驱动接口（GUI 泵定时调用；对齐 display 端"渲染循环独立、
    // ---- 窗口只拉帧"的架构，2026-11-03 第八迭代）--------------------------
    /// 【泵】状态变化检测 + 快照提交：输入（内容版本/播放头/合成尺寸）相对
    /// 上次提交有变化时，构建绘制列表快照并投递最新请求。内置自适应节流：
    /// 距上次提交不足 max(33ms, 上轮渲染耗时) 时跳过——解码慢时自动降帧，
    /// worker CPU 占用自适应（预览帧率让位于整机流畅）。
    /// 调用节奏由调用方定时器决定（预览画布 ~33ms 拉帧泵）。
    void pump();

    /// 【拉帧】取最新合成完成帧（不触发任何解码/提交；尚无完成帧返回
    /// false，画布回退占位画法）。worker 出帧经 frame_ready 回调触发重绘。
    /// \param out 输出 QImage（最近交付帧的共享拷贝）
    /// \param domain 可空出参：本帧覆盖的【画布域矩形】（内容包围盒——
    ///        起点可为负坐标：素材/screen 放到画布负区域也进预览域）。
    ///        画布贴图按此矩形映射（不再是固定的 (0,0,canvas_w,canvas_h)）
    /// 仅 SoftCompose 模式有效（GpuDeliver 走 pullGpu）。
    bool pull(QImage& out, QRectF* domain = nullptr);

    // ---- GpuDeliver 交付形态（GL 直通路径）---------------------------------
    /// 逐项交付单元：绘制项标量快照（affine/rotation/canvas_*/inv_map/
    /// opacity 全在——GL 合成所需信息齐备；clip/screen/asset 指针恒空）
    /// + 该片段解码出的源帧：
    ///   * CPU 域（软解/硬解下载回退）：frame 携带 RGBA8888 深拷贝；
    ///   * GPU 域（③阶段 cuda 硬解 0 拷贝，2026-11-05 第三步）：native
    ///     携带 AVFrame 保活引用（data[] = CUDA 显存指针），frame 为空
    ///     ——paintGL 内 interop D2D 直进纹理，零下载。
    struct DeliverItem {
        DrawItem item;  ///< 绘制项（合成域坐标；见 Request.w/h）
        /// clip 身份（对应请求 clip_ids[i]）：paintGL3D 贴图按 id 精确
        /// 匹配场景片段——几何最近匹配在同层邻近片段间会错配（A 的帧
        /// 画到 B 的面片上），id 匹配未命中即棋盘格占位（宁缺勿错）
        std::string clip_id;
        QImage   frame; ///< CPU 源帧（Format_RGBA8888；行距=width*4 无填充）
        /// GPU 域句柄（AVFrame 引用计数保活；非空 = GpuCudaNv12 交付）
        std::shared_ptr<const void> native;
        int gpu_w = 0, gpu_h = 0; ///< GPU 帧尺寸（native 非空时有效）
    };
    /// 【拉帧·GL 直通】取最新逐项交付（与 pull 同一"新帧才拷出"语义）：
    /// \param out_items 本帧全部可绘片段（z 序=合成序，逐项 draw 即可）
    /// \param out_w/out_h 合成域尺寸（绘制项坐标系；beginFrame 参数）
    /// \param out_domain 本帧内容域（画布坐标，可负起点——控件贴图映射用）
    /// \return false = 尚无完成帧（画布回退占位/软路径）
    bool pullGpu(std::vector<DeliverItem>& out_items,
                 int& out_w, int& out_h, QRectF& out_domain);

    /// 交付模式（装配只读；worker 按模式走不同产出分支）
    DeliverMode deliverMode() const noexcept { return mode_; }

    /// 【CUDA 0 拷贝协商旗标】GpuDeliver 模式下构造时乐观置位（与解码源
    /// 共享同一 shared_ptr——interop 探测失败渲染端翻 false，解码端下一帧
    /// 自动回退下载路径）。nullptr = 硬解未启用（SoftCompose/MS_PREVIEW_HW
    /// =none）。SceneCanvasWidget 建预览 GlRenderer(ExternalCtx) 时取用
    /// ——两端同旗标是 0 拷贝协商的前提（发布端/消费端必须同为已协商态）。
    std::shared_ptr<std::atomic_bool> gpuDeliveryFlag() const noexcept {
        return hw_cfg_.gpu_delivery_ok;
    }

    /// 场景内容变化后调用：递增内容版本（拖拽片段/属性编辑后由
    /// SceneCanvasWidget::refresh 统一触发；下一次 render 提交新请求）
    void invalidate();

    /// 合成降采样上限（画布 setTargetSize 通知预览显示区域像素数；
    /// 合成尺寸 = 画布等比缩到上限内，<=0 或上限≥画布 = 1:1 不缩）
    /// 仅在尺寸真变化时递增内容版本（paintEvent 每帧都会调用）
    void setTargetSize(int max_w, int max_h);

    /// 素材表变化后清空解码缓存（导入/删除素材后调用）：
    /// 清理动作经标志位转发到工作线程执行（scheduler 单线程契约），
    /// 并递增内容版本确保下一帧重建实例
    void clearMediaCache();

    /// 【跨线程】新帧就绪回调——工作线程在合成完成后调用！
    /// 调用方必须自行排队到 GUI 线程（如 QMetaObject::invokeMethod
    /// Qt::QueuedConnection），不得在回调内直接操作 QWidget。
    /// 回调须在析构前可安全置空；本类析构 join 后不再触发。
    void setFrameReadyCallback(std::function<void()> cb);

private:
    // ---- 工作线程消息（锁保护；GUI 写请求 / worker 写结果）----
    struct Request {
        std::vector<DrawItem> items;  ///< 绘制项快照（标量；asset 指针
                                      ///< 指向本快照 assets，不借用 Scene）
        std::vector<Asset>    assets; ///< 素材值拷贝（快照自持，指针稳定）
        /// clip id 平行数组（与 items 下标对齐）：pump 在 clip 指针置空
        /// 前从工作副本收集——worker 组装 DeliverItem 时透传，GUI 侧贴图
        /// 按 id 精确匹配（修复几何最近匹配的错配：文本帧缺失时同层
        /// 邻近视频帧被误配给文本片段，显示别人的内容）
        std::vector<std::string> clip_ids;
        int w = 0, h = 0;             ///< 合成尺寸（降采样后）
        double dom_x = 0.0, dom_y = 0.0; ///< 本帧内容域（画布坐标；预览
                                         ///< 虚拟屏矩形——可为负起点，见
                                         ///< computeContentRect）
        double dom_w = 0.0, dom_h = 0.0;
        std::uint64_t epoch = 0;      ///< 单调递增；worker 只渲染最新
    };
    struct Result {
        QImage frame;                 ///< 深拷贝合成帧（SoftCompose 产出）
        std::vector<DeliverItem> items; ///< 逐项交付（GpuDeliver 产出）
        int comp_w = 0, comp_h = 0;   ///< GpuDeliver 合成域尺寸（items 坐标系）
        std::uint64_t epoch = 0;      ///< 对应请求的代数
        double dom_x = 0.0, dom_y = 0.0; ///< 请求内容域透传（贴图映射用）
        double dom_w = 0.0, dom_h = 0.0;
    };

    /// 合成尺寸解算（内容域钳制 8192 + setTargetSize 等比降采样；GUI 线程）
    void computeSize(int& w, int& h) const;
    /// 内容域解算（GUI 线程）：画布 ∪ 全部 screens ∪ 全部 clips（有效
    /// 尺寸）的轴对齐包围盒——预览虚拟屏。负坐标素材/出画布屏幕也进域
    /// （无限画布语义：预览范围由【内容】决定而非 canvas 矩形）。
    /// 结果写 content_rect_（GUI 线程 only；pump 每次提交前重算）
    void computeContentRect();
    /// 工作线程主循环：等待 → 快照 → 解码合成 → 交付 → 回调
    void workerLoop();

    MediaFactory  factory_;    ///< 本地素材源工厂（Video+Image 注册）
    MediaScheduler scheduler_; ///< 源实例缓存（按 asset_id；仅 worker 使用）
    SoftRenderer  renderer_;   ///< 软件合成（与 display 端同一实现；仅 worker）
    DeliverMode   mode_;       ///< 交付模式（构造传入；只读）
    HwAccelConfig hw_cfg_;     ///< 硬解配置（构造装配；creator 闭包捕获）
    Screen        preview_scr_; ///< 预览虚拟屏（内容包围盒；buildPreviewDrawList 用）
    QRectF        content_rect_{0, 0, 0, 0}; ///< 内容域画布矩形（GUI 线程
                                             ///< computeContentRect 维护）
    QRectF delivered_domain_{0, 0, 0, 0}; ///< 最近交付帧的内容域（pull 出参）

    const Scene* scene_ = nullptr;
    std::function<std::int64_t()> clock_fn_; ///< 时钟读法（注入对齐同域）
    std::string assets_root_;

    // ---- 播放状态机（仅 GUI 线程触碰，无锁）----
    bool         playing_     = false;
    std::int64_t playhead_us_ = 0; ///< 冻结值/推进基准（Paused/Idle 语义值）
    std::int64_t t0_us_       = 0; ///< 本段播放的时钟起点（play/seek 时重置）

    // ---- 请求去重（仅 GUI 线程触碰，无锁）----
    int target_w_ = 0, target_h_ = 0;    ///< 合成降采样上限（显示尺寸）
    std::uint64_t rev_ = 1;              ///< 内容版本（invalidate 等递增）
    std::uint64_t posted_rev_ = 0;       ///< 已投递请求的内容版本
    std::int64_t  posted_playhead_ = -1; ///< 已投递请求的播放头
    int           posted_w_ = 0, posted_h_ = 0; ///< 已投递请求的合成尺寸
    std::uint64_t next_epoch_ = 0;       ///< 请求代数发生器
    std::uint64_t delivered_epoch_ = 0;  ///< 已交付结果的代数
    QImage        delivered_frame_;      ///< 最近交付帧（SoftCompose；GUI 读取）
    std::vector<DeliverItem> delivered_items_; ///< 最近逐项交付（GpuDeliver）
    int           delivered_w_ = 0, delivered_h_ = 0; ///< GpuDeliver 合成域
    std::chrono::steady_clock::time_point last_submit_tp_{}; ///< 上次提交时刻
                                                             ///< （自适应节流）

    // ---- 工作线程共享（mutex_ 保护）----
    std::mutex              mutex_;
    std::condition_variable cv_;
    bool     quit_          = false; ///< 退出标志
    bool     clear_pending_ = false; ///< 待执行的 scheduler_.clear()
    Request  req_;                   ///< 最新请求（单槽覆盖 = 风暴合并）
    Result   result_;                ///< 最新完成结果
    std::int64_t last_render_cost_us_ = 0; ///< 上轮渲染耗时（worker 写，
                                           ///< pump 自适应节流读；锁内）
    std::function<void()> frame_ready_cb_; ///< 出帧回调（锁内读写）

    std::thread worker_; ///< 工作线程（最后声明：构造完毕再启动）
};

} // namespace ms
