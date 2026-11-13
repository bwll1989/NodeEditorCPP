// ============================================================================
//  AudioEngine.hpp —— 音频混音引擎（解偶核心：场景零感知 + 输出零感知）
//
//  设计依据：《audio-output-implementation-plan.md》决策 1/3（2026-11-10）：
//    * 主时钟 = session_time（不让路）：mix(t_us, voices) 由调用方以对齐
//      时钟节拍驱动（验证阶段 = 编辑器 echo 33ms 拍；未来 = WASAPI 回调
//      按"未来窗口"预混）——引擎内部无时钟、无线程（与 TimelineEngine
//      同一"纯函数核心 + 薄壳 IO"纪律）；
//    * 场景零感知：输入 = AudioVoice 数组 {源, 增益, 素材内时间}——增益
//      由调用方经 TweenEngine::resolveGain 解算（通道/静态两来源一个出口），
//      引擎不做任何场景/clip 解读；
//    * 输出零感知：混合样本经 IAudioOutput 接口写出（解偶缝，验证实现
//      = NullAudioOutput；后续 WASAPI/其他方式各实现接入零改动）；
//    * 电平检测 = 时域 RMS（√(Σx²/N)，仅对最近混合窗口的已用前缀求
//      值）——原子发布，UI 侧任意拍轮询。
//
//  混合语义（位置直通，通道映射恒不变）：源通道 → 输出槽位按位置位对齐
//  （单声道 → FL/FR 复制；源通道数 > 输出 → 多余丢弃，绝不系数折叠——
//  宁丢 FC 不污染 FL/FR）。验证期输出恒立体声。
// ============================================================================

#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "audio/AudioTypes.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// 混音域常量 + 窗长换算（editor echo 拍与 display 渲染拍共用——单一来源
// 防两处同式漂移）
// ---------------------------------------------------------------------------
/// 混音域采样率（铁律：swr 输出率/环内簿记/窗长换算全 48k——任何一处
/// 写成源采样率都会造成音调漂移或游标失配）
inline constexpr int kMixRate = 48'000;
/// 可变窗长上限（100ms@48k）：mixFrames 超量钳制与窗长换算共用——
/// seek/重锚跳变时调用方传入的 dt 可能数秒，一次性产出会让环爆溢 +
/// 声源 read 巨量前跳（实际由源的 drop/seek 逻辑消化，分窗产出即可）
inline constexpr std::uint32_t kMaxWindowFrames = 4'800;

/// 会话时钟推进量 dt_us → 48k 域样本数（四舍五入，与 AudioMediaSource::read
/// 的 media_us→样本换算同口径，AudioMediaSource.cpp:322-324 先例）。
/// 【S2 修复 2026-11-11】此前 editor/display 两处各自 `dt*48/1'000` 整数
/// 截断：33.333ms 栅格每拍恒向下截去 ~1 帧，供给速率 ≈47970 帧/s，低于
/// 设备消费 48000（欠 604ppm）——环水位缓降 → 周期性 underrun（微爆音
/// + A/V 相位缓漂）。改为先 64 位四舍五入换算、再钳窗上限（次序：先换
/// 算后钳，钳前不丢失精度）。dt_us ≤ 0 防御性返回 0（调用方各自的
/// 跳变/重复拍分支先行处理，正常路径不会触达）。
inline std::uint32_t framesForWindowUs(std::int64_t dt_us) noexcept {
    if (dt_us <= 0) return 0;
    const std::int64_t frames =
        (dt_us * kMixRate + 500'000) / 1'000'000; // 64 位运算防溢出
    return static_cast<std::uint32_t>(
        std::min<std::int64_t>(frames, kMaxWindowFrames));
}

class IAudioOutput;

// ---------------------------------------------------------------------------
// AudioVoice —— 单路混音输入（调用方每拍构造；引擎对来源零解读）
// ---------------------------------------------------------------------------
struct AudioVoice {
    IAudioSource* source  = nullptr; ///< 段生产者（借用指针，本拍内有效）
    float         gain    = 1.0f;    ///< 本拍解算增益（TweenEngine::resolveGain）
    std::int64_t  media_us = 0;      ///< 素材内部时间（TimelineEngine 口径）
};

// ---------------------------------------------------------------------------
// AudioEngine —— 混音器（无时钟无线程；mix 由外部节拍驱动）
// ---------------------------------------------------------------------------
class AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine() = default;
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /// 注入输出实现（解偶缝；可运行中替换——旧实现 close 后交付）。
    /// nullptr = 摘除输出（引擎继续混音/算电平，样本丢弃）。
    void attach(std::unique_ptr<IAudioOutput> out);
    /// 当前输出实现名（无输出 = "无"；诊断/状态栏）。实时转发而非
    /// attach 时缓存：实现名可在 open() 后才确定（如 PortAudio 的
    /// host api 后缀），缓存会拿到 open 前的占位名。实现体在 .cpp
    ///（本头只前置声明 IAudioOutput）。
    const char* outputName() const noexcept;
    /// 丢弃输出侧未播样本（seek/跳变重对位——环内旧位置音频不清会
    /// 按旧内容继续出声，跳转后串音/音画错位）。无输出时空操作。
    /// 实现体在 .cpp（本头只前置声明 IAudioOutput）。
    void flushOutput();

    /// 开始/停止混音会话（spec 决定混音域；stop 后 mix 调用被忽略）
    bool start(const AudioSpec& spec);
    void stop();
    bool running() const noexcept { return running_; }

    /// 混一个窗口（每拍调用）：逐声源 read → 增益求和 → 位置直通 →
    /// 输出 write → RMS 电平发布。voices 空 = 输出静音窗口（电平归零）。
    /// \param t_us 播放头（会话时间；当前实现未消费——真实输出实现接入
    ///             后按需取用（如时间戳交付），时间权威恒在调用方）
    void mix(const std::vector<AudioVoice>& voices, std::int64_t t_us);

    /// 可变窗长混音（P3 display 接线，2026-11-10）：display 渲染拍以
    /// 【会话时钟推进量】产出音频（dt×48k 样本）——音频与画面同一时间
    /// 轴（A/V 同步的根基）；窗长随帧间隔波动（60fps ≈ 800 帧/窗）。
    /// \param frames 本窗样本数（> kMaxWindowFrames 被钳制——防 seek
    ///               跳变一次性产出数秒音频的爆窗）。
    void mixFrames(const std::vector<AudioVoice>& voices, std::int64_t t_us,
                   std::uint32_t frames);

    /// 最近混合窗口的 RMS 电平 [0,~1+]（原子发布；UI 任意拍轮询）
    float level() const noexcept {
        return level_.load(std::memory_order_relaxed);
    }
    /// 混音域规格（start 后有效）
    const AudioSpec& spec() const noexcept { return spec_; }

private:
    AudioSpec spec_{};
    bool      running_ = false;

    std::unique_ptr<IAudioOutput> out_;   ///< 输出实现（解偶缝）

    std::vector<float> mixbuf_;   ///< 混合缓冲（交错；start 时按 spec 分配）
    std::vector<float> tmpbuf_;   ///< 单声源读取缓冲（平面 → 位置直通展开）

    std::atomic<float> level_{0.0f}; ///< 最近窗口 RMS（已用前缀）
};

} // namespace ms
