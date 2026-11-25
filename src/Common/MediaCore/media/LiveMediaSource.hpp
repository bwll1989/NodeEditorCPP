// ============================================================================
//  LiveMediaSource —— 实时源媒体（IMediaSource 实现；Live Hub 消费端）
//
//  双模式（架构定稿 2026-11-06"源统一接收转发"；NDI 分发 2026-11-24
//  短暂复活后于 2026-11-25 再退役——带宽按接收端数放大，跨机 WLAN 卡死；
//  转发通道回归 MPEG-TS over UDP 组播）：
//    * Hub 模式（editor 预览进程内）：prepare 探针 LiveHub::probeFeed
//      命中 → 帧直接取自 Hub 进程内缓冲（零中间拷贝——
//      架构③路径"预览进程内直用"）；
//    * 直收模式（display 端 / Hub 未就绪的兜底）：内嵌收端核心
//      （display 端 LiveMediaSource 是唯一 Live 实现——"display 端
//      除固定媒体外全部抽象成统一 LiveHubMedia"的用户定案落点）。
//      直收类型按 Ingest 注册表分派：NDI 源唯一通道 = 编辑端组播转发
//      （live_url → Network 收流）；同机源（Spout/窗口/显示器/摄像头）
//      按注册表直收（同机低延迟路径）。
//
//  定位：
//    * produce(media_us) 恒给最新帧（实时源无时间轴语义，media_us 占位）
//      ——【首次 produce(t>0) 直达】天然满足：无 seek 分支、无 GOP 依赖；
//    * 质量五原则：带宽恒 highest（不降分辨率）、RGBA 直出（转换仅
//      RGBA→YUV→RGBA 编解码一代）、latency=0 drain（宁丢勿延迟）/
//      latency=1 直通（SDK 队列缓冲 2~3 帧）、不插帧。
//
//  帧同步排程（2026-11-25 语义定稿：应显时刻随帧走，仅 display 直收模式生效）：
//    * 每帧带应显时刻 pts_us（= 帧就绪时刻 + 延迟预算，如 120ms；进程
//      对齐时钟域，clock/ClockProvider.hpp processAlignedNowUs）：
//      - editor 捕获类源（WGC/Spout/摄像头）在捕获端打"捕获+预算"（v14g
//        用户定案"editor 端捕获时就把 120ms 加上"）；
//      - display 组播收流（NetworkIngest）在解码出画点打"解码+预算"——
//        组播同流同达，多 display 解码时刻差亚毫秒 → 跨机同步成立；
//      - editor 预览（Hub 模式）用捕获戳 + 组播链路补偿 kHubPathLatencyUs
//        （60ms，见 .cpp）与 display 同帧；
//    * live_target_latency_us > 0 时按 "now ≥ pts" 到点回放：取 pts <=
//      now 的最大 pts 帧（早到等 / 晚到顺延）——显示节拍钉在打戳节拍上，
//      网络波动只影响到达早晚、不改节拍；排程独立于播放头（时间轴暂停
//      画面继续流动）。时钟同域由 display 底层（SNTP/ClockProvider）
//      保证，片段层零补偿；
//    * =0 时退化 latest 直显（监视语义，与旧行为完全一致，零开销）。
//    * 缓冲按时间窗 + 帧数硬顶裁剪（storeFrame），内存有界。
//
//  线程契约：prepare/release 在调度器线程；直收模式接收线程在 Ingest
//  内部（sink 回调写 latest_）；frameView 在渲染线程——latest_/Feed 帧均为
//  shared_ptr 快照，渲染端 keep 保活本帧缓冲，接收端发布新帧不悬垂旧帧
//  （与解码线程化后的 FrameView.keep 同一模式）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
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
    /// 【启用闸 2026-11-23】asset.live_enabled=false：不进 Hub 不直收，
    /// prepare 恒成功但无收端（produce 恒空 → 占位）——收流/捕获的唯一
    /// 运行开关，与 LiveHub::ensureAsset 同源；启停经调度器指纹触发重建。
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

private:
    // ---- Hub 模式（editor 预览进程内直用）----
    std::shared_ptr<LiveHub::Feed> hub_feed_; ///< 非空 = Hub 模式（prepare 固定）

    // ---- 直收模式（内嵌收端核心；类型由 Ingest 注册表分派）----
    std::unique_ptr<ILiveIngest> ingest_; ///< 收端（sink 回调写 latest_）

    // ---- 帧快照（直收模式：sink 在接收线程发布 / 渲染线程读取）----
    mutable std::mutex              snap_mutex_;
    LiveFramePtr                    latest_;
    /// 帧同步排程缓冲（target_latency>0 时使用；按到达序 = pts 升序）。
    /// 与 latest_ 同锁保护；latest_ 恒 = 最新帧（监视语义直读用）。
    std::deque<LiveFramePtr>        sync_buffer_;
    /// 最近一次排程选中帧（frameView 排程选帧结果缓存；与 sync_buffer_
    /// 同锁 snap_mutex_ 保护——const 方法内更新故 mutable）。
    mutable LiveFramePtr            last_shown_;
    void storeFrame(LiveFramePtr frame); ///< sink 回调（接收线程）

    // ---- 配置（prepare 期固定）----
    std::string source_name_;  ///< NDI 源名（诊断/重连日志用）
    int         latency_mode_ = 0;
    /// 帧同步延迟预算（微秒；0 = 监视语义 latest 直显）。帧 pts 已含该
    /// 预算（捕获端打戳：捕获 + 预算 = 应显时刻）——本类仅用它算排程
    /// 缓冲容量，排程公式恒 "now"（按帧自带 pts 到点回放）。
    std::int64_t target_latency_us_ = 0;
    /// 排程缓冲容量（帧数，prepare 期按 target_latency 动态计算）：
    /// 必须 ≥ target_latency 对应的历史帧数，否则"未到期帧被容量裁剪"
    /// → 选帧恒空 → 永久占位黑屏（审查修复 2026-11-21）。按 30fps 保守
    /// 估计所需历史 + 8 帧余量，clamp [16, 48]（48×8.3MB≈400MB 极限）。
    std::size_t sync_capacity_ = 16;

    // ---- 帧同步（2026-11-25 语义定稿：应显时刻随帧走）----
    // 帧的 pts_us = 应显时刻（捕获端统一打戳：捕获时刻 + 延迟预算；
    // "editor 端捕获时就把 120ms 加上"用户定案）——display 片段层只按
    // 帧自带 pts 到点回放（now ≥ pts 显示；早到等 / 晚到顺延），不做
    // 任何钟差补偿（时钟同步归 display 底层 SNTP/ClockProvider——片段
    // 层越层补偿已撤销：v14f 中位数被排队延迟污染致本机幻灯片）。
    // target_latency_us_ 仅用于缓冲容量计算（帧早到约一个预算时长，
    // 缓冲需容纳"未来帧"）。
    /// 排程健康日志限频（frameView 渲染线程写，snap_mutex_ 内，故 mutable）
    mutable std::int64_t last_sched_log_us_ = 0;

    // ---- 诊断 ----
    std::atomic<bool>          connected_{false};
    std::atomic<std::uint64_t> frames_received_{0};
};

} // namespace ms
