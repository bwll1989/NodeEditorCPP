// ============================================================================
//  IngestBase —— 四个采集器实现（Ndi/Spout/Wgc/Camera）的公共骨架
//  （2026-11-09 复用整理：四份逐行同构的生命周期样板收敛一处）
//
//  收敛的样板（原四份 ×~80 行逐行同构——Ndi/Spout/Wgc/Camera 链式复制）：
//    * start() 幂等 + 线程创建（先置 running 再起线程；创建失败回滚）；
//    * stop() join + connected 复位（幂等）；
//    * 接收线程异常兜底壳（任何漏接异常不得击穿 std::thread →
//      terminate → abort → 进程退出码 3；NDI 原版缺壳——本次统一补齐）；
//    * 诊断三原子（connected / frames_received / frames_dropped）；
//    * 限频日志（[logTag] 前缀）。
//
//  子类只实现：
//    * recvLoop()   —— 接收主体（发现/连接/拉帧循环，自愈语义自持）；
//    * logTag()     —— 日志前缀；
//    * onStart()    —— 启动前置（NDI 运行时 acquire / WGC IsSupported）；
//    * onCleanup()  —— stop 尾部资源销毁（join 后调用；必须幂等）。
//
//  帧出口契约：sink_ 在接收线程执行（订阅方自行保证发布安全）；
//  emitFrameFrame 统一"上抛 + 收帧计数"。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

#include "live/LiveIngest.hpp"

namespace ms {

class IngestBase : public ILiveIngest {
public:
    bool start() override final;
    void stop()  override final;

    bool          connected()      const noexcept final;
    std::uint64_t framesReceived() const noexcept final;
    std::uint64_t framesDropped()  const noexcept final;

protected:
    IngestBase(std::string source_ref, int latency_mode, FrameSink sink);
    ~IngestBase() override; // 未 stop 的实例兜底停线程（RAII）

    IngestBase(const IngestBase&)            = delete;
    IngestBase& operator=(const IngestBase&) = delete;

    // ---- 子类钩子 ----
    /// 接收主体（在异常兜底壳内执行；循环以 running() 为退出条件）
    virtual void recvLoop() = 0;
    /// start 前置（运行时初始化/系统能力检查；false = 拒绝启动）——默认恒真
    virtual bool onStart() { return true; }
    /// stop 尾部资源销毁（join 后调用；必须幂等——NDI recv/finder 销毁等）。
    /// 注意：start 失败（线程创建失败）也会调用——子类按幂等释放
    virtual void onCleanup() {}
    /// 日志前缀（"ndi"/"spout"/"wgc"/"camera"）
    virtual const char* logTag() const noexcept = 0;

    // ---- 子类工具 ----
    /// 限频日志（接收线程专用；输出 "[<tag>] <msg>"）
    void logThrottled(const std::string& msg, std::int64_t& last_ms,
                      std::int64_t gap_ms) const noexcept;
    /// 帧出口：sink 上抛 + 收帧计数（接收线程调用）
    void emitFrame(LiveFramePtr f);
    /// 帧丢弃计数
    void countDropped() noexcept { frames_dropped_.fetch_add(1, std::memory_order_relaxed); }

    // ---- 配置（start 期固定；线程只读）----
    const std::string source_ref_; ///< 连接串（NDI 源名 / sender 名 / SymbolicLink / ref）
    const int         latency_mode_; ///< 0=最低 1=同步（语义按通道解释）
    FrameSink         sink_;       ///< 帧出口（接收线程调用）

    // ---- 运行态（子类循环以 running() 为退出条件）----
    std::thread       worker_;
    std::atomic<bool> running_{false};

    // ---- 诊断（子类可直接置位）----
    std::atomic<bool>          connected_{false};
    std::atomic<std::uint64_t> frames_received_{0};
    std::atomic<std::uint64_t> frames_dropped_{0};

private:
    /// 线程入口 = 异常兜底壳（recvLoop 漏接异常不得击穿 std::thread）
    void recvLoopShell();
};

/// wide → UTF-8（Ingest 枚举/连接共用；合并自 WgcIngest/CameraIngest 的
/// 重复匿名副本。wchar_t 是 C++ 核心类型——头文件不引 windows.h）
std::string utf8Of(const wchar_t* w);

/// UTF-8 → wide（同上；实现文件含 windows.h，头保持平台中立）
std::wstring wideOf(const std::string& u8);

} // namespace ms
