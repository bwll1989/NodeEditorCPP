// ============================================================================
//  MediaScheduler.hpp —— 媒体源调度内核（§5.5）
//
//  设计依据：《media-server-sync-design.md》§5.5 / §4.7
//    * 持有「素材 → IMediaSource 实例」缓存表，每帧驱动 produce(local_us)；
//      具体实现（FfmpegMediaSource / 未来 Text/NDI/Spout）经 MediaFactory
//      注册表创建——调度层对媒体类型零感知（§4.7 插件化）；
//    * 实例生命周期：一个实例对应一个素材在某个 Player 上的播放上下文，
//      场景切换/素材删除时显式 releaseSource / clear。
//
//  故障隔离与重试语义（素材分发与播放并发的现实约束）：
//    * 工厂 create 返回 nullptr（类型未注册）→ 记日志返回空，不缓存坏状态；
//    * prepare 失败（文件尚未下载完/损坏）→ 丢弃实例【不留坏缓存】，下次
//      ensureSource 调用自动重试——素材稍后才就绪的场景无需上层干预；
//    * 单素材故障不扩散：任何失败只影响该素材，其余照常渲染。
//
//  线程契约（渲染架构改造 2026-11-03；shared_ptr 保活修订同日）：
//    * 解码驱动线程（DecodeDriver）调用 ensureSource/produce——表写（创建
//      实例）与解码都在驱动线程；渲染线程调用 find/frameView——只读。
//    * 源实例表（sources_）经 shared_mutex 并发：驱动线程写锁、渲染线程
//      读锁（每帧每项一次 map 查找 <1µs，无竞争热点）；produce/prepare
//      本身不持表锁（先查建完释放，再长解码——渲染线程读锁零等待）。
//    * 【shared_ptr 保活】表值与 find/ensureSource 返回 shared_ptr：渲染
//      线程/驱动线程持有的实例引用在换表（releaseSource/clear/path 变化
//      重建）期间保活到用毕——表操作即时生效（新场景即刻生效），旧实例
//      析构延迟到最后一个持有者释放，消除"清表瞬间渲染线程正用裸指针"
//      的 use-after-free 窗口（此前 unique_ptr 表 + 裸指针返回存在此隐患，
//      只因 clear 从未被调用而未爆雷）。
//    * 单线程使用（editor 预览 worker / 旧测试）完全兼容：锁无竞争退化
//      为普通互斥开销（纳秒级）。
//  MediaFactory::create 自带互斥（注册/创建与装配线程并发安全）。
// ============================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "engine/MediaFactory.hpp"

namespace ms {

class MediaScheduler {
public:
    /// \param factory 素材源工厂（默认全局实例；测试注入局部实例隔离）
    explicit MediaScheduler(MediaFactory& factory = MediaFactory::instance()) noexcept
        : factory_(factory) {}

    /// Text 素材内容指纹（FNV-1a 64；非 Text 恒 0）。
    /// 【同 id 内容变更失效（2026-11-05 文本片段完整化）】文本随场景 pb
    /// 分发、path 恒空——既有"path 变化重建"判定对文本永远不触发，用户
    /// 改文本后场景重发画面永不更新。指纹覆盖全部渲染输入字段
    /// （content/font/size/color/bold/italic/align/bg），内容变 → 失效。
    static std::uint64_t textFingerprint(const Asset& a) {
        if (a.kind != AssetKind::Text) return 0;
        std::uint64_t h = 1469598103934665603ull; // FNV-1a 64 offset basis
        const auto feed = [&h](const void* p, std::size_t n) {
            const auto* b = static_cast<const std::uint8_t*>(p);
            for (std::size_t i = 0; i < n; ++i) {
                h ^= b[i];
                h *= 1099511628211ull;
            }
        };
        feed(a.text_content.data(), a.text_content.size());
        feed(a.text_font.data(), a.text_font.size());
        feed(&a.text_font_size, sizeof(a.text_font_size));
        feed(&a.text_color, sizeof(a.text_color));
        feed(&a.text_bold, sizeof(a.text_bold));
        feed(&a.text_italic, sizeof(a.text_italic));
        feed(&a.text_h_align, sizeof(a.text_h_align));
        feed(&a.text_bg_color, sizeof(a.text_bg_color));
        return h;
    }

    /// Live 素材接入指纹（FNV-1a 64；非 Live 恒 0）。
    /// 【2026-11-08 Live Hub】实时源同文本：path 恒空随场景 pb 分发——
    /// 源名/URL/采集器/延迟模式任一变化（用户换了源）即失效重建。
    static std::uint64_t liveFingerprint(const Asset& a) {
        if (a.kind != AssetKind::Live) return 0;
        std::uint64_t h = 1469598103934665603ull; // FNV-1a 64 offset basis
        const auto feed = [&h](const void* p, std::size_t n) {
            const auto* b = static_cast<const std::uint8_t*>(p);
            for (std::size_t i = 0; i < n; ++i) {
                h ^= b[i];
                h *= 1099511628211ull;
            }
        };
        feed(a.live_source_name.data(), a.live_source_name.size());
        feed(a.live_url.data(), a.live_url.size());
        feed(&a.live_ingest, sizeof(a.live_ingest));
        feed(&a.live_latency, sizeof(a.live_latency));
        return h;
    }

    /// 无文件素材的统一内容指纹（Text/Live 分派；文件类素材恒 0——
    /// 其失效判定走既有 path 比较）。调用方用本函数替代原 textFingerprint。
    static std::uint64_t contentFingerprint(const Asset& a) {
        if (a.kind == AssetKind::Text) return textFingerprint(a);
        if (a.kind == AssetKind::Live) return liveFingerprint(a);
        return 0;
    }

    // 禁拷贝：缓存表持有源实例，语义上调度器是单线程独占资源
    MediaScheduler(const MediaScheduler&) = delete;
    MediaScheduler& operator=(const MediaScheduler&) = delete;

    /// 确保素材的源实例就绪（缓存命中直接返回；未命中→工厂创建+prepare）
    /// 【解码驱动线程调用】prepare（含文件打开/流探测）在本调用内同步
    /// 完成；渲染线程不经过此路径（只 find 已就绪实例 + frameView）
    /// 【同 id 换文件自动失效（改素材不同步修复）】缓存命中时校验 path：
    /// 用户在编辑端更换素材文件（id 不变 path 变）后场景重发，旧实例
    /// 命中旧文件导致画面永不更新——path 不一致即释放旧实例按新路径
    /// 重建（shared_ptr 保活下旧实例析构延迟到正在用它的线程用毕）。
    /// 【同 id 内容变更自动失效（2026-11-05 文本片段完整化）】Text 素材
    /// path 恒空、内容随场景 pb 分发——再叠加内容指纹校验：文案/字体/
    /// 字号/颜色/粗斜/对齐/背景任一变化即重建（无指纹时文本改内容
    /// 永不失效）。非 Text 指纹恒 0、比较零开销退化回纯 path 判定。
    /// \return 就绪的源实例（shared_ptr 保活，用毕即释放）；
    ///         nullptr = 类型未注册或 prepare 失败（可重试）
    std::shared_ptr<IMediaSource> ensureSource(const Asset& asset) {
        // 锁外析构收容（失效重建时摘出的旧实例——见下方写锁段注释）
        std::shared_ptr<IMediaSource> doomed;
        // 缓存命中：已 prepare 成功的实例直接复用（零开销路径；读锁）
        const std::uint64_t fp = contentFingerprint(asset);
        {
            std::shared_lock<std::shared_mutex> lock(table_mutex_);
            const auto it = sources_.find(asset.id);
            // 命中且 path/内容指纹均未变：直接复用；未命中或已变化
            // （同 id 换文件/文本改内容）→ 走下方写锁段（并发竞争下
            // 别人可能刚建好，写锁内再收敛）
            if (it != sources_.end() && it->second.path == asset.path &&
                it->second.fingerprint == fp)
                return it->second.src;
        }

        // ---- 写锁段：双重检查 + 旧实例释放（内容变化/并发创建收敛）----
        // 【2026-11-10 锁外析构】旧实例先移出表再在锁外析构——Live 收端
        // stop() 内 join 接收线程（最长 ~500ms：NDI 发现等待 500ms 一拍），
        // 旧版在写锁内 erase 析构 = 持锁 join，渲染线程 find 读锁同步阻塞
        // （切换实况源瞬间 display 整帧卡顿）。shared_ptr 保活语义不变：
        // 正在使用它的线程用毕才真正析构，只是析构时机挪到锁外。
        {
            std::unique_lock<std::shared_mutex> lock(table_mutex_);
            const auto it = sources_.find(asset.id);
            if (it != sources_.end()) {
                if (it->second.path == asset.path &&
                    it->second.fingerprint == fp)
                    return it->second.src; // 并发竞争：别人刚建好同内容实例
                // 失效重建（换文件/换源/文本改内容）：摘出旧实例（锁外析构）
                doomed = std::move(it->second.src);
                sources_.erase(it);
                std::fprintf(stderr,
                             "[sched] 素材变化，重建源实例: %s (path=%s)\n",
                             asset.id.c_str(), asset.path.c_str());
            }
        }
        // 锁外析构旧实例（若有）——join 不持锁
        doomed.reset();

        // 工厂创建：未注册的类型返回 nullptr（单素材故障隔离，不抛异常）
        auto src = factory_.create(asset);
        if (!src) {
            // 【日志去重（刷屏修复）】本调用每帧发生：此前"未注册"警告每帧
            // 打印，裸启动（assets_root 空 → 零源注册）时同一素材每秒刷
            // 上百行淹没全部有效日志。现按 asset_id 记忆只警告一次——
            // 配置问题（根目录没配）一次提示足够定位，反复刷屏没有增量信息。
            // 【并发】去重表与 clear()（场景切换线程）并发：insert 持表锁
            // （此前"渲染线程独占免锁"的注释已失实——本方法在驱动线程）
            bool first_warn = false;
            {
                std::unique_lock<std::shared_mutex> lock(table_mutex_);
                first_warn = unregistered_warned_.insert(asset.id).second;
            }
            if (first_warn) {
                std::fprintf(stderr,
                             "[sched] 素材类型未注册，跳过（本素材仅提示一次）: "
                             "%s (kind=%d)\n",
                             asset.id.c_str(), static_cast<int>(asset.kind));
            }
            return nullptr;
        }

        // prepare：失败→丢弃实例不留坏缓存（下次调用重试——
        // 素材可能稍后才通过 HTTP 分发到位）
        if (!src->prepare(asset)) {
            // 【日志节流】prepare 失败在"素材下载中"窗口内每帧重试每帧
            // 打印同样刷屏；按 asset_id 限频（同素材 5s 一条），保留
            // "仍未就绪"的可观测性又不淹没日志。（并发：同去重表，持锁）
            const auto now = std::chrono::steady_clock::now();
            bool should_warn = false;
            {
                std::unique_lock<std::shared_mutex> lock(table_mutex_);
                const auto it_last = prepare_warn_tp_.find(asset.id);
                if (it_last == prepare_warn_tp_.end() ||
                    now - it_last->second >= std::chrono::seconds(5)) {
                    prepare_warn_tp_[asset.id] = now;
                    should_warn = true;
                }
            }
            if (should_warn) {
                std::fprintf(stderr,
                             "[sched] 素材 prepare 失败，稍后重试（5s 限频）: "
                             "%s (%s)\n",
                             asset.id.c_str(), asset.path.c_str());
            }
            return nullptr;
        }

        // prepare 成功：清掉该素材的节流状态（下次失败重新计时节流；持锁——
        // 与 clear() 跨线程并发）
        {
            std::unique_lock<std::shared_mutex> lock(table_mutex_);
            prepare_warn_tp_.erase(asset.id);
        }

        auto shared = std::shared_ptr<IMediaSource>(std::move(src));
        {
            std::unique_lock<std::shared_mutex> lock(table_mutex_);
            sources_.insert_or_assign(
                asset.id, Entry{shared, asset.path, fp});
        }
        return shared;
    }

    /// 驱动取帧：ensure + produce(media_us)（每帧对每个活跃 clip 调用）
    /// 【解码驱动线程调用】解码全程不持表锁（查建完即释放）——渲染线程
    /// find 的读锁零等待
    /// \return false = 无帧（源未就绪/该时刻无载荷），上层跳过该 clip 绘制
    bool produce(const Asset& asset, std::int64_t media_us) {
        std::shared_ptr<IMediaSource> src = ensureSource(asset);
        return src != nullptr && src->produce(media_us);
    }

    /// 释放单个素材实例（场景切换/素材删除；写锁）
    /// 【shared_ptr 保活】erase 后正在用它的线程（驱动/渲染）用毕才析构
    /// \return 是否曾存在缓存
    bool releaseSource(const std::string& asset_id) {
        std::unique_lock<std::shared_mutex> lock(table_mutex_);
        return sources_.erase(asset_id) > 0;
    }

    /// 清空全部实例（新场景加载前；实例析构延迟到最后持有者释放）
    /// 警告去重状态一并清空：新场景里同一素材 id 重新出现时值得再提示
    void clear() noexcept {
        std::unique_lock<std::shared_mutex> lock(table_mutex_);
        sources_.clear();
        unregistered_warned_.clear();
        prepare_warn_tp_.clear();
    }

    // ---- 诊断 ----

    /// 当前缓存的源实例数
    std::size_t source_count() const noexcept {
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        return sources_.size();
    }

    /// 查找已缓存的源实例（不触发创建；渲染线程取帧入口，读锁）
    /// 【shared_ptr 保活】返回值持引用：换表（releaseSource/clear）期间
    /// 帧访问安全，实例用毕自动释放
    std::shared_ptr<IMediaSource> find(const std::string& asset_id) const noexcept {
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        const auto it = sources_.find(asset_id);
        return it == sources_.end() ? nullptr : it->second.src;
    }

private:
    /// 缓存条目：实例 + 装配时路径 + 内容指纹（同 id 内容变化失效检测）
    struct Entry {
        std::shared_ptr<IMediaSource> src; ///< 源实例（shared_ptr 保活）
        std::string path;                  ///< 装配时的素材路径
        /// 内容指纹（contentFingerprint 结果；文件类素材恒 0）——Text 的 path
        /// 恒空、失效检测全靠它（2026-11-05 文本片段完整化）
        std::uint64_t fingerprint = 0;
    };

    MediaFactory& factory_; ///< 素材源工厂（注入：全局实例或测试局部实例）
    /// 源实例表并发锁：解码驱动线程写（ensureSource 创建/releaseSource/
    /// clear）× 渲染线程读（find）。操作均为 map 查改（<1µs），无热点。
    mutable std::shared_mutex table_mutex_;
    std::unordered_map<std::string, Entry>
        sources_; ///< 素材 id → 源实例缓存（含装配路径）
    std::unordered_set<std::string>
        unregistered_warned_; ///< 已警告过"类型未注册"的素材 id（去重；
                              ///  驱动线程写 × clear 跨线程清 → 经
                              ///  table_mutex_ 互斥；clear 时一并清空——
                              ///  新场景下同一 id 值得再警告一次）
    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        prepare_warn_tp_; ///< 素材 id → 上次"prepare 失败"警告时刻（5s 限频）
};

} // namespace ms
