//
// Created by TRAE on 2026/09/15.
// 音频波形峰值容器 + 多级下采样金字塔
// 不依赖 Qt Widgets / QObject，可单独序列化缓存
//

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <QtCore/qglobal.h>

#ifdef GUI_ELEMENTS_LIBRARY
#define AUDIO_WAVEFORM_PEAKS_EXPORT Q_DECL_EXPORT
#else
#define AUDIO_WAVEFORM_PEAKS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief 单个峰值级别的数据（单声道：多声道已预先合并为 max-abs 单声道）
 */
struct AUDIO_WAVEFORM_PEAKS_EXPORT AudioWaveformLevel
{
    int bins = 0;                  ///< 该级别包含的峰值块数目
    std::vector<float> mins;       ///< 每个块的最小值（归一化范围 -1~1）
    std::vector<float> maxs;       ///< 每个块的最大值（归一化范围 -1~1）
};

/**
 * @brief 音频波形峰值金字塔
 *
 * 规则：
 * - levels[0] 每 bin 对应 kSamplesPerBin 个采样（最细粒度）
 * - levels[i+1] 每 bin 对应 levels[i] 的 2 个 bin（两两合并取更小的 min / 更大的 max）
 * - 多声道 PCM 在 feed 阶段通过"取所有通道样本绝对值的最大值"合并为单声道峰值
 *   （便于绘制常见的上下对称整体波形，无需区分左右声道条）
 */
class AUDIO_WAVEFORM_PEAKS_EXPORT AudioWaveformPeaks
{
public:
    static constexpr int kSamplesPerBin = 256;
    static constexpr int kMaxLevels = 24;  ///< 金字塔最大层级（足以覆盖约 256 * 2^24 ≈ 40亿采样的超长音频）

    AudioWaveformPeaks() = default;

    /**
     * @brief 开始一段音频的累积，调用后可以多次 feedXXX，最后 endBuild() 生成金字塔
     * @param channels      通道数
     * @param sampleRate    采样率（Hz）
     */
    void beginBuild(int channels, int sampleRate);

    /**
     * @brief 喂入 Float Planar PCM（每通道独立指针，如 FFmpeg FLTP）
     * @param planes              各通道指针数组
     * @param samplesPerChannel   每通道样本数
     */
    void feedPlanar(const float* const* planes, int samplesPerChannel);

    /**
     * @brief 喂入 Float Interleaved PCM（如 L R L R ...）
     * @param interleaved         交织数据指针
     * @param samplesPerChannel   每通道样本数
     * @param channels            通道数
     */
    void feedInterleaved(const float* interleaved, int samplesPerChannel, int channels);

    /**
     * @brief 喂入 S16 Interleaved PCM，会内部归一化到 -1~1 并走 Interleaved 流程
     */
    void feedS16Interleaved(const int16_t* interleaved, int samplesPerChannel, int channels);

    /**
     * @brief 结束累积：根据 levels[0] 构建全部下采样级别
     */
    void endBuild();

    /**
     * @brief 取给定时间/像素宽度下最合适的峰值级别与 bin 范围
     * @param startSec       起始时间（秒）
     * @param endSec         结束时间（秒）
     * @param pixelWidth     需要渲染的像素宽度
     * @param outLevel       [out] 推荐使用的级别下标（levels 索引）
     * @param outStartBin    [out] 该级别起始 bin 索引
     * @param outBinCount    [out] 该级别 bin 数
     * @return  true 数据有效；false 为空数据（未解码或长度为 0）
     */
    bool viewForRange(double startSec, double endSec, int pixelWidth,
                      int* outLevel, int* outStartBin, int* outBinCount) const;

    /**
     * @brief 是否已经完成 endBuild 并有有效数据
     */
    bool isValid() const { return !m_levels.empty() && m_levels[0].bins > 0; }

    /**
     * @brief 清空所有数据（文件切换时调用）
     */
    void reset();

    // === 属性访问 ===
    int channels() const { return m_channels; }
    int sampleRate() const { return m_sampleRate; }
    double totalSeconds() const;
    int64_t totalSamples() const { return m_totalSamples; }
    const std::vector<AudioWaveformLevel>& levels() const { return m_levels; }

private:
    friend class AudioPeakDecoder; // 允许缓存反序列化阶段直接写回私有字段（totalSamples / channels / sampleRate 等）

private:
    /**
     * @brief 累积单个样本到当前 bin（多声道合并后调用）
     */
    void accumulateSample(float sample);

    /**
     * @brief 强制把当前未满的 bin 收尾（endBuild / flush 时调用）
     */
    void flushPendingBin();

private:
    int m_channels = 0;
    int m_sampleRate = 48000;
    int64_t m_totalSamples = 0;

    // 未凑满 kSamplesPerBin 的临时累积
    int m_pendingSamples = 0;
    float m_pendingMin = 0.0f;
    float m_pendingMax = 0.0f;

    std::vector<AudioWaveformLevel> m_levels;
};
