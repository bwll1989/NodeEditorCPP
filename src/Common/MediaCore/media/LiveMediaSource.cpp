// ============================================================================
//  LiveMediaSource 实现 —— 双模式消费端（Hub 进程内 / Network 直收）
//
//  收端逻辑唯一实现在 core/live/（NdiIngest=采集、NetworkIngest=组播收流）
//  ——本类只做模式分派与快照缓冲，NDI API 不在本 TU 出现（公共头与实现
//  均零 NDI 依赖）。
//
//  【2026-11-25 NDI 分发再退役（用户定案：NDI 单播带宽按接收端数放大，
//  跨机 WLAN 实测 3.8fps 幻灯片）】转发通道回归 MPEG-TS over UDP 组播
//  （2026-11-21 定案原样恢复：MpegTsSender + NetworkIngest + live_url）；
//  v14g 应显时刻语义保留（帧 pts = 捕获/解码 + 延迟预算，排程恒 target=now）。
// ============================================================================

#include "media/LiveMediaSource.hpp"

#include <cstdio>

#include "clock/ClockProvider.hpp"
#include "live/LiveHub.hpp"

namespace ms {

/// 预览排程的组播链路补偿（2026-11-23 预览对齐；2026-11-25 组播回归后
/// 重新生效）：display 显示的帧先经编码+组播+解码才上屏（时间轴整体
/// 滞后 δ），预览按 pts 应显时刻再延后 δ 排程才能与 display 同帧。
/// δ 实测随链路 30~100ms，取 60ms 常量吸收大头——残余差 = 链路抖动
/// （< 一帧），肉眼基本不可辨。
constexpr std::int64_t kHubPathLatencyUs = 60'000;

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
// sink 回调（Ingest 接收线程执行）：快照发布 + 排程缓冲管理
//
// 裁剪策略（帧同步排程启用时）：
//   * 时间窗：保留 pts 在 [newest_pts - keep_window, newest_pts] 内的帧
//     （keep_window = target_latency + 500ms 网络抖动余量）——排程窗外的
//     老帧渲染端永远不会选（v14g 选帧只取 pts <= now），留之无益；
//   * 帧数容量 sync_capacity_：prepare 期按 target_latency 动态计算
//     （clamp [16,48]，保证 ≥ target_latency 历史帧数——容量不足会把
//     未到期帧裁掉 → 选帧恒空 → 永久占位黑屏），同时防异常高频源
//     （如 120fps 捕获）内存膨胀（48 帧 ≈ 400MB/源 极限）。
// 监视语义（target_latency=0）不进缓冲：latest_ 直读零开销。
// ---------------------------------------------------------------------------
void LiveMediaSource::storeFrame(LiveFramePtr frame) {
    if (!frame || frame->w <= 0 || frame->h <= 0) return;
    const bool sync_on = target_latency_us_ > 0;
    {
        std::lock_guard<std::mutex> lock(snap_mutex_);
        latest_ = std::move(frame); // 旧快照由渲染端 keep 接管
        if (sync_on) {
            sync_buffer_.push_back(latest_); // 到达序 = pts 单调（单收线程）
            const std::int64_t keep_window =
                target_latency_us_ + 500'000; // 500ms 抖动余量
            const std::int64_t newest_pts = latest_->pts_us;
            while (!sync_buffer_.empty() &&
                   (newest_pts - sync_buffer_.front()->pts_us > keep_window ||
                    sync_buffer_.size() > sync_capacity_)) {
                sync_buffer_.pop_front();
            }
        }
    }
    frames_received_.fetch_add(1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// prepare：Hub 探针 → 直收收端装配
// ---------------------------------------------------------------------------
bool LiveMediaSource::prepare(const Asset& asset) {
    if (asset.kind != AssetKind::Live) return false; // 装配错位防御
    if (asset.id.empty()) {
        std::fprintf(stderr, "live: 素材 id 为空，无法装配\n");
        std::fflush(stderr);
        return false;
    }

    // ---- 启用闸（2026-11-23 用户定案「启用才捕获下发」）--------------------
    // 未启用 = 不进 Hub（不建 Ingest/组播发送器）也不直收：prepare 恒成功
    // （避免调度器按 prepare 失败 5s 限频重试刷屏），无收端 → produce 恒空
    // → 占位黑屏。启用时 live_enabled 变化参与调度器指纹 → 失效重建走
    // 正常装配。与 LiveHub::ensureAsset 的启用闸同源（asset.live_enabled）。
    if (!asset.live_enabled) return true;

    // ---- Hub 模式（editor 预览进程内直用；display 端未启用恒跳过）----
    // 惰性 ensure：首个消费者装配时建 Ingest+Feed+组播 sender（幂等），
    // Hub 生命周期独立于源实例（display 侧收流中不随预览 release 中断）
    if (LiveHub::instance().enabled()) {
        // 【2026-11-25 组播回归·预览对齐】Hub 进程内帧 pts = 捕获 + 预算
        //（v14g 捕获端打戳），display 收流帧 pts = 解码 + 预算——同一内
        // 容帧 display 恒滞后 δ = 编码+组播+解码链路延迟。预览按应显时
        // 刻再延后 kHubPathLatencyUs 排程（target = now - δ）才与 display
        // 同帧；残余差 = 链路抖动，肉眼基本不可辨。
        target_latency_us_ = asset.live_target_latency_us > 0
                                 ? asset.live_target_latency_us
                                 : 120'000; // 旧"监视"档存量兜底（同 display）
        LiveHub::instance().ensureAsset(asset);
        if (auto feed = LiveHub::probeFeed(asset.id)) {
            hub_feed_ = std::move(feed);
            // 排程历史容量（30fps 保守 + 8 帧余量，同 display 端口径；
            // 500ms 上限 ≈ 23 帧 < Feed 历史裁剪安全域）
            hub_feed_->setScheduleCapacity(
                static_cast<std::size_t>(target_latency_us_ / 33'333) + 8 +
                kHubPathLatencyUs / 33'333);
            connected_ = true; // 进程内源：无网络连接语义，恒"已连接"
            std::fprintf(stderr,
                         "live: %s: Hub 进程内模式（预览排程延迟 %.0fms"
                         "+组播链路补偿 %.0fms）\n",
                         asset.id.c_str(),
                         static_cast<double>(target_latency_us_) / 1e3,
                         static_cast<double>(kHubPathLatencyUs) / 1e3);
            std::fflush(stderr);
            return true;
        }
    }

    // ---- 直收模式（display 端 / Hub 未命中兜底）----
    // 【2026-11-23 全源组播（2026-11-25 回归）】组播转发全源类型通用：
    // live_url 非空 = display 纯收流（不碰本机设备——源设备在 display
    // 机器上根本不存在，Spout/摄像头按名直收是跨机黑屏根因），editor 端
    // Hub 统一编码再发布（MpegTsSender，H.264 over UDP 组播——多 display
    // 共享一路入网带宽）。
    // 【2026-11-21 NDI 分发退役】NDI 源唯一接收通道 = 编辑端组播转发
    // （live_url 空 → 拒绝装配并提示，不再退回 NDI 直收——display 端
    // NDI 接收路径已删除）。
    // 未启用组播的其余源（Spout/窗口/显示器/摄像头）保持按注册表本机
    // 直收（display 与 editor 同机场景仍然有效）。
    const IngestDesc* desc = nullptr;
    std::string connect_ref;
    if (!asset.live_url.empty()) {
        desc        = findIngest(static_cast<std::int32_t>(LiveIngest::Network));
        connect_ref = asset.live_url;
    } else if (asset.live_ingest == static_cast<std::int32_t>(LiveIngest::Ndi)) {
        std::fprintf(stderr,
                     "live: %s: NDI 源未启用组播转发，无法接收"
                     "（在编辑端 Live 片段属性勾选启用即自动生成组播地址）\n",
                     asset.id.c_str());
        std::fflush(stderr);
        return false;
    } else {
        desc = findIngest(asset.live_ingest);
        connect_ref = asset.live_source_name;
    }
    if (desc == nullptr || !desc->implemented) {
        std::fprintf(stderr,
                     "live: ingest=%d 暂未支持（等待 Live Hub Ingest "
                     "注册表实现）: %s\n", asset.live_ingest,
                     asset.live_source_name.c_str());
        std::fflush(stderr);
        return false;
    }
    if (connect_ref.empty()) {
        std::fprintf(stderr, "live: 连接串为空（源名/URL），无法连接\n");
        std::fflush(stderr);
        return false;
    }

    // 幂等：已在运行（重复 ensure）直接成功
    if (ingest_ != nullptr) return true;

    source_name_  = connect_ref;
    // 【2026-11-23 必须同步】延迟模式恒同步（低延迟丢帧档退役——用户定案）
    latency_mode_ = 1;
    // 帧同步延迟预算（2026-11-21 引入，2026-11-25 语义更新）：帧 pts =
    // 就绪/解码 + 该预算（应显时刻，Ingest 打戳时加——打戳落点按链路
    // 段分工：editor 捕获类源 = 帧就绪时刻；display 组播收流 = 解码出画
    // 时刻）——本类仅用它算排程缓冲容量（帧早到约一个预算时长，缓冲须
    // 容纳"未来帧"）。≤0（旧"监视"档存量值）兜底 120ms——同步恒启用。
    target_latency_us_ = asset.live_target_latency_us > 0
                             ? asset.live_target_latency_us
                             : 120'000;
    {
        // 容量必须覆盖 target_latency 的历史（30fps 保守估计 + 8 帧余量）：
        // 容量不足时帧被裁 → 选帧恒空 → 永久占位黑屏（审查修复）。
        const std::size_t need =
            static_cast<std::size_t>(target_latency_us_ / 33'333) + 8;
        sync_capacity_ = need < 16 ? 16 : (need > 48 ? 48 : need);
        std::fprintf(stderr,
                     "live: %s 帧同步排程启用: 延迟预算=%.1fms（随帧 pts，"
                     "应显时刻=就绪/解码+预算; buffer_capacity=%zu 帧）\n",
                     asset.id.c_str(),
                     static_cast<double>(target_latency_us_) / 1e3,
                     sync_capacity_);
        std::fflush(stderr);
    }

    // sink 弱捕获：release join 后回调不再触发；shared_ptr 直捕亦安全
    //（ingest_ 先于 latest_ 销毁，回调与析构无并发窗口）——直捕简洁
    ingest_ = desc->create(
        source_name_, latency_mode_, target_latency_us_,
        [this](LiveFramePtr frame) { storeFrame(std::move(frame)); });
    if (ingest_ == nullptr || !ingest_->start()) {
        ingest_.reset();
        std::fprintf(stderr, "live: %s 运行时初始化失败\n", desc->name);
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
    // Hub 模式：帧同步排程——display 的帧 pts = display 端解码时刻 + 预算
    //（组播链路 display 重打戳），预览帧 pts = 捕获 + 预算：同一内容帧
    // display 恒滞后 δ（编码+组播+解码），预览按 pts 应显时刻再延后 δ
    // 排程（target = now - δ）才能与 display 同帧（2026-11-23 预览对齐）。
    // 排程历史空/首帧未到期回退最新帧（预览宁可超前不黑屏——与 display
    // 的"等语义"刻意不同：编辑端操作反馈优先）。
    if (hub_feed_) {
        LiveFramePtr snap;
        if (target_latency_us_ > 0)
            snap = hub_feed_->scheduled(processAlignedNowUs() -
                                        kHubPathLatencyUs);
        if (!snap) snap = hub_feed_->latest();
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
    // 直收模式：排程选帧 / 监视直显（同一把锁内完成选帧 + last_shown_ 更新）
    LiveFramePtr snap;
    {
        std::lock_guard<std::mutex> lock(snap_mutex_);
        if (target_latency_us_ > 0 && !sync_buffer_.empty()) {
            // 帧同步排程（2026-11-25 语义定稿）：帧 pts = 应显时刻（打戳
            // 落点按链路段分工：editor 捕获类源 = 帧就绪 + 延迟预算；
            // display 组播收流 = 解码出画 + 延迟预算）。display 只按帧
            // 自带 pts 到点回放——取 pts <= now 的最大 pts 帧；早到帧等
            // （等语义：不跳不催）/ 晚到帧顺延显示。延迟预算从出生就定
            // 死在 pts 里，网络波动只影响帧到达早晚、不改显示节拍；多
            // display 同戳 → 跨机同步自动成立。时钟同域由 display 底层
            //（SNTP / ClockProvider）保证——片段层零补偿（v14f/v14g 片段
            // 层补偿已撤销：中位数被排队延迟污染致本机幻灯片，且越层）。
            const std::int64_t target = processAlignedNowUs();
            LiveFramePtr chosen;
            for (auto it = sync_buffer_.rbegin();
                 it != sync_buffer_.rend(); ++it) {
                if ((*it)->pts_us <= target) { // 缓冲 pts 单调：首个即最大
                    chosen = *it;
                    break;
                }
            }
            if (chosen) {
                last_shown_ = chosen;
                snap        = chosen;
            } else {
                snap = last_shown_; // 全未到期（帧早到）：等语义
            }
            // 排程健康取证（5s 限频）：显示相位差 pts-target（健康 = 刚
            // 到期 ∈ [0,33ms)）与水位（最新-显示 ≈ 延迟预算+网络延迟）。
            // 幻灯片指纹 = 相位差大幅负值（显示帧超前应显时刻）或水位
            // 远超预算；跨机部署把本行回传用于定位。
            if (snap && target - last_sched_log_us_ > 5'000'000) {
                last_sched_log_us_ = target;
                std::fprintf(stderr,
                             "live-sched: 显示相位 pts-target=%+.1f ms 水位"
                             "=%.1f ms 缓冲=%zu 帧\n",
                             static_cast<double>(snap->pts_us - target) / 1e3,
                             static_cast<double>(sync_buffer_.back()->pts_us -
                                                 snap->pts_us) / 1e3,
                             sync_buffer_.size());
                std::fflush(stderr);
            }
        } else {
            snap = latest_; // 监视语义（target=0）/ 缓冲空
        }
    }
    if (!snap || snap->w <= 0 || snap->h <= 0 || snap->rgba.empty())
        return false;
    out.width  = snap->w;
    out.height = snap->h;
    out.stride = static_cast<std::uint32_t>(snap->w) * 4;
    out.rgba   = snap->rgba.data();
    out.keep   = snap; // shared_ptr 保活快照（含像素向量）
    out.domain = FrameDomain::CpuRgba;
    out.native = nullptr;
    // 【刻意不填 out.pts_us】FrameView.pts_us = 素材内部 pts（RenderLoop
    // "未来帧拒绝"用，恒 0 = Live 天然豁免）；LiveFrame.pts_us = 应显时
    // 刻戳（就绪/解码 + 延迟预算，进程对齐时钟域，排程专用）。两者命名
    // 同、域不同——若把应显时刻戳填进 out.pts_us（Unix 微秒级巨大值），
    // 未来帧拒绝恒触发 = 全 Live 黑屏。
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
        sync_buffer_.clear();
        last_shown_.reset();
    }
    // 排程健康日志限频复位（ingest 已 stop/join，无并发写）
    last_sched_log_us_ = 0;
    connected_.store(false, std::memory_order_release);
}

} // namespace ms
