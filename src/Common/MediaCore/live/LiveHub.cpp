// ============================================================================
//  LiveHub 实现 —— 注册表 + NDI sender 再发布（Feed → NDIlib_send）
// ============================================================================

#include "live/LiveHub.hpp"

#include <chrono>
#include <cstdio>

#include "Processing.NDI.Lib.h"
#include "Processing.NDI.Send.h"

namespace ms {

namespace {

/// NDI 运行时引用计数（第三次出现：与 LiveMediaSource/NdiIngest 同语义）。
/// 【堆上泄漏式单例】静态析构期 LiveHub 析构会经 stopEntry→ndiRelease
/// 触碰本计数——函数局部静态对象在逆序析构下可能先于 Hub 析构，
/// 泄漏分配保证进程全生命周期有效（OS 回收，字节量可忽略）。
std::atomic<int>& ndiRefs() {
    static std::atomic<int>* refs = new std::atomic<int>{0};
    return *refs;
}

bool ndiAcquire() {
    if (ndiRefs().fetch_add(1, std::memory_order_acq_rel) == 0) {
        if (!NDIlib_initialize()) {
            ndiRefs().fetch_sub(1, std::memory_order_acq_rel);
            return false;
        }
    }
    return true;
}

void ndiRelease() {
    // 【2026-11-10 同步修改】与 NdiIngest.cpp 同语义：只减计数、不再
    // NDIlib_destroy()——sender 线程收尾（stopEntry join）与 display 端
    // 收端析构/装配跨线程并发时，destroy 会与运行中 NDI 实例竞态。
    // 运行时进程级常驻，退出由 OS 统一回收。
    ndiRefs().fetch_sub(1, std::memory_order_acq_rel);
}

} // namespace

// ---------------------------------------------------------------------------
// 单例与静态探针
// ---------------------------------------------------------------------------
LiveHub& LiveHub::instance() {
    static LiveHub hub;
    return hub;
}

// ---------------------------------------------------------------------------
// 析构：统一回收全部条目（Ingest/sender 线程 join + sender 销毁）。
// 【修复 2026-11-10】此前无析构——用过 Live 素材的进程退出时，静态单例
// 隐式析构 entries_，携带 joinable sender_thread 的 Entry 随 map 析构 =
// std::thread 析构 joinable 线程 → std::terminate()（退出必崩）。
// 正常路径 editor main 已显式 clear()；本析构为兑底网（空表零成本）。
// ---------------------------------------------------------------------------
LiveHub::~LiveHub() { clear(); }

std::shared_ptr<LiveHub::Feed> LiveHub::probeFeed(const std::string& asset_id) {
    auto& hub = instance();
    std::lock_guard<std::mutex> lock(hub.mutex_);
    const auto it = hub.entries_.find(asset_id);
    return it == hub.entries_.end() ? nullptr : it->second->feed;
}

// ---------------------------------------------------------------------------
// ensureAsset：Ingest + Feed + 再发布 sender（幂等）
// ---------------------------------------------------------------------------
void LiveHub::ensureAsset(const Asset& asset, const std::string& project_name) {
    if (asset.kind != AssetKind::Live) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& entry = entries_[asset.id];
    if (entry == nullptr) entry = std::make_unique<Entry>();

    // ---- Feed：预览出口（探针句柄；每素材唯一）----
    if (entry->feed == nullptr) entry->feed = std::make_shared<Feed>();

    // ---- 再发布名：ms-live/<project>/<asset_id>（project 变化时更新）----
    const std::string pub = "ms-live/" +
        (project_name.empty() ? std::string("project") : project_name) +
        "/" + asset.id;

    // ---- Ingest：收外部源 → Feed（注册表分派；未实现类型仅占位）----
    // 【2026-11-10 源切换修复】ingest 装配指纹（源名/延迟/类型）变化 →
    // 停旧建新——旧版只在 ingest 为空时创建，用户在编辑端切换源后 Hub
    // 永远转发旧源画面（Feed 句柄保留：预览端不断流，仅收源短暂换轨）。
    // stop 在持锁下 join（≤500ms 一次性，切换为低频点击操作；Feed 自带
    // mutex_，预览取帧不经本锁）。
    if (entry->ingest != nullptr &&
        (entry->ingest_source != asset.live_source_name ||
         entry->ingest_latency != asset.live_latency ||
         entry->ingest_kind != asset.live_ingest)) {
        std::fprintf(stderr, "[live-hub] %s: 源变化（'%s' → '%s'），重建收端\n",
                     asset.id.c_str(), entry->ingest_source.c_str(),
                     asset.live_source_name.c_str());
        std::fflush(stderr);
        entry->ingest->stop();
        entry->ingest.reset();
        entry->ingest_source.clear();
        entry->ingest_latency = -1;
        entry->ingest_kind    = -1;
    }
    if (entry->ingest == nullptr) {
        const IngestDesc* desc = findIngest(asset.live_ingest);
        if (desc == nullptr || !desc->implemented ||
            asset.live_source_name.empty()) {
            std::fprintf(stderr,
                         "[live-hub] %s: ingest=%d 不受支持或源名为空，跳过 "
                         "Ingest（仅建发布器占位）\n",
                         asset.id.c_str(), asset.live_ingest);
            std::fflush(stderr);
        } else {
            std::weak_ptr<Feed> weak_feed = entry->feed; // 悬垂安全：Hub 先于
            // 回调销毁时（clear 持锁 stopEntry join 线程，回调不会越过 Feed
            // 析构——weak 捕获双保险）
            entry->ingest = desc->create(
                asset.live_source_name, asset.live_latency,
                [weak_feed](LiveFramePtr frame) {
                    if (auto feed = weak_feed.lock()) feed->publish(std::move(frame));
                });
            entry->ingest->start(); // 失败仅告警：重试语义与源离线占位一致
            // 记录装配指纹（下次 ensureAsset 变化检测）
            entry->ingest_source  = asset.live_source_name;
            entry->ingest_latency = asset.live_latency;
            entry->ingest_kind    = asset.live_ingest;
        }
    }

    // ---- 再发布 sender：发布名变化（project 改名）→ 重建 ----
    if (entry->pub_name != pub) {
        stopEntry(*entry);
        entry->pub_name = pub;
        if (ndiAcquire()) {
            NDIlib_send_create_t desc{};
            desc.p_ndi_name = pub.c_str();
            // clock 关闭：转发线程自带 version 轮询节流（有新帧才发），
            // SDK 侧再按时钟节流会与实际提交节奏（源帧率非恒定）冲突
            // ——曾以 clock_video=true + 60p 元数据跑 30fps 输入，收端
            // 异常断连重连（实测 2026-11-08 质量探针）
            desc.clock_video = false;
            desc.clock_audio = false;
            entry->sender = NDIlib_send_create(&desc);
            if (entry->sender == nullptr) {
                std::fprintf(stderr, "[live-hub] %s: 发布器创建失败 (%s)\n",
                             asset.id.c_str(), pub.c_str());
                std::fflush(stderr);
                ndiRelease();
            }
        }
        // 转发线程：Feed 有新帧 → NDIlib_send（发送方同步拷贝语义）
        if (entry->sender != nullptr) {
            entry->sender_run.store(true, std::memory_order_release);
            Entry* e = entry.get();
            try {
                e->sender_thread = std::thread([this, e] { senderLoop(*e); });
            } catch (...) {
                entry->sender_run.store(false, std::memory_order_release);
            }
            std::fprintf(stderr, "[live-hub] %s: 再发布 '%s'\n",
                         asset.id.c_str(), pub.c_str());
            std::fflush(stderr);
        }
    }
}

// ---------------------------------------------------------------------------
// senderLoop：Feed → NDIlib_send（版本号轮询：仅新帧发布——零重复编码）
// ---------------------------------------------------------------------------
void LiveHub::senderLoop(Entry& entry) {
    std::uint64_t last_version = entry.feed->version(); // 跳过存量帧
    // 上一帧缓冲保活（2026-11-12 加固）：NDI 发送端对帧缓冲的消费与调用
    // 返回存在异步窗口（SDK 契约：同步事件 = 下一次 send 调用，SDK 头
    // 文件 async 变体注释同款契约）。旧版每次循环取 latest() 后上一帧
    // LiveFrame 立即析构——若 SDK 消费越过本次调用返回，堆块被后续分配
    // 复用即读到混合内容。prev_sent 在本次 send 返回后才换绑：上一帧
    // 缓冲贯穿至"下一次 send 调用"这一同步事件，与契约严格对齐。
    //（2026-11-12 探针"撕裂帧"排查的最终定论：现象实为编码条 YUV 往返
    //  ±1 偏移导致配对错帧，非本处缓冲问题——但本保活按契约仍属正确
    //  姿势，保留。）
    LiveFramePtr prev_sent;
    while (entry.sender_run.load(std::memory_order_acquire) &&
           entry.sender != nullptr) {
        const std::uint64_t v = entry.feed->version();
        if (v == last_version) { // 无新帧：20ms 一拍（≈50Hz 探询，CPU 可忽略）
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }
        last_version = v;
        const LiveFramePtr frame = entry.feed->latest();
        if (!frame || frame->w <= 0 || frame->h <= 0) continue;

        // Hub 零加工：RGBA 快照原样发布（FourCC=RGBA；Full NDI 一代压缩）
        NDIlib_video_frame_v2_t out{};
        out.xres               = frame->w;
        out.yres               = frame->h;
        out.FourCC             = NDIlib_FourCC_video_type_RGBA;
        out.frame_rate_N       = 30000; // 帧率元数据（收端时钟参考；30p——
        out.frame_rate_D       = 1001;  //  与 Feed 转发的典型源帧率一致）
        out.frame_format_type  = NDIlib_frame_format_type_progressive;
        out.p_data             = const_cast<std::uint8_t*>(frame->rgba.data());
        out.line_stride_in_bytes = frame->w * 4;
        // 本次调用同时是上一帧（prev_sent）的消费同步点（见上方注释）
        NDIlib_send_send_video_v2(
            static_cast<NDIlib_send_instance_t>(entry.sender), &out);
        prev_sent = frame; // send 返回后才释放上一帧（换绑即释放）
    }
}

// ---------------------------------------------------------------------------
// stopEntry：停转发线程 + 销毁 sender（调用方持 mutex_）
// ---------------------------------------------------------------------------
void LiveHub::stopEntry(Entry& entry) {
    if (entry.sender_run.load(std::memory_order_acquire)) {
        entry.sender_run.store(false, std::memory_order_release);
        if (entry.sender_thread.joinable()) entry.sender_thread.join();
    }
    if (entry.sender != nullptr) {
        NDIlib_send_destroy(static_cast<NDIlib_send_instance_t>(entry.sender));
        entry.sender = nullptr;
        ndiRelease();
    }
}

// ---------------------------------------------------------------------------
// dropAsset / clear
// ---------------------------------------------------------------------------
void LiveHub::dropAsset(const std::string& asset_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = entries_.find(asset_id);
    if (it == entries_.end()) return;
    stopEntry(*it->second); // 停 sender（join 后回调链不再触发）
    if (it->second->ingest) it->second->ingest->stop();
    entries_.erase(it);
}

void LiveHub::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, entry] : entries_) {
        stopEntry(*entry);
        if (entry->ingest) entry->ingest->stop();
    }
    entries_.clear();
}

} // namespace ms
