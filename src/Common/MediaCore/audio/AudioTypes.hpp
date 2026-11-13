// ============================================================================
//  AudioTypes.hpp —— 音频域基础类型（流规格 + 声源接口 + 池条目）
//
//  设计依据：《audio-output-implementation-plan.md》决策 2（2026-11-10）：
//    * PCM 域统一 float32 / 48kHz；通道 layout 保持源声明原样直传
//     （通道位置语义 = 标准位置位掩码 AV_CH_*，混合时按位并集直通——
//      不做系数下混，通道映射恒不变）；
//    * 生产侧交付走 PcmRing（交错 float32 直推，零中间快照）——跨线程
//      交接带类型由 PcmRing 承担，本头不再定义帧快照结构。
// ============================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ms {

/// 声源侧最大声道数（5.1/7.1 富余）。【声道契约（[S3] 2026-11-11）】
/// IAudioSource 实现的 channelCount() 不得超本值——AudioEngine 按此上限
/// 配单源读取缓冲（超限声明按契约截断 + 选配层告警，绝非静默越界）。
/// 现有实现均在构造期自钳（AudioMediaSource min(8)/Sine 恒 1）；新音源
///（Live NDI 音频等）接入时须遵守同一契约。
inline constexpr std::uint32_t kMaxSourceChannels = 8;

// ---------------------------------------------------------------------------
// 音频流规格（混音域：float32 交错 / 采样率 / 通道数）
// ---------------------------------------------------------------------------
struct AudioSpec {
    int         sample_rate = 48'000; ///< 采样率 Hz（混音域统一 48k）
    int         channels    = 2;      ///< 混音输出通道数（验证期立体声）
    std::uint32_t frames_per_tick = 1'600; ///< 每拍样本数（48k 下 ≈33ms，
                                          ///<  与编辑器 echo 拍同节奏）
};

// ---------------------------------------------------------------------------
// IAudioSource —— 音频段生产者接口（AudioEngine 混音输入；解偶缝的"源"侧）
//
// 实现族：SineAudioSource（正弦回退）/ AudioMediaSource（素材解码）——
// 新音源（Live NDI 音频等）实现本接口接入，混音器与选配层零改动。
// ---------------------------------------------------------------------------
class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    /// 源声道数（位置直通映射的依据：1=单声道→FL/FR 复制；≥2=按位直通，
    /// 源不足的输出通道保持静默，多余源通道丢弃——绝不系数折叠）。
    /// 【契约上限】返回值不得超 kMaxSourceChannels(8)——实现须在构造期
    /// 自钳；超限声明 = 契约违约：AudioEngine 按上限截断配读取缓冲（防
    /// 堆越界），选配层（AudioCasting::buildVoices）启动期告警
    virtual std::uint32_t channelCount() const noexcept = 0;
    /// 读取一段 PCM（交错 float32，按源声道布局写：帧 f 声道 c =
    /// dst[f * channelCount() + c]；源结束/静音可 < frames，剩余区段由
    /// 调用方补零）。
    /// \param media_us 素材内部时间（调用方按 TimelineEngine 口径换算）
    /// \return 实际写出的帧数（源结束/静音可 < frames，剩余区段由调用方补零）
    virtual std::uint32_t read(float* dst, std::int64_t media_us,
                               std::uint32_t frames) = 0;
};

// ---------------------------------------------------------------------------
// AudioPoolEntry —— 音源池条目（编辑器/RenderThread 音源池共用值类型）
//
// 【池条目指纹铁律】池键 = clip id，但源内容取决于素材路径——片段换绑
// 素材（同 clip id 指向新 asset）时旧源不得命中：条目记录构建时的素材
// 路径，不匹配即重建（否则声音错素材）。
// ---------------------------------------------------------------------------
struct AudioPoolEntry {
    std::string                   asset_path; ///< 构建源时的素材路径（空=正弦回退）
    std::unique_ptr<IAudioSource> source;
};

} // namespace ms
