// ============================================================================
//  ClockProvider.cpp —— 复合对齐时钟实现
//
//  为什么需要"单调钟推进 + offset"而不是直接读系统墙钟？
//    NTP 客户端（尤其 Windows w32time 默认策略）在偏差过大时会 step——
//    墙钟瞬间前跳/后跳数十毫秒甚至秒级。若直接读墙钟做 session_time，
//    播放头会瞬跳（画面抽搐、音频参考错位）。
//    本实现构造时锚定一次墙钟，之后用 steady_clock（单调、不受系统改时
//    影响）推进；NTP 的持续微调通过 offset 原子量缓慢注入（配合 slew 模式），
//    读取值平滑连续。
// ============================================================================

#include "clock/ClockProvider.hpp"

#include <chrono>

namespace ms {

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

// ---------------------------------------------------------------------------
// 构造：锚定 (墙钟, 单调钟) 当前值对；offset 初始为 0（未校准假设时钟即准，
// Phase 2 由 NTP 探测/SyncClient 设置真实 offset）
// ---------------------------------------------------------------------------
ClockProvider::ClockProvider() noexcept
    : anchor_wall_us_(wallNowRawUs()),
      anchor_mono_us_(monoNowRawUs()),
      offset_us_(0) {}

// ---------------------------------------------------------------------------
// offset 原子存取
// ---------------------------------------------------------------------------
void ClockProvider::setOffsetUs(std::int64_t offset_us) noexcept {
    offset_us_.store(offset_us, std::memory_order_relaxed);
}

std::int64_t ClockProvider::offsetUs() const noexcept {
    return offset_us_.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// 原始时钟读取（静态工具）
// ---------------------------------------------------------------------------
std::int64_t ClockProvider::wallNowRawUs() const noexcept {
    // system_clock：Unix 纪元起的墙钟；duration_cast 到微秒
    return duration_cast<microseconds>(system_clock::now().time_since_epoch())
        .count();
}

std::int64_t ClockProvider::monoNowRawUs() const noexcept {
    // steady_clock：单调递增，不受系统时间修改/NTP step 影响
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch())
        .count();
}

// ---------------------------------------------------------------------------
// 复合读法：aligned = anchor_wall + (mono_now - anchor_mono) + offset
//   * mono_now - anchor_mono：自锚定以来真实流逝的时间（单调可靠）
//   * 加上锚点墙钟值 → 连续推进的"虚拟墙钟"
//   * 加上 NTP offset → 对齐到全场共享基准
// ---------------------------------------------------------------------------
std::int64_t ClockProvider::alignedNowUs() const noexcept {
    const std::int64_t mono_delta = monoNowRawUs() - anchor_mono_us_;
    return anchor_wall_us_ + mono_delta + offsetUs();
}

// ---------------------------------------------------------------------------
// 原始墙钟（诊断用：与 alignedNow 的差值应约等于 -offset + 漂移）
// ---------------------------------------------------------------------------
std::int64_t ClockProvider::wallNowUs() const noexcept {
    return wallNowRawUs();
}

} // namespace ms
