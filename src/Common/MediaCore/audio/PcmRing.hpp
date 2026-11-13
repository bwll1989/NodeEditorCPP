// ============================================================================
//  PcmRing.hpp —— 交错 float32 PCM 环形缓冲（音频域的跨线程交接带）
//
//  用途（2026-11-10 P1/P2 共用）：
//    * AudioOut（WASAPI）：混音拍（生产者）→ 设备渲染线程（消费者）——
//      Qt 定时器节拍与设备 48k 时钟的速率差/抖动由本环吸收（P4 漂移
//      纠偏前的缓震层；溢出丢弃 + 欠载补静音均由调用方计数）；
//    * AudioMediaSource：解码线程（生产者）→ 混音拍（消费者）——解码
//      前置量经本环解耦（生产满 = 阻塞等待，消费不足 = 返回现有量）。
//
//  同步原语：std::mutex + 条件变量。SPSC 无锁版留待实测有争用再换——
//  当前两线程各以 ≤10ms 粒度触碰，临界区微秒级，正确性优先。
//  游标语义：读/写位置为【累计帧数】（不回绕），访问按 % 容量取模。
// ============================================================================

#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

namespace ms {

class PcmRing {
public:
    PcmRing(std::uint32_t channels, std::uint64_t capacity_frames)
        : channels_(channels ? channels : 1),
          cap_frames_(capacity_frames ? capacity_frames : 1),
          buf_(static_cast<std::size_t>(cap_frames_) * channels_, 0.0f) {}

    /// 尝试推入（满则只推能容纳的前缀，返回实推帧数；0 = 全满）。
    /// AudioOut 生产侧（UI 拍）用非阻塞语义——溢出丢弃由调用方计数。
    std::uint64_t tryPush(const float* interleaved, std::uint64_t frames) {
        std::lock_guard<std::mutex> lk(m_);
        const std::uint64_t n = freeFramesLocked();
        const std::uint64_t pushed = std::min(frames, n);
        copyIn(interleaved, pushed);
        if (pushed > 0) not_empty_.notify_all();
        return pushed;
    }

    /// 阻塞推入（解码线程生产侧：环满等待空间，推完返回实推数）。
    /// \param stop_fn 谓词（每段推入前检查）：返回 true 即中止——
    ///                停止旗标与"代次已变（seek 重灌）"共用本出口
    template <typename StopFn>
    std::uint64_t pushBlocking(const float* interleaved, std::uint64_t frames,
                               StopFn&& stop_fn) {
        std::uint64_t pushed = 0;
        std::unique_lock<std::mutex> lk(m_);
        while (pushed < frames) {
            // 中止谓词在【每次推入前】检查——环被 clear() 唤醒后若只在
            // "满"分支检查，唤醒的生产者会先把旧代次数据灌回环内
            // （seek 重灌语义被破坏），再等下一轮满时才中止。
            if (stop_fn()) break;
            const std::uint64_t free_f = freeFramesLocked();
            if (free_f == 0) {
                not_full_.wait_for(lk, std::chrono::milliseconds(50));
                continue;
            }
            const std::uint64_t n =
                std::min<std::uint64_t>(frames - pushed, free_f);
            copyIn(interleaved + pushed * channels_, n);
            pushed += n;
            not_empty_.notify_all();
        }
        return pushed;
    }

    /// 取走现有（最多 max_frames；返回实取帧数——不足由调用方补零）
    std::uint64_t pop(float* dst, std::uint64_t max_frames) {
        std::lock_guard<std::mutex> lk(m_);
        const std::uint64_t n = std::min(max_frames, write_f_ - read_f_);
        copyOut(dst, n);
        if (n > 0) not_full_.notify_all();
        return n;
    }

    /// 丢弃最旧 n 帧（读侧跳帧追赶用）
    std::uint64_t drop(std::uint64_t frames) {
        std::lock_guard<std::mutex> lk(m_);
        const std::uint64_t n = std::min(frames, write_f_ - read_f_);
        read_f_ += n;
        if (n > 0) not_full_.notify_all();
        return n;
    }

    /// 当前可读帧数
    std::uint64_t available() const {
        std::lock_guard<std::mutex> lk(m_);
        return write_f_ - read_f_;
    }
    /// 清空（seek 重灌语义；调用方保证此刻无并发读写方持有旧区间）
    void clear() {
        std::lock_guard<std::mutex> lk(m_);
        read_f_ = write_f_;
        not_full_.notify_all();
    }

    std::uint64_t capacityFrames() const noexcept { return cap_frames_; }
    std::uint32_t channels() const noexcept { return channels_; }

private:
    std::uint64_t freeFramesLocked() const {
        return cap_frames_ - (write_f_ - read_f_);
    }
    void copyIn(const float* src, std::uint64_t n) {
        while (n > 0) {
            const std::uint64_t w = write_f_ % cap_frames_;
            const std::uint64_t run = std::min<std::uint64_t>(n, cap_frames_ - w);
            std::copy(src, src + run * channels_,
                      buf_.begin() + static_cast<std::ptrdiff_t>(w * channels_));
            src += run * channels_;
            write_f_ += run;
            n -= run;
        }
    }
    void copyOut(float* dst, std::uint64_t n) {
        while (n > 0) {
            const std::uint64_t r = read_f_ % cap_frames_;
            const std::uint64_t run = std::min<std::uint64_t>(n, cap_frames_ - r);
            std::copy(buf_.begin() + static_cast<std::ptrdiff_t>(r * channels_),
                      buf_.begin() + static_cast<std::ptrdiff_t>((r + run) * channels_),
                      dst);
            dst += run * channels_;
            read_f_ += run;
            n -= run;
        }
    }

    // 注意：成员初始化按【声明顺序】执行——channels_/cap_frames_ 必须排在
    // buf_ 之前，否则 buf_ 构造时读到未初始化的容量值（垃圾尺寸 = 巨量
    // 分配或越界写，症状为 bad allocation / vector too long / 段错误）。
    std::uint32_t channels_;            ///< 通道数（交错布局步长）
    std::uint64_t cap_frames_;          ///< 环容量（帧）
    mutable std::mutex m_;
    std::condition_variable not_empty_; ///< 生产→消费：有新数据可取
    std::condition_variable not_full_;  ///< 消费→生产：有空位可推
    std::vector<float>      buf_;       ///< 交错 PCM（帧 × 通道）
    std::uint64_t read_f_  = 0;         ///< 读游标（累计帧）
    std::uint64_t write_f_ = 0;         ///< 写游标（累计帧）
};

// ---------------------------------------------------------------------------
// ringTrimToLevel —— 水位裁齐（P4 漂移纠偏的产出侧执行器，2026-11-10）
//
// 语义：供给速率（会话时钟锁定产出）略高于设备消费速率时，环水位单调
// 缓涨——延迟持续增大；超过高水位线（high）后一次性丢弃最旧样本直至
// 目标水位（target），把输出延迟钉回目标值。稳态速率差为 ppm 级，
// 触发频率极低（100ms 漂移在 200ppm 下约 8 分钟累积一次）；单次裁齐
// 幅度 = 漂移量本身，无逐窗微跳。抽成自由函数：AudioOut::write 调用 +
// 单测直接验证（headless 环境无设备也能确定性覆盖纠偏语义）。
// ---------------------------------------------------------------------------
inline std::uint64_t ringTrimToLevel(PcmRing& ring, std::uint64_t target,
                                     std::uint64_t high) {
    const std::uint64_t avail = ring.available();
    if (avail <= high || avail <= target) return 0;
    return ring.drop(avail - target);
}

} // namespace ms

