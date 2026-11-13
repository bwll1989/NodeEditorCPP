// ============================================================================
//  LiveMediaSource —— 实时源媒体（IMediaSource 实现；Live Hub 消费端）
//
//  双模式（架构定稿 2026-11-06"源统一接收转发"）：
//    * Hub 模式（editor 预览进程内）：prepare 探针 LiveHub::probeFeed
//      命中 → 帧直接取自 Hub 进程内缓冲（零中间拷贝、零再发布代损——
//      架构③路径"预览进程内直用"）；
//    * NDI 模式（display 端 / Hub 未就绪的兜底）：内嵌收端核心
//      （display 端 LiveMediaSource 是唯一 Live 实现——"display 端
//      除固定媒体外全部抽象成统一 LiveHubMedia"的用户定案落点），按
//      asset.live_source_name 直连源（收 Hub 再发布流 ms-live/...
//      或测试直收外部源）。直收类型按 Ingest 注册表分派——NDI/Spout/
//      窗口/显示器捕获已实现协议均可直收（同机低延迟路径）。
//
//  定位：
//    * produce(media_us) 恒给最新帧（实时源无时间轴语义，media_us 占位）
//      ——【首次 produce(t>0) 直达】天然满足：无 seek 分支、无 GOP 依赖；
//    * 质量五原则：NDI 模式带宽恒 highest（不降分辨率）、RGBX_RGBA 直出
//      （零颜色空间转换）、latency=0 drain（宁丢勿延迟）/ latency=1 直通
//      （SDK 队列缓冲 2~3 帧）、不插帧。
//
//  线程契约：prepare/release 在调度器线程；NDI 模式接收线程在 NdiIngest
//  内部（sink 回调写 latest_）；frameView 在渲染线程——latest_/Feed 帧均为
//  shared_ptr 快照，渲染端 keep 保活本帧缓冲，接收端发布新帧不悬垂旧帧
//  （与解码线程化后的 FrameView.keep 同一模式）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "engine/MediaFactory.hpp"
#include "live/LiveFrame.hpp"
#include "live/LiveIngest.hpp" // ILiveIngest + 注册表（直收装配查表）
#include "live/LiveHub.hpp" // Feed 嵌套类型（Hub 模式句柄）需要完整定义

namespace ms {

class LiveMediaSource : public IMediaSource {
public:
    AssetKind kind() const noexcept override { return AssetKind::Live; }

    /// 析构先停收端（见实现注释——成员逆序析构前必须 join 接收线程，
    /// 否则 storeFrame 写已析构的快照成员 = display 切换崩溃根因）
    ~LiveMediaSource() override;

    /// 双模式装配：Hub 探针命中 → 进程内帧源；否则校验参数 + NDI 收端
    /// 启动（连接异步；源未上线不算失败——produce 返回 false → 占位）。
    /// 只有参数非法 / NDI 运行时初始化失败才返回 false（调度器 5s 限频
    /// 重试语义与视频源一致）。
    bool prepare(const Asset& asset) override;

    /// 恒给最新帧（media_us 占位：实时源与时间轴解耦）
    bool produce(std::int64_t media_us) override;

    /// 帧视图（CpuRgba 域；keep = 快照 shared_ptr——接收端随时发布
    /// 新帧，渲染线程 draw 期间旧快照由 keep 保活，无悬垂窗口）
    bool frameView(FrameView& out) const override;

    /// 停收端 → 清快照（可随后重新 prepare 复用）
    void release() override;

    // ---- 诊断 ----
    bool          connected()      const noexcept { return connected_; }
    std::uint64_t framesReceived() const noexcept { return frames_received_.load(std::memory_order_relaxed); }
    std::uint64_t framesDropped()  const noexcept { return frames_dropped_.load(std::memory_order_relaxed); }

private:
    // ---- Hub 模式（editor 预览进程内直用）----
    std::shared_ptr<LiveHub::Feed> hub_feed_; ///< 非空 = Hub 模式（prepare 固定）

    // ---- 直收模式（内嵌收端核心；类型由 Ingest 注册表分派）----
    std::unique_ptr<ILiveIngest> ingest_; ///< 收端（sink 回调写 latest_）

    // ---- 帧快照（NDI 模式：sink 在接收线程发布 / 渲染线程读取）----
    mutable std::mutex              snap_mutex_;
    LiveFramePtr                    latest_;
    void storeFrame(LiveFramePtr frame); ///< sink 回调（接收线程）

    // ---- 配置（prepare 期固定）----
    std::string source_name_;  ///< NDI 源名（诊断/重连日志用）
    int         latency_mode_ = 0;

    // ---- 诊断 ----
    std::atomic<bool>          connected_{false};
    std::atomic<std::uint64_t> frames_received_{0};
    std::atomic<std::uint64_t> frames_dropped_{0};
};

} // namespace ms
