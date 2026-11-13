// ============================================================================
//  PreviewRenderer.cpp —— Editor 预览所见即所得渲染器实现（解码线程化）
// ============================================================================

#include "PreviewRenderer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

#include <QImage>

#include "media/FfmpegMediaSource.hpp" // Video/Image creator 装配（FFmpeg 解码）
#include "media/TextMediaSource.hpp"   // Text 注册（无条件，无文件依赖）
#include "media/LiveMediaSource.hpp"   // Live 注册（无条件：NDI 实时源）

namespace ms {

namespace {

/// 缺省时钟：steady 毫秒级对齐读法（与 ClockProvider.alignedNowUs 同为
/// 单调毫秒栅格；预览为本地参考画面，无需与 display 强制同源）
std::int64_t defaultClockUs() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

/// 工作线程空闲等待上限（ms）：condvar 正常由 notify 唤醒；超时兜底
/// 防止极端情况下的信号丢失（醒来发现无新请求 → 继续睡，零成本）
constexpr auto kWorkerIdleWait = std::chrono::milliseconds(100);

} // namespace

// ---------------------------------------------------------------------------
// 构造：本地素材源装配（照抄 RenderThread 运行期默认装配模式——
// assets_root 非空 → 注册 Video+Image creator；空 = 全占位降级）
// 工作线程在构造尾部启动（所有成员已就绪）。
// 【硬解装配（第三步 2026-11-05）】GpuDeliver 模式缺省请求 CUDA 硬解 +
// 0 拷贝协商（MS_PREVIEW_HW=none 关闭）。SoftCompose 保持软解——③阶段
// 基准实证硬解下载路径比纯软解慢 1.9~2.2x（GPU→CPU 读回占大头），软
// 合成模式下硬解无收益；仅 GL 直通（0 拷贝或协商失败自动回退下载）有
// 意义。探测失败自动回退软解（tryEnableHwAccel 内部），零风险。
// ---------------------------------------------------------------------------
PreviewRenderer::PreviewRenderer(std::string assets_root,
                                 std::function<std::int64_t()> clock_fn,
                                 DeliverMode mode)
    : scheduler_(factory_),
      mode_(mode),
      clock_fn_(clock_fn ? std::move(clock_fn) : defaultClockUs),
      assets_root_(std::move(assets_root)) {
    // ---- 硬解配置装配（GpuDeliver 缺省 cuda；MS_PREVIEW_HW=none 关）----
    if (mode_ == DeliverMode::GpuDeliver) {
        const char* env = std::getenv("MS_PREVIEW_HW");
        if (!env || std::string(env) != "none") {
            hw_cfg_.type = HwAccelType::Cuda;
            hw_cfg_.gpu_delivery = true; // 0 拷贝意图（发布分支的前提之一）
            // 协商旗标乐观置位：与解码源共享（creator 闭包）、与预览
            // GlRenderer 共享（gpuDeliveryFlag getter）——interop 探测
            // 失败由渲染端 disableGpuDelivery 翻 false，解码端下一帧
            // 自动回退 av_hwframe_transfer_data 下载（无黑屏自愈）。
            hw_cfg_.gpu_delivery_ok = std::make_shared<std::atomic_bool>(true);
            std::fprintf(stderr,
                         "[preview] 硬解装配：type=cuda gpu_delivery=on "
                         "（MS_PREVIEW_HW=none 可关）\n");
        }
    }
    // 【空根目录也注册（2026-11-12 视频测试实证修复）】FfmpegMediaSource
    // 对空 assets_root 有明确语义：full = asset.path 直通（绝对路径寻址，
    // 见 FfmpegMediaSource.cpp openInput 的空根分支）。此前"空根 = 不注册
    // 解码器"把这条合法路径挡死——编辑器端（MainWindow）以空根装配 +
    // 素材绝对路径入库，视频/图片片段全部占位降级、预览永远无画面。
    // creator 闭包值拷贝 root（可为空），路径语义由媒体源端裁决。
    {
        const std::string root = assets_root_;
        // creator 值拷贝根目录 + 硬解配置：生命周期与工厂一致，避免悬垂
        factory_.registerCreator(
            AssetKind::Video, [root, hw = hw_cfg_](const Asset&) {
                return std::make_unique<FfmpegMediaSource>(root,
                                                           AssetKind::Video,
                                                           hw);
            });
        factory_.registerCreator(
            AssetKind::Image, [root](const Asset&) {
                return std::make_unique<FfmpegMediaSource>(root, AssetKind::Image);
            });
    }
    // ---- 文本源注册【无条件】------------------------------------------------
    // 文本随场景 pb 分发（无文件、不依赖 assets_root）——预览永远可渲染
    // 文本片段（与 TextMediaSource 在 display 端 RenderThread 的注册同源）
    factory_.registerCreator(AssetKind::Text, [](const Asset&) {
        return std::make_unique<TextMediaSource>();
    });
    // ---- 实时源注册【无条件】------------------------------------------------
    // Live 与 Text 同款"无文件素材"：随场景 pb 分发，预览端 NDI 直收
    // （与 display 端 RenderThread 注册同源——两端同源渲染铁律）
    factory_.registerCreator(AssetKind::Live, [](const Asset&) {
        return std::make_unique<LiveMediaSource>();
    });
    // 【卡死终修】解码+合成全部移入工作线程——GUI 线程（paintEvent）只做
    // 请求提交与最新帧取用，时间轴拖动/片段拖拽/窗口缩放再无解码阻塞。
    worker_ = std::thread([this] { workerLoop(); });
}

// ---------------------------------------------------------------------------
// 析构：停工作线程（join 后回调不再触发——持有 this 的回调方安全销毁）
// ---------------------------------------------------------------------------
PreviewRenderer::~PreviewRenderer() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        quit_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

// ---------------------------------------------------------------------------
// 场景绑定 + 播放状态机（全部 GUI 线程；只动无锁状态 + 内容版本）
// ---------------------------------------------------------------------------
void PreviewRenderer::setScene(const Scene* s) {
    scene_ = s;
    // 场景切换：播放头归零回 Idle（与新场景时间轴对齐；素材缓存保留——
    // 同路径素材换场景仍可命中，删素材路径经 clearMediaCache 显式清理）
    playing_     = false;
    playhead_us_ = 0;
    ++rev_; // 内容版本递增 → 下一次 render 重新解算绘制列表
}

void PreviewRenderer::play() {
    if (playing_) return; // 幂等（对齐 EditorRuntime::cmdPlay）
    // Idle → 从头播；Paused → 从暂停点续播（base=冻结值，非 now 推算）
    playing_ = false; // 复位后统一走推进锚点重置
    t0_us_   = clock_fn_();
    playing_ = true;
}

void PreviewRenderer::pause() {
    if (!playing_) return;
    // 冻结：playhead 已由 tick 持续刷新，此处按锚点精确收尾（防最后一次
    // tick 与 pause 之间的间隙丢时间）
    playhead_us_ += clock_fn_() - t0_us_;
    playing_ = false;
}

void PreviewRenderer::stop() {
    playing_     = false;
    playhead_us_ = 0; // 对齐 cmdStop：归零回 Idle
}

void PreviewRenderer::seekTo(std::int64_t us) {
    // 【诊断 MS_CANVAS_DBG=1】seek 探针：echoSessionClock 每拍喂入的播放头
    // 值——暂停补拍/跳转是否到达本函数由此直接可读（配合 submit 探针区分
    // "echo 未喂" vs "pump 未提交" vs "worker 未解算"三段责任面）
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_n = 0;
            if (++s_n <= 300) {
                std::fprintf(stderr, "[preview-dbg] seekTo#%d us=%lld\n",
                             s_n, (long long)us);
                std::fflush(stderr);
            }
        }
    }
    playhead_us_ = us < 0 ? 0 : us;
    if (playing_) t0_us_ = clock_fn_(); // 播放态跳转：重置推进锚点不中断
}

bool PreviewRenderer::tick() {
    if (!playing_) return false;
    const std::int64_t now = clock_fn_();
    if (now <= t0_us_) return false; // 时钟未前进（沙箱节流/同毫秒）：跳过
    playhead_us_ += now - t0_us_;
    t0_us_ = now;
    return true;
}

// ---------------------------------------------------------------------------
// 降载接口：内容版本（invalidate/setTargetSize/clearMediaCache 只递增
// rev_，实际重解算延后到下一次 render——GUI 高频调用零成本）
// ---------------------------------------------------------------------------
void PreviewRenderer::invalidate() { ++rev_; }

void PreviewRenderer::setTargetSize(int max_w, int max_h) {
    // 尺寸未变：paintEvent 每帧都会调用，必须零成本短路（不递增版本，
    // 否则每次重绘都会重新解算并触发一次合成）
    if (max_w == target_w_ && max_h == target_h_) return;
    target_w_ = max_w;
    target_h_ = max_h;
    ++rev_; // 显示尺寸变化 → 下一帧按新尺寸重新合成（窗口拉大升清晰度）
}

void PreviewRenderer::clearMediaCache() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        clear_pending_ = true; // 清理动作转发到 worker（scheduler 单线程契约）
    }
    cv_.notify_all();
    ++rev_; // 内容版本递增 → 下一帧重建绘制列表与解码实例
}

void PreviewRenderer::setFrameReadyCallback(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_ready_cb_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// computeContentRect —— 内容域解算（GUI 线程；纯只读）
//
// 预览虚拟屏 = 画布 ∪ 全部 screens ∪ 全部 clips（有效尺寸）的轴对齐包围盒。
// 【无限画布语义】素材/screen 放到画布负坐标或超出 canvas 边界时同样进域——
// 预览所见范围由【内容】决定，而非固定的 (0,0,canvas_w,canvas_h) 矩形；
// 空场景退回画布矩形（保底显示域）。clips 的有效尺寸规则与
// ScreenMapper::buildDrawItemsForScreen 同源（transform.w<=0 → 素材原始尺寸）。
// ---------------------------------------------------------------------------
void PreviewRenderer::computeContentRect() {
    // 起点：虚拟画布矩形（canvas_w/h<=0 的异常场景由 pump 入口防御）
    double min_x = 0.0, min_y = 0.0;
    double max_x = scene_->canvas_w, max_y = scene_->canvas_h;
    auto expand = [&](double x, double y, double w, double h) {
        if (w <= 0 || h <= 0) return; // 退化矩形不扩展（未设置尺寸）
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x + w);
        max_y = std::max(max_y, y + h);
    };
    for (const auto& s : scene_->screens) expand(s.x, s.y, s.w, s.h);
    for (const auto& tr : scene_->timeline.tracks) {
        for (const auto& c : tr.clips) {
            const Asset* a = scene_->findAsset(c.asset_id);
            const double ew = c.transform.w > 0
                                  ? c.transform.w : (a ? a->width : 0.0);
            const double eh = c.transform.h > 0
                                  ? c.transform.h : (a ? a->height : 0.0);
            expand(c.transform.x, c.transform.y, ew, eh);
        }
    }
    content_rect_ = QRectF(min_x, min_y,
                           std::max(1.0, max_x - min_x),
                           std::max(1.0, max_y - min_y));
}

// ---------------------------------------------------------------------------
// computeSize —— 合成尺寸解算（GUI 线程；纯数学）
//
// 内容域钳制（8192 上限防异常大内容撑爆内存）+ setTargetSize 等比降采样
// （预览显示区域像素；窗口小 → 合成小，窗口拉大自动升清晰度）。
// 基准是【内容域】而非画布：素材拖出画布（负坐标/越界）后合成范围随之
// 扩展，预览帧始终覆盖全部可见内容。
// ---------------------------------------------------------------------------
void PreviewRenderer::computeSize(int& out_w, int& out_h) const {
    constexpr int kMaxPreviewDim = 8192;
    int w = std::clamp(static_cast<int>(std::lround(content_rect_.width())),
                       1, kMaxPreviewDim);
    int h = std::clamp(static_cast<int>(std::lround(content_rect_.height())),
                       1, kMaxPreviewDim);
    if (target_w_ > 0 && target_h_ > 0) {
        const double s = std::min(
            {1.0, (double)target_w_ / w, (double)target_h_ / h});
        if (s < 1.0) {
            w = std::max(1, (int)std::lround(w * s));
            h = std::max(1, (int)std::lround(h * s));
        }
    }
    out_w = w;
    out_h = h;
}

// ---------------------------------------------------------------------------
// pump —— 状态变化检测 + 快照提交（GUI 泵定时调用；解码线程化核心入口）
//
// 【第八迭代：引擎自转 + 窗口只拉帧】用户实测两处卡死：
//   ① 下发 patch 卡死（根因在素材 CRC，见 AssetHttpServer——另修）；
//   ② 添加媒体后播放卡死——预览"重绘驱动渲染"：每次 paint 提交请求，
//     worker 解码跟不上就连续满负荷（多路视频软解 CPU 打满，整机卡）。
// 本迭代把职责拆清：
//   * pump（本函数）：GUI 泵定时调用；输入变化才提交快照；自适应节流
//     （提交间隔 ≥ max(33ms, 上轮渲染耗时)）——解码慢时预览自动降帧，
//     worker 不再满负荷打满 CPU；
//   * pull：画布只拉最新完成帧上屏（paintEvent 零解码零提交）；
//   * worker：只渲染最新请求（单槽覆盖），解码/合成全部离场 GUI。
// ---------------------------------------------------------------------------
void PreviewRenderer::pump() {
    if (!scene_ || scene_->canvas_w <= 0 || scene_->canvas_h <= 0) return;

    // 【播放头自推进（2026-11-05 seek 跳帧修复）】播放态下按墙钟推进
    // playhead（tick 复活——此前从无调用点是死代码）：echo 回显定时器
    // 的到达率受 GUI 事件循环抖动影响（实测播放期可被拉到 250ms/拍），
    // 播放头完全依赖 echo 喂值时画面提交率被锁死在 echo 节奏（4fps 自
    // 锁：echo 慢 → 播放头大步 → produce 走大步/seek 分支 → cost 高 →
    // pump 节流同步拉长）。tick 按墙钟增量推进后，pump 每拍（33ms）播
    // 放头都前进真实增量 → input_changed 恒真 → 提交率回归泵节拍；
    // echo 的 seekTo 每拍仍会校准 playhead 为会话时钟值并重置锚点
    // （两者同源墙钟，校准间隙漂移≈0），Seek/暂停/跳转语义零变化。
    tick();

    // ---- 内容域 + 合成尺寸（降采样解算，纯数学）----
    // 内容域每次提交前重算（拖拽/属性编辑会 invalidate → rev_ 变化 →
    // 必然走到这里；域变化伴随 rev 变化，input_changed 天然涵盖）。
    computeContentRect();
    int w = 0, h = 0;
    computeSize(w, h);

    // ---- 输入未变化：零成本返回（静态画面零渲染零唤醒）----
    const bool input_changed = rev_ != posted_rev_ ||
                               playhead_us_ != posted_playhead_ ||
                               w != posted_w_ || h != posted_h_;
    if (!input_changed) {
        // 【Live 实时流 2026-11-11 用户定案】暂停态画布照常刷新：Live 是
        // "最新帧"实时源（Feed/ingest 持续出新帧，与播放状态无关），而
        // rev/播放头/尺寸都不反映帧更新——不放行则暂停态 pump 恒在此
        // 零提交，预览画布冻结在暂停瞬间的最后一帧（display 端恒每帧
        // 渲染故正常，两端表现分叉）。场景含 Live 素材时恒按泵节拍提交
        //（worker 取 Feed 最新帧重新合成——Live 帧常驻内存，合成轻量；
        // 节流闸门 33ms/cost 自适应控频）。无 Live 的场景保持"输入未变
        // 零渲染零唤醒"优化完全不变。
        bool has_live = false;
        for (const auto& a : scene_->assets)
            if (a.kind == AssetKind::Live) { has_live = true; break; }
        if (!has_live) return;
    }

    // ---- 自适应节流：worker 渲染慢时降低提交频率（CPU 占用自适应）----
    // 间隔 = max(33ms, 上轮渲染耗时)：渲染 50ms/帧 → 提交也 50ms 一拍，
    // worker 占用 ~50% 而非 100%，整机不再被多路软解拖死。
    // 【交互跟手修正】纯自适应节流在重素材下拖动预览仅 2~3fps（渲染
    // 300ms/帧 → 提交也 300ms 一拍），时间轴拖片段时预览滞后明显，且
    // 松手后还要再等一拍才出最终帧（体感"位置移了内容不动"）。现加
    // 100ms 硬上限：距上次提交 ≥100ms 且输入仍在变化 → 强制放行提交。
    // worker 单槽覆盖保证只渲最新请求（积压自动丢弃），CPU 峰值仅在
    // 拖动期间升高；停止拖动后 input_changed=false 零渲染，自适应
    // 降帧逻辑（防多路软解打满 CPU）在静止场景下完全不受影响。
    std::int64_t cost_us = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cost_us = last_render_cost_us_;
    }
    const auto now = std::chrono::steady_clock::now();
    if (last_submit_tp_.time_since_epoch().count() != 0) {
        const auto since_submit = now - last_submit_tp_;
        const auto min_interval =
            std::max<std::chrono::steady_clock::duration>(
                std::chrono::milliseconds(33),
                std::chrono::microseconds(cost_us));
        // 双闸门：常规节流窗（自适应降帧）+ 硬上限（100ms 交互跟手）。
        // 两者都未到才跳过——渲染再慢，交互中最多 100ms 必提交一拍。
        if (since_submit < min_interval &&
            since_submit < std::chrono::milliseconds(100))
            return; // 本拍跳过：下轮泵再试（状态最新值届时自然带上）
    }

    // 预览虚拟屏 = 内容包围盒（负坐标素材/出画布 screen 也进预览域——
    // 无限画布语义；全空时即画布矩形，与历史行为一致）
    preview_scr_.x = content_rect_.x();
    preview_scr_.y = content_rect_.y();
    preview_scr_.w = content_rect_.width();
    preview_scr_.h = content_rect_.height();

    // 绘制列表（可见性/有效尺寸/z 序与 display 端同源）——GUI 线程
    // 读取 Scene 工作副本的【唯一】跨对象路径，纯函数无副作用。
    const std::vector<DrawItem> items =
        buildPreviewDrawList(*scene_, preview_scr_, w, h, playhead_us_);

    // 快照：标量值拷贝 + Asset 值拷贝（不借用 Scene 内部指针——
    // worker 合成期间 GUI 可能并发拖拽/编辑工作副本）。
    Request r;
    r.w     = w;
    r.h     = h;
    r.epoch = ++next_epoch_;
    r.dom_x = content_rect_.x();
    r.dom_y = content_rect_.y();
    r.dom_w = content_rect_.width();
    r.dom_h = content_rect_.height();
    r.items.reserve(items.size());
    r.assets.reserve(items.size());
    r.clip_ids.reserve(items.size());
    for (const auto& di : items) {
        r.items.push_back(di);
        r.items.back().clip   = nullptr; // 快照只保留标量与 asset 拷贝
        r.items.back().screen = nullptr;
        // clip id 在指针置空前收集（di.clip 指向工作副本 Scene，此处
        // GUI 线程读取安全）——DeliverItem 带身份，GUI 贴图精确匹配用
        r.clip_ids.push_back(di.clip ? di.clip->id : std::string());
        if (di.asset) {
            r.assets.push_back(*di.asset); // 值拷贝自持
        } else {
            r.assets.emplace_back();       // 下标对齐占位（worker 跳过）
        }
    }
    // 指针重定向到快照自持的 Asset（vector 移动不换缓冲，仍有效；
    // 无素材项保持 nullptr，worker 跳过）
    for (std::size_t i = 0; i < r.items.size(); ++i) {
        r.items[i].asset = r.items[i].asset ? &r.assets[i] : nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        req_ = std::move(r); // 覆盖旧请求：worker 永远只渲最新
    }
    // 【诊断 MS_CANVAS_DBG=1】提交目标时刻探针（seek/暂停精度定位）：
    // 每次实际提交打印播放头与逐项解码目标——暂停/跳转后"画面停在的
    // 时刻"由此直接可读：media_us 与时间码一致 → 责任在解码/交付端；
    // media_us 偏旧 → 责任在提交端（echo/seekTo/pump 节流链）。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_scnt = 0;
            if (++s_scnt <= 200) {
                std::fprintf(stderr,
                             "[preview-dbg] submit#%d epoch=%llu "
                             "playhead=%lldus",
                             s_scnt, (unsigned long long)r.epoch,
                             (long long)playhead_us_);
                for (size_t i = 0; i < items.size() && i < 3; ++i)
                    std::fprintf(stderr, " m%zu=%lld", i,
                                 (long long)items[i].media_us);
                std::fprintf(stderr, "\n");
                std::fflush(stderr);
            }
        }
    }
    cv_.notify_all();

    // 【诊断 MS_CANVAS_DBG=1】失效驱动的提交探针（暂停态拖拽链路定位）：
    // rev 变化 = invalidate 触发（拖拽/属性编辑）。打印本次提交的项变换——
    // 若拖拽后此处仍是旧坐标 → refresh/invalidate 链路未触发；
    // 若是新坐标而画面不动 → 问题在 worker 交付或 pullGpu 消费链路。
    if (rev_ != posted_rev_) {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg) {
            static int s_pcnt = 0;
            if (++s_pcnt <= 60) {
                std::fprintf(stderr,
                             "[preview-dbg] pump#%d rev-submit epoch=%llu "
                             "items=%zu",
                             s_pcnt, (unsigned long long)r.epoch,
                             items.size());
                for (size_t i = 0; i < items.size() && i < 3; ++i)
                    std::fprintf(stderr,
                                 " i%zu=(%.1f,%.1f %.1fx%.1f r%.1f z%d a%d)",
                                 i, items[i].x, items[i].y, items[i].w,
                                 items[i].h, items[i].rotation, items[i].z,
                                 items[i].affine ? 1 : 0);
                std::fprintf(stderr, "\n");
                std::fflush(stderr);
            }
        }
    }

    posted_rev_      = rev_;
    posted_playhead_ = playhead_us_;
    posted_w_        = w;
    posted_h_        = h;
    last_submit_tp_  = now;
}

// ---------------------------------------------------------------------------
// pull —— 取最新完成帧（纯读；paintEvent/拉帧泵调用，零解码零提交）。
// domain 出参：帧对应的画布内容域（贴图映射基准；与帧同 epoch 原子更新）
// ---------------------------------------------------------------------------
bool PreviewRenderer::pull(QImage& out, QRectF* domain) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!result_.frame.isNull() && result_.epoch != delivered_epoch_) {
            delivered_frame_   = result_.frame;
            delivered_epoch_   = result_.epoch;
            delivered_domain_  = QRectF(result_.dom_x, result_.dom_y,
                                        result_.dom_w, result_.dom_h);
        }
    }
    out = delivered_frame_;
    if (domain) *domain = delivered_domain_;
    return !out.isNull();
}

// ---------------------------------------------------------------------------
// pullGpu —— GpuDeliver 取最新逐项交付（与 pull 同构：epoch 新帧才拷出；
// QImage 隐式共享使 items 拷贝零像素复制——GUI 只拿浅拷贝引用计数+1）
// ---------------------------------------------------------------------------
bool PreviewRenderer::pullGpu(std::vector<DeliverItem>& out_items,
                              int& out_w, int& out_h, QRectF& out_domain) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 【修复 2026-11-05】空交付也是合法交付：播放头越过片段区间（或
        // 空场景）时 worker 解算为空集并照常推进 epoch——旧守卫
        // `!result_.items.empty()` 把空交付拒之门外，delivered_items_ 从此
        // 永久残留最后一批陈旧项（旧位置/旧变换），此后任何 invalid 都
        // 无法刷新画面（症状：暂停拖拽内容不动、时间码走出区间后画面冻结）。
        // 改为仅 epoch 判定：空交付清空 delivered_items_ → pullGpu 返回
        // false → paintGL 自然走 QPainter 回退画出"无片段"场景（与软路径
        // pull 的 null 帧守卫语义对齐——空帧/空集都是"当前时刻无内容"）。
        if (result_.epoch != delivered_epoch_) {
            delivered_items_ = result_.items; // 浅拷贝（COW 共享像素；空集=清空）
            delivered_w_     = result_.comp_w;
            delivered_h_     = result_.comp_h;
            delivered_epoch_ = result_.epoch;
            delivered_domain_ = QRectF(result_.dom_x, result_.dom_y,
                                       result_.dom_w, result_.dom_h);
            // 【诊断 MS_CANVAS_DBG=1】新 epoch 交付探针（限频）：验证
            // worker 交付是否持续到达、item0 变换是否随拖拽更新
            static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
            if (s_dbg) {
                static int s_gcnt = 0;
                if (++s_gcnt <= 60) {
                    std::fprintf(stderr,
                                 "[preview-dbg] pull#%d epoch=%llu items=%zu",
                                 s_gcnt, (unsigned long long)delivered_epoch_,
                                 delivered_items_.size());
                    for (size_t i = 0; i < delivered_items_.size() && i < 3;
                         ++i)
                        std::fprintf(stderr,
                                     " i%zu=(%.1f,%.1f %.1fx%.1f r%.1f)",
                                     i, delivered_items_[i].item.x,
                                     delivered_items_[i].item.y,
                                     delivered_items_[i].item.w,
                                     delivered_items_[i].item.h,
                                     delivered_items_[i].item.rotation);
                    std::fprintf(stderr, "\n");
                    std::fflush(stderr);
                }
            }
        }
    }
    out_items = delivered_items_;
    out_w     = delivered_w_;
    out_h     = delivered_h_;
    out_domain = delivered_domain_;
    return !out_items.empty();
}

// ---------------------------------------------------------------------------
// workerLoop —— 工作线程主循环：等待 → 快照 → 解码合成 → 交付 → 回调
//
// 逐项取帧三步（与 display 端 Player 渲染循环同构）：
//   ensureSource（缓存命中复用/工厂创建+prepare）→ produce(media_us) 解码
//   → frameView 零拷贝视图 → SoftRenderer::draw（alpha 混合 + z 遮挡）。
// 失败降级：该片段区域保持底色（与 display 端"素材缺失不遮挡"一致）。
//
// 【GpuDeliver 分支】合成不出现在 worker：逐项解码 → 源帧 QImage 深拷贝
// → 交付 DrawItem+frame 列表；GL 合成在 GUI paintGL（GlRenderer
// ExternalCtx）完成。失败项直接缺席交付列表 = 该区域露底色（与 Soft
// 路径"留底色"语义一致——GL 里不画即透明）。
// ---------------------------------------------------------------------------
void PreviewRenderer::workerLoop() {
    std::uint64_t rendered_epoch = 0; // worker 已完成的最大请求代数
    for (;;) {
        // ---- 取最新请求快照（锁内拷贝，锁外重活）----
        Request job;
        bool     have_job  = false;
        bool     do_clear  = false;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 有新请求/待清理/退出即醒；定时醒一次兜底信号丢失
            cv_.wait_for(lock, kWorkerIdleWait, [this, rendered_epoch] {
                return quit_ || clear_pending_ || req_.epoch > rendered_epoch;
            });
            if (quit_) return;
            do_clear = clear_pending_;
            clear_pending_ = false;
            if (req_.epoch > rendered_epoch) {
                job      = req_; // 深拷贝快照：合成期间 GUI 可继续覆盖 req_
                have_job = true;
            }
        }

        if (do_clear) scheduler_.clear(); // 解码实例缓存清理（worker 独占）

        if (!have_job) continue; // 空转唤醒：无新请求，回去等

        // ---- 指针重定向到快照自持的 Asset（job 是深拷贝，原指针失效）----
        for (std::size_t i = 0; i < job.items.size(); ++i) {
            job.items[i].asset =
                job.items[i].asset ? &job.assets[i] : nullptr;
        }

        const auto render_t0 = std::chrono::steady_clock::now();

        if (mode_ == DeliverMode::GpuDeliver) {
            // ================================================================
            // GL 直通：逐项解码 + 源帧快照（合成移交 GUI paintGL）
            // ================================================================
            std::vector<DeliverItem> out;
            out.reserve(job.items.size());
            for (std::size_t i = 0; i < job.items.size(); ++i) {
                const DrawItem& di = job.items[i];
                if (!di.asset) continue; // 无素材元数据：区域露底色
                auto src = scheduler_.ensureSource(*di.asset);
                if (!src || !src->produce(di.media_us)) continue;
                FrameView fv;
                if (!src->frameView(fv)) continue;
                if (!fv.valid()) continue;
                DeliverItem it;
                it.item  = di;
                it.item.clip = nullptr;  // 快照只留标量（GL 合成不触指针）
                it.item.screen = nullptr;
                it.item.asset = nullptr;
                // clip 身份透传（下标对齐 job.items；produce 失败项不进
                // 交付列表，故身份必须在组装时随项携带而非按下标对应）
                it.clip_id = (i < job.clip_ids.size()) ? job.clip_ids[i]
                                                       : std::string();
                if (fv.domain == FrameDomain::CpuRgba) {
                    // CPU 域（软解 / 硬解下载回退）：源帧深拷贝（借 src
                    // 内部缓冲必悬垂——下次 produce 复用）；Format_RGBA8888
                    // 行距恒 width*4（32bit 行对齐零填充），GL 上传
                    // ROW_LENGTH 语义与 stride 契约一致
                    QImage img(fv.rgba, fv.width, fv.height,
                               static_cast<qsizetype>(fv.stride),
                               QImage::Format_RGBA8888);
                    if (img.isNull()) continue;
                    it.frame = std::move(img).copy(); // 深拷贝自持
                } else if (fv.domain == FrameDomain::GpuCudaNv12 &&
                           fv.native) {
                    // 【第三步 2026-11-05】GPU 域直通：零下载零拷贝——
                    // native 是 AVFrame 引用计数保活容器（引用 +1 自持，
                    // 释放线程安全：FFmpeg 引用计数原子），CUDA 设备指针
                    // 在 paintGL 的 interop D2D 直进纹理。仅透传尺寸标量
                    // （绘制几何需要；linesize 由 GlRenderer 内部从 AVFrame
                    // 取——帧内存布局只有解码端可见）
                    it.native = fv.native;
                    it.gpu_w  = fv.width;
                    it.gpu_h  = fv.height;
                } else {
                    continue; // 未知域防御（与 display 端语义一致）
                }
                out.push_back(std::move(it));
            }

            const auto render_cost_us = std::chrono::duration_cast<
                std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                           render_t0).count();
            rendered_epoch = job.epoch;

            // ---- 交付 + 触发出帧回调（回调在锁外调用——GUI 排队重绘）----
            std::function<void()> cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                result_.frame        = QImage();  // 双形态互斥：清软帧
                result_.items        = std::move(out);
                result_.comp_w       = job.w;
                result_.comp_h       = job.h;
                result_.epoch        = rendered_epoch;
                result_.dom_x        = job.dom_x; // 内容域透传
                result_.dom_y        = job.dom_y;
                result_.dom_w        = job.dom_w;
                result_.dom_h        = job.dom_h;
                last_render_cost_us_ = render_cost_us;
                cb                   = frame_ready_cb_;
                // 【诊断 MS_CANVAS_DBG=1】worker 轮耗时（自适应节流数据源）
                // ——cost 即 pump 提交间隔下限（max(33ms, cost)）：cost
                // 200ms 级 = 解码/合成慢 → 预览帧率 1000/cost 直接跌落。
                // 逐拍打印（前 400 拍全量，诊断期足够覆盖播放+seek 全程）
                {
                    static const bool s_wdbg =
                        std::getenv("MS_CANVAS_DBG") != nullptr;
                    if (s_wdbg) {
                        static int s_w = 0;
                        if (++s_w <= 400)
                            std::fprintf(stderr,
                                         "[preview-dbg] worker#%d cost=%lldus "
                                         "epoch=%llu items=%zu\n",
                                         s_w, (long long)render_cost_us,
                                         (unsigned long long)rendered_epoch,
                                         result_.items.size()),
                            std::fflush(stderr);
                    }
                }
            }
            if (cb) cb(); // 跨线程回调：调用方负责排队到 GUI 线程
            continue;
        }

        // ================================================================
        // SoftCompose：CPU 合成整帧（历史路径，行为零变化）
        // ================================================================
        // ---- 合成：清屏（不透明黑）→ 逐项取帧绘制（z 升序已由解算保证）----
        // 【自适应节流数据源】记录本轮耗时：GUI 提交泵按 max(33ms, 耗时)
        // 降频提交，多路软解时 worker 占用自适应回落（不再连续满负荷）。
        renderer_.beginFrame(job.w, job.h);
        for (const auto& di : job.items) {
            if (!di.asset) continue; // 无素材元数据：留底色
            auto src = scheduler_.ensureSource(*di.asset); // shared_ptr 保活
            if (!src || !src->produce(di.media_us)) continue;
            FrameView fv;
            if (!src->frameView(fv)) continue; // 视图无效：留底色
            renderer_.draw(di, fv);            // alpha 混合 + z 遮挡
        }
        const auto render_cost_us = std::chrono::duration_cast<
            std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                       render_t0).count();

        // ---- 深拷贝结果帧（renderer_ 缓冲每帧复用，借用必悬垂）----
        QImage frame;
        if (!renderer_.pixels().empty()) {
            frame = QImage(renderer_.pixels().data(), job.w, job.h,
                           static_cast<qsizetype>(job.w) * 4,
                           QImage::Format_RGBA8888)
                        .copy();
        }
        rendered_epoch = job.epoch;

        // ---- 交付 + 触发出帧回调（回调在锁外调用——GUI 排队重绘）----
        std::function<void()> cb;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            result_.frame        = std::move(frame);
            result_.items.clear();           // 双形态互斥：清逐项交付
            result_.epoch        = rendered_epoch;
            result_.dom_x        = job.dom_x; // 内容域透传（画布贴图映射用）
            result_.dom_y        = job.dom_y;
            result_.dom_w        = job.dom_w;
            result_.dom_h        = job.dom_h;
            last_render_cost_us_ = render_cost_us;
            cb                   = frame_ready_cb_;
        }
        if (cb) cb(); // 跨线程回调：调用方负责排队到 GUI 线程
    }
}

} // namespace ms
