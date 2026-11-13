// ============================================================================
//  SineAudioSource.hpp —— 验证用正弦测试音源（IAudioSource 唯一实现）
//
//  设计依据：《audio-output-implementation-plan.md》验证阶段（2026-11-10）：
//    * 真实媒体音频解码（AudioMediaSource / 视频音轨副通道）按计划 P2
//      接入；验证期用正弦源驱动整条"增益曲线 → 混音 → RMS 电平"链路——
//      电平表的起伏即增益 tween 曲线的可视化；
//    * 相位由 media_us 推导（确定性纯函数，无内部相位状态）——同一
//      (media_us, freq) 恒得同一段样本，与 TimelineEngine 纯函数纪律同构；
//    * 频率可按片段区分（多片段同播可听出叠加/拍频）。
// ============================================================================

#pragma once

#include <cmath>
#include <cstdint>

#include "audio/AudioTypes.hpp"

namespace ms {

class SineAudioSource final : public IAudioSource {
public:
    /// \param freq_hz 正弦频率（按片段哈希取 220~880Hz 可听域，多源可辨）
    /// \param amp     基准幅度 [0,1]（0.25 恒定——增益曲线才是电平的主角）
    SineAudioSource(double freq_hz = 440.0, float amp = 0.25f) noexcept
        : freq_hz_(freq_hz), amp_(amp) {}

    std::uint32_t channelCount() const noexcept override { return 1; }

    std::uint32_t read(float* dst, std::int64_t media_us,
                       std::uint32_t frames) override {
        // 相位起点 = media_us（素材内时间即波形时间轴——seek/裁剪天然对位）
        const double t0 = static_cast<double>(media_us) * 1e-6;
        for (std::uint32_t f = 0; f < frames; ++f) {
            const double t =
                t0 + static_cast<double>(f) / 48'000.0;
            dst[f] = amp_ * static_cast<float>(
                                 std::sin(2.0 * 3.14159265358979323846 *
                                          freq_hz_ * t));
        }
        return frames;
    }

private:
    double freq_hz_;
    float  amp_;
};

} // namespace ms
