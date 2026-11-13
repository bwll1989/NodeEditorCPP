// ============================================================================
//  DriftController.hpp —— 失同步检测与纠偏决策（§7.1，纯函数）
//
//  设计依据：《media-server-sync-design.md》§7.1（检测/决策/纠偏）：
//    每帧本地算 target = aligned_now - session_start（"应播到哪"）与
//    current = 本机实际渲染到的播放头，delta = target - current：
//      * |delta| < resume 阈值（默认 2ms）   → 不动作（在环内，避免抖动）
//      * 40ms < |delta|                      → 硬 seek（跳变代价 < 长期不同步）
//      * 其余（1~40ms）                      → 微调 playbackRate 平滑追平
//        （delta>0 本机落后 → 加速；delta<0 本机超前 → 减速；上限 ±0.5%）
//
//  纯函数设计：无内部状态，rate 由调用方持有并按决策步进/复位——便于单元
//  测试覆盖全部分支，也让"播放头推进"与"纠偏决策"解耦（渲染循环只管执行）。
// ============================================================================

#pragma once

#include <cmath>
#include <cstdint>

namespace ms {

class DriftController {
public:
    enum class Action : int {
        None = 0,     ///< 偏差在环内，不动作
        SpeedUp = 1,  ///< 落后 → 提速追赶（rate += step）
        SlowDown = 2, ///< 超前 → 降速等待（rate -= step）
        Seek = 3,     ///< 偏差过大 → 硬 seek（无跳变上限，正确性优先）
    };

    struct Decision {
        Action action = Action::None;
        double rate = 1.0;       ///< 决策后建议的播放速率（None 时为当前值）
        std::int64_t seek_target_us = 0; ///< 仅 Seek 时有效：应跳到的播放头
    };

    /// 纠偏决策
    /// \param target_us  应播到的播放头（共享时钟推导）
    /// \param current_us 本机实际播放头
    /// \param current_rate 当前播放速率（决策在其上步进）
    /// \param seek_threshold_us 硬 seek 阈值（默认 40ms，§7.1）
    /// \param in_band_us     视为"已同步"的恢复阈值（默认 2ms，提供滞回防抖）
    /// \param max_rate_step  单次调速幅度（默认 0.5%，即 ±0.005）
    static Decision decide(std::int64_t target_us,
                           std::int64_t current_us,
                           double current_rate = 1.0,
                           std::int64_t seek_threshold_us = 40000,
                           std::int64_t in_sync_threshold_us = 2000,
                           double max_rate_step = 0.005) {
        Decision d;
        d.rate = current_rate;

        const std::int64_t delta = target_us - current_us; // >0 落后，<0 超前
        const std::int64_t abs_delta = delta < 0 ? -delta : delta;

        // ---- 1. 硬 seek：偏差超过阈值（Player 掉线重连/长时间失同步）----
        if (abs_delta > seek_threshold_us) {
            d.action = Action::Seek;
            d.seek_target_us = target_us;
            d.rate = 1.0; // seek 后速率复位，避免残留调速叠加
            return d;
        }

        // ---- 2. 环内：不动作（并让速率向 1.0 回归，避免长期偏离）----
        if (abs_delta <= in_sync_threshold_us) {
            d.action = Action::None;
            d.rate = 1.0;
            return d;
        }

        // ---- 3. 中等偏差：单步调速（方向按 delta 符号，幅度有上限）----
        if (delta > 0) {
            d.action = Action::SpeedUp;
            d.rate = std::min(1.0 + max_rate_step, current_rate + max_rate_step);
        } else {
            d.action = Action::SlowDown;
            d.rate = std::max(1.0 - max_rate_step, current_rate - max_rate_step);
        }
        return d;
    }
};

} // namespace ms
