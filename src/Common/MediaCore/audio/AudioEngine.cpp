// ============================================================================
//  AudioEngine.cpp —— 混音引擎实现（位置直通求和 + 时域 RMS 电平）
// ============================================================================

#include "audio/AudioEngine.hpp"

#include <algorithm>
#include <cmath>

#include "audio/IAudioOutput.hpp"

namespace ms {

namespace {
// 声源侧最大通道数已上移 AudioTypes.hpp（kMaxSourceChannels）：声道契约
// 需被接口注释/选配层告警/引擎钳制三处共用，引擎私有不满足闭环。
//（kMaxWindowFrames/kMixRate 同步上移 AudioEngine.hpp：窗长换算 helper
// 与两处调用点共用）
} // namespace

// ---------------------------------------------------------------------------
// attach：输出实现替换（解偶缝——引擎对其余部分零感知）
// ---------------------------------------------------------------------------
void AudioEngine::attach(std::unique_ptr<IAudioOutput> out) {
    if (out_) out_->close();
    out_ = std::move(out);
    if (out_ && running_) out_->open(spec_); // 运行中替换：按当前 spec 重开
}

const char* AudioEngine::outputName() const noexcept {
    return out_ ? out_->name() : "无";
}

void AudioEngine::flushOutput() {
    if (out_) out_->flush();
}

// ---------------------------------------------------------------------------
// start / stop：混音会话生命周期
// ---------------------------------------------------------------------------
bool AudioEngine::start(const AudioSpec& spec) {
    spec_    = spec;
    running_ = true;
    // 缓冲按 spec 分配（交错混合缓冲 + 单源平面读取缓冲）；上限按可变
    // 窗长封顶——mixFrames 的最大窗不得触发逐拍 resize（start 一次到位）
    const std::size_t max_frames =
        (std::max)(static_cast<std::size_t>(spec.frames_per_tick),
                   static_cast<std::size_t>(kMaxWindowFrames));
    mixbuf_.assign(max_frames * static_cast<std::size_t>(spec.channels),
                   0.0f);
    tmpbuf_.assign(max_frames * kMaxSourceChannels, 0.0f);
    level_.store(0.0f, std::memory_order_relaxed);
    if (out_) return out_->open(spec_);
    return true; // 无输出 = 静默混音（电平链路照常，验证场景合法）
}

void AudioEngine::stop() {
    running_ = false;
    level_.store(0.0f, std::memory_order_relaxed);
    if (out_) out_->close();
}

// ---------------------------------------------------------------------------
// mix：逐声源读取 → 增益求和 → 输出 → RMS（混合内核 sumInto 见下）
// ---------------------------------------------------------------------------
namespace {
// 声源混合进交错输出缓冲（增益乘在求和处——单次乘法每样本）。
// 抽出成自由函数：单声道/多声道两快路径各自整段循环，内核无逐样本
// 分支（此前分支在内层通道循环里，mono 时每样本白算通道数-1 次判断）。
inline void sumInto(float* mixbuf, std::uint32_t out_ch,
                    const float* src, std::uint32_t src_ch,
                    std::uint32_t frames, float gain) noexcept {
    if (src_ch == 1) { // 单声道：每样本一乘、逐输出通道同值累加
        for (std::uint32_t f = 0; f < frames; ++f) {
            const float s = src[f] * gain;
            for (std::uint32_t c = 0; c < out_ch; ++c)
                mixbuf[static_cast<std::size_t>(f) * out_ch + c] += s;
        }
        return;
    }
    // 多声道：声道号逐位直拷（源超出输出部分丢弃）
    const std::uint32_t n = std::min(src_ch, out_ch);
    for (std::uint32_t f = 0; f < frames; ++f)
        for (std::uint32_t c = 0; c < n; ++c)
            mixbuf[static_cast<std::size_t>(f) * out_ch + c] +=
                src[static_cast<std::size_t>(f) * src_ch + c] * gain;
}
} // namespace

// ---------------------------------------------------------------------------
// mix：逐声源读取 → 增益求和 → 输出 → RMS
// 混合内核（位置直通最小实现，验证期输出立体声）：
//   * 源声道 1（单声道）→ FL/FR 同值复制（位置并集下 mono 居中）；
//   * 源声道 ≥2 → 前 min(源,输出) 通道逐位直拷（L→L R→R 映射恒不变），
//     多余源通道丢弃（无系数折叠）；源不足的输出通道保持 0。
// ---------------------------------------------------------------------------
void AudioEngine::mix(const std::vector<AudioVoice>& voices,
                      std::int64_t t_us) {
    mixFrames(voices, t_us, spec_.frames_per_tick);
}

void AudioEngine::mixFrames(const std::vector<AudioVoice>& voices,
                            std::int64_t /*t_us*/, std::uint32_t frames_in) {
    if (!running_) return;
    // 窗长钳制（可变窗入口的防爆窗闸门；固定窗 mix 恒 ≤ spec 值不受影响）
    const auto frames =
        std::min(frames_in, kMaxWindowFrames);
    if (frames == 0) return;
    const auto channels = static_cast<std::uint32_t>(spec_.channels);

    std::fill(mixbuf_.begin(), mixbuf_.end(), 0.0f);

    for (const auto& v : voices) {
        if (!v.source || v.gain <= 0.0f) continue; // 静音/无效源零成本跳过
        // 声源读取：源声道数由源自声明（channelCount），按源布局交错写入
        const auto src_ch =
            std::min(v.source->channelCount(), kMaxSourceChannels);
        const std::size_t need = static_cast<std::size_t>(frames) * src_ch;
        if (tmpbuf_.size() < need) tmpbuf_.resize(need);
        const auto got = v.source->read(tmpbuf_.data(), v.media_us, frames);
        if (got == 0) continue;
        // read 契约：返回 < frames 时剩余区段由调用方补零——源提前结束
        // 的尾部不得残留上一声源/上一拍的旧样本（否则旧声音泄进本拍）
        if (got < frames)
            std::fill(tmpbuf_.begin() + static_cast<std::ptrdiff_t>(got) * src_ch,
                      tmpbuf_.begin() + static_cast<std::ptrdiff_t>(frames) * src_ch,
                      0.0f);
        sumInto(mixbuf_.data(), channels, tmpbuf_.data(), src_ch,
                frames, v.gain);
    }

    // 输出写出（解偶缝——Null 实现丢弃；后续 WASAPI 实现写设备）
    if (out_) out_->write(mixbuf_.data(), frames);

    // RMS 电平（Gist rootMeanSquare 同公式 √(Σx²/N)，但只对已用前缀
    // frames×channels 求——mixbuf_ 按 100ms 爆窗上限分配，整缓冲求值会把
    // 未用尾段当静音摊薄，电平表恒偏小 √(实际窗/分配窗)）。窗口静音时 0。
    const std::size_t used =
        static_cast<std::size_t>(frames) * static_cast<std::size_t>(spec_.channels);
    double acc = 0.0;
    for (std::size_t i = 0; i < used; ++i) acc += double(mixbuf_[i]) * mixbuf_[i];
    const float rms = static_cast<float>(std::sqrt(acc / double(used)));
    level_.store(rms, std::memory_order_relaxed);
}

} // namespace ms
