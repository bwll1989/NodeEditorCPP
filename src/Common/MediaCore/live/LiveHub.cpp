// ============================================================================
//  LiveHub 实现 —— 注册表 + 组播转发（Feed → MpegTsSender）
//  （2026-11-21 NDI 分发退役：原 NDIlib_send 再发布整段删除，组播为
//   唯一分发通道；NDI 仅存于采集段 NdiIngest——editor 收外部源）
// ============================================================================

#include "live/LiveHub.hpp"

#include <cstdio>

#include "live/MpegTsSender.hpp" // 组播发送器完整定义（Entry 析构需要）

namespace ms {

// ---------------------------------------------------------------------------
// 单例与静态探针
// ---------------------------------------------------------------------------
LiveHub& LiveHub::instance() {
    static LiveHub hub;
    return hub;
}

// ---------------------------------------------------------------------------
// 析构：统一回收全部条目（Ingest/组播线程 join）。
// 【修复 2026-11-10】此前无析构——用过 Live 素材的进程退出时，静态单例
// 隐式析构 entries_，携带 joinable 线程的 Entry 随 map 析构 =
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
// ensureAsset：Ingest + Feed + 组播 sender（幂等）
// ---------------------------------------------------------------------------
void LiveHub::ensureAsset(const Asset& asset) {
    if (asset.kind != AssetKind::Live) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& entry = entries_[asset.id];
    if (entry == nullptr) entry = std::make_unique<Entry>();

    // ---- Feed：预览出口（探针句柄；每素材唯一）----
    if (entry->feed == nullptr) entry->feed = std::make_shared<Feed>();

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
        std::fprintf(stderr, "live-hub: %s: 源变化（'%s' → '%s'），重建收端\n",
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
                         "live-hub: %s: ingest=%d 不受支持或源名为空，跳过 "
                         "Ingest（仅建 Feed 占位）\n",
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

    // ---- 组播 sender（2026-11-21 组播转发；NDI 分发已退役——组播为
    // 唯一分发通道）：live_url 非空 = 启用。URL 变化 → 停旧建新。
    if (entry->mcast_url != asset.live_url) {
        if (entry->mcast != nullptr) {
            entry->mcast->stop();
            entry->mcast.reset();
            entry->mcast_url.clear();
        }
        if (!asset.live_url.empty()) {
            entry->mcast = std::make_unique<MpegTsSender>(
                asset.live_url, entry->feed);
            if (entry->mcast->start()) {
                entry->mcast_url = asset.live_url;
                std::fprintf(stderr, "live-hub: %s: 组播转发 '%s'（%s）\n",
                             asset.id.c_str(), asset.live_url.c_str(),
                             entry->mcast->encoderName().c_str());
            } else {
                entry->mcast.reset(); // 线程创建失败：下次 ensure 重试
                std::fprintf(stderr, "live-hub: %s: 组播转发启动失败 (%s)\n",
                             asset.id.c_str(), asset.live_url.c_str());
            }
            std::fflush(stderr);
        }
    }
}

// ---------------------------------------------------------------------------
// stopEntry：停组播线程（调用方持 mutex_）
// ---------------------------------------------------------------------------
void LiveHub::stopEntry(Entry& entry) {
    if (entry.mcast != nullptr) {
        entry.mcast->stop();
        entry.mcast.reset();
        entry.mcast_url.clear();
    }
}

// ---------------------------------------------------------------------------
// dropAsset / clear
// ---------------------------------------------------------------------------
void LiveHub::dropAsset(const std::string& asset_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = entries_.find(asset_id);
    if (it == entries_.end()) return;
    stopEntry(*it->second); // 停组播线程（join 后回调链不再触发）
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
