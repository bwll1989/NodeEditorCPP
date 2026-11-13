// ============================================================================
//  IngestBase 实现 —— 采集器公共骨架（设计意图见头注释）
// ============================================================================

#include "live/IngestBase.hpp"

#include <chrono>
#include <cstdio>

#include <windows.h> // WideCharToMultiByte / MultiByteToWideChar

namespace ms {

// ---------------------------------------------------------------------------
// 生命周期模板方法（start/stop 幂等；诊断/线程骨架唯一实现）
// ---------------------------------------------------------------------------
bool IngestBase::start() {
    if (running_.load(std::memory_order_acquire)) return true; // 幂等
    if (!onStart()) return false; // 子类前置（运行时初始化/系统能力检查）
    running_.store(true, std::memory_order_release); // 先置位：线程首查即真
    try {
        worker_ = std::thread([this] { recvLoopShell(); });
    } catch (...) {
        running_.store(false, std::memory_order_release);
        onCleanup(); // 对称回收前置资源（NDI recv/finder 尚未建——幂等跳过）
        return false;
    }
    return true;
}

void IngestBase::stop() {
    if (running_.load(std::memory_order_acquire)) {
        running_.store(false, std::memory_order_release);
        if (worker_.joinable()) worker_.join();
    }
    onCleanup(); // 子类资源销毁（join 后调用——无并发；必须幂等）
    connected_.store(false, std::memory_order_release);
}

bool IngestBase::connected() const noexcept {
    return connected_.load(std::memory_order_relaxed);
}

std::uint64_t IngestBase::framesReceived() const noexcept {
    return frames_received_.load(std::memory_order_relaxed);
}

std::uint64_t IngestBase::framesDropped() const noexcept {
    return frames_dropped_.load(std::memory_order_relaxed);
}

IngestBase::IngestBase(std::string source_ref, int latency_mode, FrameSink sink)
    : source_ref_(std::move(source_ref)),
      latency_mode_(latency_mode),
      sink_(std::move(sink)) {}

IngestBase::~IngestBase() { stop(); } // 未 stop 的实例兜底停线程

// ---------------------------------------------------------------------------
// 线程入口：异常兜底壳 —— 任何漏接异常都不允许击穿 std::thread
// （std::terminate → abort → 进程退出码 3；NDI 原版缺壳，本次统一）
// ---------------------------------------------------------------------------
void IngestBase::recvLoopShell() {
    try {
        recvLoop();
    } catch (...) {
        std::fprintf(stderr, "[%s] 接收线程异常退出（兜底捕获）\n", logTag());
        std::fflush(stderr);
    }
    connected_.store(false, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// 子类工具：限频日志 + 帧出口
// ---------------------------------------------------------------------------
void IngestBase::logThrottled(const std::string& msg, std::int64_t& last_ms,
                              std::int64_t gap_ms) const noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const std::int64_t cur =
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    if (last_ms >= 0 && cur - last_ms < gap_ms) return;
    last_ms = cur;
    std::fprintf(stderr, "[%s] %s\n", logTag(), msg.c_str());
    std::fflush(stderr);
}

void IngestBase::emitFrame(LiveFramePtr f) {
    if (f == nullptr) return;
    if (sink_) sink_(std::move(f));
    frames_received_.fetch_add(1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// 编码工具（wide ↔ UTF-8；各 Ingest 枚举/连接共用）
// ---------------------------------------------------------------------------
std::string utf8Of(const wchar_t* w) {
    if (w == nullptr) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0,
                                      nullptr, nullptr);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n,
                                   nullptr, nullptr);
    return s;
}

std::wstring wideOf(const std::string& u8) {
    if (u8.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, u8.c_str(),
                                      static_cast<int>(u8.size()),
                                      nullptr, 0);
    std::wstring w(n > 0 ? n : 0, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, u8.c_str(),
                                   static_cast<int>(u8.size()), w.data(), n);
    return w;
}

} // namespace ms
