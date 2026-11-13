// ============================================================================
//  FramePacer.hpp —— 60fps present 相位对齐（§5.6 设计文档指出的最大缺口）
//
//  问题（§5.6）："present 按统一时钟的帧调度时机提交 swap，确保多端同一
//  物理时刻出帧"。若各端各自"解算完就 present"，两端的提交时刻会随机
//  分布在一个刷新周期内（S2 风险：两屏点亮差 ≈ 一个刷新周期且不稳定）。
//
//  方案（应用层 present 对齐调度）：
//    * 所有端共享同一 NTP 对齐时钟 → 把 present 时刻对齐到【共享时钟的
//      帧栅格】上：栅格点 g(i) = epoch + i * 1e6 / fps（整数微秒）；
//    * 帧序号由共享时钟直接推导：i = (now - epoch) * fps / 1e6 ——
//      无需任何端间通信，同一物理时刻各端推导出【同一帧序号】，
//      各自渲染 g(i) 时刻的画面并等待到 g(i) 才提交 swap；
//    * 残余点亮差 = NTP 对齐误差（亚毫秒）+ swap 提交耗时抖动，
//      远小于一个刷新周期（16.7ms@60fps）。
//
//  长期无累积漂移（关键）：
//    栅格点用 (i * 1_000_000) / fps 整数除法【floor 语义，与帧序号推导的
//    floor 互逆自洽】，而非 i * floor(1e6/fps) 累加——后者每帧丢 0.67us，
//    1 小时漂移 ~2.4ms。第 1000 帧栅格 = 16'666'666us（floor 精确值），
//    而非朴素累加的 16'667'000us。
//
//  时间注入：本类不做任何时钟读取（tick 注入模式，与 Runtime 一致），
//  纯数学函数全部可单测；真实等待 waitUntil 由主循环用本地时钟驱动。
// ============================================================================

#pragma once

#include <cstdint>
#include <thread>

namespace ms {

class FramePacer {
public:
    /// \param fps 目标帧率（60 = 60fps；<=0 退化 30）
    /// \param epoch_us 帧栅格基准时刻（对齐时钟域，us；通常取 session_start，
    ///                 使帧序号与会话时间轴对齐）
    explicit FramePacer(int fps, std::int64_t epoch_us = 0) noexcept
        : epoch_us_(epoch_us),
          fps_(fps > 0 ? fps : 30) {}

    int  fps() const noexcept { return fps_; }
    std::int64_t epoch() const noexcept { return epoch_us_; }

    /// 帧序号 i 的栅格时刻（对齐时钟域，us）：g(i) = epoch + i*1e6/fps
    /// 整数先乘后除——任意远的帧都无累积漂移
    std::int64_t gridTimeUs(std::uint64_t frame_index) const noexcept {
        return epoch_us_ + frame_index * 1'000'000 / fps_;
    }

    /// 对齐时钟 now → 当前应渲染的帧序号
    /// 语义：now ∈ [g(i), g(i+1)) → 帧 i（栅格点恰落在 now 时属新帧）。
    /// 互逆公式推导：g(i) = floor(i*1e6/fps) <= now
    ///   ⟺ i*1e6 < (now-epoch+1)*fps ⟺ i = ((t+1)*fps - 1) / 1e6（floor 除）
    /// （不能用 floor(t*fps/1e6)：now 恰为栅格点 g(i) 时会回退到帧 i-1，
    ///   破坏与 gridTimeUs 的互逆性——同一帧各端必须解算同一时刻）
    std::uint64_t frameIndexOf(std::int64_t now_us) const noexcept {
        if (now_us < epoch_us_) return 0; // 会话开始前钳 0
        const std::int64_t t = now_us - epoch_us_;
        return static_cast<std::uint64_t>((t + 1) * fps_ - 1) / 1'000'000;
    }

    /// 对齐时钟 now → 该帧的目标会话时刻（解算用：produce 以此取帧，
    /// 各端对同一帧序号解算同一时刻 → 渲染同一画面）
    std::int64_t frameSessionTimeUs(std::int64_t now_us) const noexcept {
        return gridTimeUs(frameIndexOf(now_us)) - epoch_us_;
    }

    /// 对齐时钟 now → 下一次 present 的目标栅格时刻（本帧栅格若已过
    /// 则顺延一帧）。渲染主循环：解算当前帧 → 等待到该时刻 → present
    std::int64_t nextPresentTimeUs(std::int64_t now_us) const noexcept {
        return gridTimeUs(frameIndexOf(now_us) + 1);
    }

    /// 阻塞等待到目标时刻（真实主循环用；now_fn 返回对齐时钟 us）
    /// 策略：剩余 >2ms 先 sleep 让出 CPU，最后 2ms 自旋——
    /// 规避 Windows sleep 精度（1~15ms）导致的系统性迟到
    template <typename NowFn>
    void waitUntil(NowFn now, std::int64_t target_us) const {
        for (;;) {
            const std::int64_t remain = target_us - now();
            if (remain <= 0) return;
            if (remain > 2'000)
                std::this_thread::sleep_for(
                    std::chrono::microseconds(remain - 2'000));
            else
                std::this_thread::yield();
        }
    }

private:
    std::int64_t epoch_us_; ///< 帧栅格基准（对齐时钟域）
    int          fps_;      ///< 目标帧率
};

} // namespace ms
