// ============================================================================
//  LiveHub —— editor 端实时源统一接收转发中心（架构定稿 2026-11-06）
//
//  架构位置（用户定案："源统一接收转发才是最彻底的解决方案"）：
//      editor 端：四类源 → Ingest 层统一收（本轮 NdiIngest）→ Hub 帧缓冲
//        ├─ ① 预览进程内直用：LiveMediaSource(Hub 模式) 经 Feed 取帧
//        ├─ ② NDI 再发布：sender 线程 Feed → NDIlib_send，源名
//        │      ms-live/<project>/<asset_id>（转发不自研协议，复用 NDI
//        │      多接收者——任意多 display 各自建 receiver 即可）
//        └─ ③ 集中诊断：收发帧计数（单测/联测钩子）
//      display 端：LiveMediaSource(NDI 模式) 纯接收（无 Hub 依赖）
//
//  质量五原则落点：
//    * Hub 零加工：不缩放、不转码——收到的 RGBA 快照原样上抛/发布；
//    * 代际损失：Full NDI 一代（收端解码 → 发端再编码，视觉无损）；
//    * 流畅度：发送端按 Feed 最新帧节拍直发，不插帧、不缓冲。
//
//  生命周期（最小闭环）：Hub = editor 进程单例；ensureAsset 惰性建
//  （LiveMediaSource::prepare 探针触发），dropAsset 显式删（当前仅在
//  进程退出统一回收——场景删 clip 后 Hub 继续发短时间是已知限制，
//  后续由场景 diff 联动收口）。
//
//  线程契约：ensureAsset/dropAsset 在调度器/编辑线程（mutex 保护）；
//  Feed::latest 多线程读；sender 线程每实例一个（Feed → NDIlib_send）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "live/LiveFrame.hpp"
#include "live/LiveIngest.hpp" // ILiveIngest（注册表接口——实现头零依赖）
#include "model/SceneModel.hpp"

namespace ms {

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
        /// 投递一帧（新帧 = 新对象，不可变语义）。写者约定：Ingest 链
        /// （NdiIngest 回调经 Hub 装配）与单测注入——外部代码不应直接
        /// 持 Feed 投帧（探针 probeFeed 返回值仅供消费 latest）。
        void publish(LiveFramePtr frame) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                latest_ = std::move(frame);
            }
            version_.fetch_add(1, std::memory_order_relaxed);
        }

    private:
        friend class LiveHub;
        mutable std::mutex mutex_;
        LiveFramePtr       latest_;
        std::atomic<std::uint64_t> version_{0};
    };

    // ------------------------------------------------------------------------
    // 单例与静态探针
    // ------------------------------------------------------------------------
    static LiveHub& instance();

    /// 进程角色开关（默认 false）：仅 editor 主程序置 true——启用后
    /// LiveMediaSource::prepare 惰性 ensure（Ingest+Feed+sender）并走
    /// 进程内 Hub 模式；display 程序不置位 → 恒 NDI 直收（纯接收角色，
    /// 不误建 Ingest/sender）
    void setEnabled(bool on) noexcept { enabled_ = on; }
    bool enabled() const noexcept { return enabled_; }

    /// 再发布前缀项目名（ms-live/<project>/<asset_id>）；project 改名
    /// 后新 ensure 的素材用新名，存量素材在下一次 ensureAsset 时重建
    void setProjectName(std::string name) noexcept { project_name_ = std::move(name); }
    const std::string& projectName() const noexcept { return project_name_; }

    /// LiveMediaSource::prepare 探针：该素材是否已有 Hub 进程内帧源
    /// （display 进程无 Hub 实例注册 → 恒空 → 走 NDI 直收模式）
    static std::shared_ptr<Feed> probeFeed(const std::string& asset_id);

    // ------------------------------------------------------------------------
    // 素材生命周期（编辑/调度线程）
    // ------------------------------------------------------------------------

    /// 确保素材的 Ingest + Feed + 再发布 sender 就位（幂等；已存在则
    /// 更新 project 前缀变化时的发布名）。ingest 类型按 Ingest 注册表
    /// （live/LiveIngest.hpp）分派——未实现类型拒绝并记日志。
    void ensureAsset(const Asset& asset, const std::string& project_name);
    /// 停止并移除素材的全部资源（Ingest/sender 线程 join）
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
    ~LiveHub(); ///< 统一回收条目（join 全部 Ingest/sender 线程——见 cpp）

    bool        enabled_      = false;      ///< 进程角色开关（editor 置 true）
    std::string project_name_ = "project";  ///< 再发布前缀（ms-live/<project>/...）

    struct Entry {
        std::shared_ptr<ILiveIngest> ingest;      ///< 收端（外部源 → Feed）
        std::shared_ptr<Feed>      feed;          ///< 预览出口（探针返回它）
        void*                      sender = nullptr; ///< NDIlib_send_instance_t
        std::thread                sender_thread; ///< Feed → NDIlib_send 转发
        std::string                pub_name;      ///< ms-live/<project>/<asset_id>
        std::atomic<bool>          sender_run{false};
        // ---- ingest 装配指纹（2026-11-10 源切换检测）----
        // 用户在编辑端换源（源名/延迟/接入方式任一变化）→ ensureAsset
        // 停旧建新；旧版只在 ingest 为空时创建 = 切换后永远转发旧源
        std::string  ingest_source;           ///< ingest 装配时的源标识
        int          ingest_latency = -1;     ///< -1 = 未装配
        std::int32_t ingest_kind    = -1;     ///< LiveIngest proto 值
    };

    void senderLoop(Entry& entry); // 转发线程主体（Entry 生命周期内运行）
    void stopEntry(Entry& entry);  // 停线程 + 销毁 sender（持锁外调用）

    mutable std::mutex                                  mutex_;
    std::unordered_map<std::string, std::unique_ptr<Entry>> entries_;
};

} // namespace ms
