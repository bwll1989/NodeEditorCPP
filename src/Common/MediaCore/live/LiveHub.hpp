// ============================================================================
//  LiveHub —— editor 端实时源统一接收转发中心
//  （2026-11-21 NDI 分发退役：转发通道 = MPEG-TS over UDP 组播）
//
//  架构位置（用户定案："源统一接收转发才是最彻底的解决方案"）：
//      editor 端：四类源 → Ingest 层统一收（NdiIngest 等）→ Hub 帧缓冲
//        ├─ ① 预览进程内直用：LiveMediaSource(Hub 模式) 经 Feed 取帧
//        ├─ ② 组播再发布：MpegTsSender Feed → H.264(NVENC/x264) →
//        │      MPEG-TS → UDP 组播（组播地址由 multicastUrlFor(asset_id)
//        │      确定性生成，display 端 NetworkIngest 收流——多 display
//        │      共享一路入网带宽）
//        └─ ③ 集中诊断：收发帧计数（单测/联测钩子）
//      display 端：LiveMediaSource(Network 模式) 纯接收（无 Hub 依赖）；
//      NDI 直收路径已随分发退役删除（NDI 源必须经编辑端组播转发）
//
//  质量五原则落点：
//    * Hub 零加工：不缩放、不转码——收到的 RGBA 快照原样上抛；
//      组播编码由 MpegTsSender 独立承担（帧接收戳锚定 PTS）；
//    * 流畅度：发送端按 Feed 最新帧节拍直发，不插帧、不缓冲。
//
//  生命周期（最小闭环）：Hub = editor 进程单例；ensureAsset 惰性建
//  （LiveMediaSource::prepare 探针触发），dropAsset 显式删（当前仅在
//  进程退出统一回收——场景删 clip 后 Hub 继续发短时间是已知限制，
//  后续由场景 diff 联动收口）。
//
//  线程契约：ensureAsset/dropAsset 在调度器/编辑线程（mutex 保护）；
//  Feed::latest 多线程读；组播线程归 MpegTsSender 自持。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "live/LiveFrame.hpp"
#include "live/LiveIngest.hpp" // ILiveIngest（注册表接口——实现头零依赖）
#include "model/SceneModel.hpp"

namespace ms {

class MpegTsSender; // 组播发送器（前向声明——Entry 持 unique_ptr，析构
                    // 在 cpp 实例化；本头不 include 它：MpegTsSender.hpp
                    // 反向 include 本头取 Feed 嵌套类型，防循环包含）

class LiveHub {
public:
    // ------------------------------------------------------------------------
    // Feed —— 单素材帧出口（预览消费端句柄；不可变快照轮询）
    // ------------------------------------------------------------------------
    class Feed {
    public:
        /// 最新帧（无帧返回空——调用方占位语义）
        LiveFramePtr latest() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return latest_;
        }
        /// 帧版本号（收到新帧 +1；sender 线程据此只在变化时发布）
        std::uint64_t version() const noexcept {
            return version_.load(std::memory_order_relaxed);
        }
        /// 帧同步排程取帧（2026-11-23 预览对齐）：pts <= target 的最大
        /// pts 帧（缓冲按产出序即 pts 升序，逆序首个即命中）；全未到期
        /// 返回空。未开历史（容量 0）恒空——调用方回退 latest()。
        LiveFramePtr scheduled(std::int64_t target_us) const {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto it = history_.rbegin(); it != history_.rend(); ++it)
                if ((*it)->pts_us <= target_us) return *it;
            return nullptr;
        }
        /// 排程历史容量（帧数；0 = 不留历史）。消费方 prepare 时按
        /// target_latency 换算设置（同 display 端 sync_capacity_ 口径）。
        void setScheduleCapacity(std::size_t frames) {
            std::lock_guard<std::mutex> lock(mutex_);
            history_cap_ = frames;
            while (history_.size() > history_cap_)
                history_.erase(history_.begin());
        }
        /// 投递一帧（新帧 = 新对象，不可变语义）。写者约定：Ingest 链
        /// （NdiIngest 回调经 Hub 装配）与单测注入——外部代码不应直接
        /// 持 Feed 投帧（探针 probeFeed 返回值仅供消费 latest）。
        void publish(LiveFramePtr frame) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                // 排程历史（pts 升序——Ingest 顺序产出 + 戳单调）：容量
                // 裁剪丢最老。pts_us==0（未打戳异常帧）不入史（排程没法
                // 定位它），仅作 latest 直显。
                if (history_cap_ > 0 && frame && frame->pts_us != 0) {
                    history_.push_back(frame);
                    while (history_.size() > history_cap_)
                        history_.erase(history_.begin());
                }
                latest_ = std::move(frame);
            }
            version_.fetch_add(1, std::memory_order_relaxed);
        }

    private:
        friend class LiveHub;
        mutable std::mutex mutex_;
        LiveFramePtr       latest_;
        /// 帧同步排程历史（2026-11-23）：预览端与 display 同延迟排程
        /// （"预览 = display 所见"对齐）；容量 setScheduleCapacity 控制
        std::vector<LiveFramePtr> history_;
        std::size_t               history_cap_ = 0;
        std::atomic<std::uint64_t> version_{0};
    };

    // ------------------------------------------------------------------------
    // 单例与静态探针
    // ------------------------------------------------------------------------
    static LiveHub& instance();

    /// 进程角色开关（默认 false）：仅 editor 主程序置 true——启用后
    /// LiveMediaSource::prepare 惰性 ensure（Ingest+Feed+组播 sender）
    /// 并走进程内 Hub 模式；display 程序不置位 → 走 Network 直收模式
    ///（收编辑端组播流，不误建 Ingest/sender）
    void setEnabled(bool on) noexcept { enabled_ = on; }
    bool enabled() const noexcept { return enabled_; }

    /// LiveMediaSource::prepare 探针：该素材是否已有 Hub 进程内帧源
    ///（display 进程无 Hub 实例注册 → 恒空 → 走 Network 直收模式）
    static std::shared_ptr<Feed> probeFeed(const std::string& asset_id);

    // ------------------------------------------------------------------------
    // 素材生命周期（编辑/调度线程）
    // ------------------------------------------------------------------------

    /// 确保素材的 Ingest + Feed + 组播 sender 就位（幂等）。ingest 类型
    /// 按 Ingest 注册表（live/LiveIngest.hpp）分派——未实现类型拒绝并
    /// 记日志。组播通道按 asset.live_url（空 = 未启用组播转发）。
    /// 【启用闸 2026-11-23】asset.live_enabled=false = 停止采集 + 停止
    /// 组播下发（收端/发送器拆净，Entry/Feed 保留占位）——采集与下发
    /// 的唯一运行开关，编辑端「启用/停止」按钮经场景 pb 同步到全链路。
    void ensureAsset(const Asset& asset);
    /// 停止并移除素材的全部资源（Ingest/组播线程 join）
    void dropAsset(const std::string& asset_id);
    /// 全部回收（进程退出 / 场景整体重载）
    void clear();

    // ---- 诊断 ----
    std::size_t assetCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

private:
    LiveHub() = default;
    ~LiveHub(); ///< 统一回收条目（join 全部 Ingest/组播线程——见 cpp）

    bool        enabled_      = false;      ///< 进程角色开关（editor 置 true）

    struct Entry {
        std::shared_ptr<ILiveIngest> ingest;      ///< 收端（外部源 → Feed）
        std::shared_ptr<Feed>      feed;          ///< 预览出口（探针返回它）
        /// 组播发送器（2026-11-21 组播转发；同日 NDI 分发退役——组播为
        /// 唯一分发通道）：live_url 非空时启动——Feed → H.264(NVENC/
        /// x264) → MPEG-TS → UDP 组播（多 display 共享一路入网带宽）
        std::unique_ptr<MpegTsSender> mcast;
        std::string                   mcast_url; ///< 装配指纹（URL 变化重建）
        // ---- ingest 装配指纹（2026-11-10 源切换检测）----
        // 用户在编辑端换源（源名/延迟/接入方式任一变化）→ ensureAsset
        // 停旧建新；旧版只在 ingest 为空时创建 = 切换后永远转发旧源
        std::string  ingest_source;           ///< ingest 装配时的源标识
        int          ingest_latency = -1;     ///< -1 = 未装配
        std::int32_t ingest_kind    = -1;     ///< LiveIngest proto 值
    };

    void stopEntry(Entry& entry); ///< 停组播线程（持锁外调用）

    mutable std::mutex                                  mutex_;
    std::unordered_map<std::string, std::unique_ptr<Entry>> entries_;
};

} // namespace ms
