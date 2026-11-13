// ============================================================================
//  LiveMediaSource 实现 —— 双模式消费端（Hub 进程内 / NDI 直收）
//
//  收端逻辑唯一实现在 NdiIngest（core/live/）——本类只做模式分派与
//  快照缓冲，NDI API 不在本 TU 出现（公共头与实现均零 NDI 依赖）。
// ============================================================================

#include "media/LiveMediaSource.hpp"

#include <cstdio>

#include "live/LiveHub.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// 析构：先停收端再进成员析构（display 直收模式切换崩溃根因修复）
//
// 【竞态推演（2026-11-10）】本类无显式析构时成员按声明逆序析构：
// latest_/snap_mutex_（78/77 行）先析构，ingest_（74 行）最后析构——
// 其 stop() 内 join 接收线程，等待期间 recvLoop 仍可再跑一轮（Sleep
// 16~500ms 醒来）→ 拉到帧 → emitFrame → sink 回调 storeFrame →
// lock 已析构的 snap_mutex_ + 写已析构的 latest_ = use-after-free 崩溃。
//
// 为何只有 display 崩、预览端没事：Hub 模式 ingest_ 恒空（帧来自
// LiveHub::Feed 进程内缓冲），析构无并发回调；display 端 NDI/窗口/
// 显示器/Spout/摄像头直收全部内嵌 ingest —— 换源/换窗口必触发本路径。
//
// 修复：显式先停收端（join 返回后回调绝迹），后续成员析构无并发。
// release() 幂等，重复调用安全。
// ---------------------------------------------------------------------------
LiveMediaSource::~LiveMediaSource() {
    release();
}

// ---------------------------------------------------------------------------
// sink 回调（Ingest 接收线程执行）：快照发布
// ---------------------------------------------------------------------------
void LiveMediaSource::storeFrame(LiveFramePtr frame) {
    if (!frame || frame->w <= 0 || frame->h <= 0) return;
    {
        std::lock_guard<std::mutex> lock(snap_mutex_);
        latest_ = std::move(frame); // 旧快照由渲染端 keep 接管
    }
    frames_received_.fetch_add(1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// prepare：Hub 探针 → NDI 收端装配
// ---------------------------------------------------------------------------
bool LiveMediaSource::prepare(const Asset& asset) {
    if (asset.kind != AssetKind::Live) return false; // 装配错位防御
    if (asset.id.empty()) {
        std::fprintf(stderr, "[live] 素材 id 为空，无法装配\n");
        std::fflush(stderr);
        return false;
    }

    // ---- Hub 模式（editor 预览进程内直用；display 端未启用恒跳过）----
    // 惰性 ensure：首个消费者装配时建 Ingest+Feed+再发布 sender（幂等），
    // Hub 生命周期独立于源实例（display 侧收流中不随预览 release 中断）
    if (LiveHub::instance().enabled()) {
        LiveHub::instance().ensureAsset(asset, LiveHub::instance().projectName());
        if (auto feed = LiveHub::probeFeed(asset.id)) {
            hub_feed_  = std::move(feed);
            connected_ = true; // 进程内源：无网络连接语义，恒"已连接"
            std::fprintf(stderr, "[live] %s: Hub 进程内模式（预览直用）\n",
                         asset.id.c_str());
            std::fflush(stderr);
            return true;
        }
    }

    // ---- 直收模式（display 端 / Hub 未命中兜底）----
    // 类型按 Ingest 注册表分派：NDI/Spout/窗口/显示器捕获已实现协议均可
    // 直收（同机路径）；未实现/未知 ingest 拒绝（editor 端 Live Hub 会把
    // 未实现类型归一为 NDI 再发布后到达本源）
    const IngestDesc* desc = findIngest(asset.live_ingest);
    if (desc == nullptr || !desc->implemented) {
        std::fprintf(stderr,
                     "[live] ingest=%d 暂未支持（等待 Live Hub Ingest "
                     "注册表实现）: %s\n", asset.live_ingest,
                     asset.live_source_name.c_str());
        std::fflush(stderr);
        return false;
    }
    if (asset.live_source_name.empty()) {
        std::fprintf(stderr, "[live] live_source_name 为空，无法连接\n");
        std::fflush(stderr);
        return false;
    }

    // 幂等：已在运行（重复 ensure）直接成功
    if (ingest_ != nullptr) return true;

    source_name_  = asset.live_source_name;
    latency_mode_ = asset.live_latency == 1 ? 1 : 0; // 非 0/1 一律 0（保守低延迟）

    // sink 弱捕获：release join 后回调不再触发；shared_ptr 直捕亦安全
    //（ingest_ 先于 latest_ 销毁，回调与析构无并发窗口）——直捕简洁
    ingest_ = desc->create(
        source_name_, latency_mode_,
        [this](LiveFramePtr frame) { storeFrame(std::move(frame)); });
    if (ingest_ == nullptr || !ingest_->start()) {
        ingest_.reset();
        std::fprintf(stderr, "[live] %s 运行时初始化失败\n", desc->name);
        std::fflush(stderr);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// produce：有帧即真（双模式统一；media_us 占位）
// ---------------------------------------------------------------------------
bool LiveMediaSource::produce(std::int64_t media_us) {
    (void)media_us;
    if (hub_feed_) {
        const LiveFramePtr f = hub_feed_->latest();
        return f != nullptr && f->w > 0 && f->h > 0;
    }
    if (ingest_ != nullptr) connected_.store(ingest_->connected(),
                                             std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(snap_mutex_);
    return latest_ != nullptr && latest_->w > 0 && latest_->h > 0;
}

// ---------------------------------------------------------------------------
// frameView：快照视图（keep 保活——接收端发布新帧不影响本帧缓冲）
// ---------------------------------------------------------------------------
bool LiveMediaSource::frameView(FrameView& out) const {
    // Hub 模式：直读进程内缓冲（零拷贝优势——帧向量即 Hub 收到的原帧）
    if (hub_feed_) {
        const LiveFramePtr snap = hub_feed_->latest();
        if (!snap || snap->w <= 0 || snap->h <= 0 || snap->rgba.empty())
            return false;
        out.width  = snap->w;
        out.height = snap->h;
        out.stride = static_cast<std::uint32_t>(snap->w) * 4;
        out.rgba   = snap->rgba.data();
        out.keep   = snap;
        out.domain = FrameDomain::CpuRgba;
        out.native = nullptr;
        return true;
    }
    // NDI 模式：本地快照
    LiveFramePtr snap;
    {
        std::lock_guard<std::mutex> lock(snap_mutex_);
        snap = latest_;
    }
    if (!snap || snap->w <= 0 || snap->h <= 0 || snap->rgba.empty())
        return false;
    out.width  = snap->w;
    out.height = snap->h;
    out.stride = static_cast<std::uint32_t>(snap->w) * 4;
    out.rgba   = snap->rgba.data();
    out.keep   = snap; // shared_ptr 保活快照（含像素 vector）
    out.domain = FrameDomain::CpuRgba;
    out.native = nullptr;
    return true;
}

// ---------------------------------------------------------------------------
// release：停收端 → 清快照（调度器线程调用）
// ---------------------------------------------------------------------------
void LiveMediaSource::release() {
    hub_feed_.reset(); // Hub 模式：仅解除引用（Hub 自身生命周期独立）
    if (ingest_ != nullptr) {
        ingest_->stop(); // join 后 storeFrame 不再被调用
        ingest_.reset();
    }
    {
        std::lock_guard<std::mutex> lock(snap_mutex_);
        latest_.reset();
    }
    connected_.store(false, std::memory_order_release);
}

} // namespace ms
