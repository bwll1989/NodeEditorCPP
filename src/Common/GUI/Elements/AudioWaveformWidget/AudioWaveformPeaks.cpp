//
// Created by TRAE on 2026/09/15.
// 音频波形峰值金字塔实现
//

#include "AudioWaveformPeaks.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

void AudioWaveformPeaks::reset()
{
    m_channels = 0;
    m_sampleRate = 48000;
    m_totalSamples = 0;
    m_pendingSamples = 0;
    m_pendingMin = 0.0f;
    m_pendingMax = 0.0f;
    m_levels.clear();
}

void AudioWaveformPeaks::beginBuild(int channels, int sampleRate)
{
    reset();
    m_channels = std::max(1, channels);
    m_sampleRate = sampleRate > 0 ? sampleRate : 48000;
    // levels[0] 预分配一个空结构，feed 时 push_back 到其中
    AudioWaveformLevel l0;
    l0.bins = 0;
    m_levels.push_back(std::move(l0));
}

void AudioWaveformPeaks::accumulateSample(float sample)
{
    // 合并多声道取 max 的前置保证：sample 本身已是 max(abs(channels)) 带符号
    // 这里的 sample 可以是 -1~1 之间任意值（feed 阶段会做 clipping 保护）
    if (sample > 1.0f) sample = 1.0f;
    else if (sample < -1.0f) sample = -1.0f;

    if (m_pendingSamples == 0) {
        m_pendingMin = sample;
        m_pendingMax = sample;
        m_pendingSamples = 1;
        return;
    }
    if (sample < m_pendingMin) m_pendingMin = sample;
    if (sample > m_pendingMax) m_pendingMax = sample;
    ++m_pendingSamples;

    if (m_pendingSamples >= kSamplesPerBin) {
        // 满一 bin 写入 level0
        auto& l0 = m_levels[0];
        l0.mins.push_back(m_pendingMin);
        l0.maxs.push_back(m_pendingMax);
        ++l0.bins;
        m_pendingSamples = 0;
        m_pendingMin = 0.0f;
        m_pendingMax = 0.0f;
    }
}

void AudioWaveformPeaks::flushPendingBin()
{
    if (m_pendingSamples <= 0) return;
    auto& l0 = m_levels[0];
    l0.mins.push_back(m_pendingMin);
    l0.maxs.push_back(m_pendingMax);
    ++l0.bins;
    m_pendingSamples = 0;
    m_pendingMin = 0.0f;
    m_pendingMax = 0.0f;
}

void AudioWaveformPeaks::feedPlanar(const float* const* planes, int samplesPerChannel)
{
    if (samplesPerChannel <= 0 || m_channels <= 0) return;
    const int ch = m_channels;
    for (int i = 0; i < samplesPerChannel; ++i) {
        float maxAbs = 0.0f;
        float signedVal = 0.0f;
        for (int c = 0; c < ch; ++c) {
            const float v = planes[c][i];
            const float a = std::fabs(v);
            if (a > maxAbs) {
                maxAbs = a;
                signedVal = v;
            }
        }
        (void)maxAbs; // signedVal 已保持原符号
        accumulateSample(signedVal);
    }
    m_totalSamples += samplesPerChannel;
}

void AudioWaveformPeaks::feedInterleaved(const float* interleaved, int samplesPerChannel, int channels)
{
    if (samplesPerChannel <= 0 || channels <= 0) return;
    for (int i = 0; i < samplesPerChannel; ++i) {
        float maxAbs = 0.0f;
        float signedVal = 0.0f;
        const float* base = interleaved + static_cast<std::ptrdiff_t>(i) * channels;
        for (int c = 0; c < channels; ++c) {
            const float v = base[c];
            const float a = std::fabs(v);
            if (a > maxAbs) {
                maxAbs = a;
                signedVal = v;
            }
        }
        accumulateSample(signedVal);
    }
    m_totalSamples += samplesPerChannel;
}

void AudioWaveformPeaks::feedS16Interleaved(const int16_t* interleaved, int samplesPerChannel, int channels)
{
    if (samplesPerChannel <= 0 || channels <= 0) return;
    constexpr float kInv = 1.0f / 32768.0f;
    for (int i = 0; i < samplesPerChannel; ++i) {
        float maxAbs = 0.0f;
        float signedVal = 0.0f;
        const int16_t* base = interleaved + static_cast<std::ptrdiff_t>(i) * channels;
        for (int c = 0; c < channels; ++c) {
            const int16_t v = base[c];
            const float fv = static_cast<float>(v) * kInv;
            const float a = std::fabs(fv);
            if (a > maxAbs) {
                maxAbs = a;
                signedVal = fv;
            }
        }
        accumulateSample(signedVal);
    }
    m_totalSamples += samplesPerChannel;
}

void AudioWaveformPeaks::endBuild()
{
    if (m_levels.empty()) return;

    // 把未满的 samples 收尾为最后一个 bin
    flushPendingBin();

    // 构建后续级别，直到 bins <= 1 或达到 kMaxLevels
    while (static_cast<int>(m_levels.size()) < kMaxLevels) {
        const auto& prev = m_levels.back();
        if (prev.bins <= 1) break;
        AudioWaveformLevel next;
        next.bins = (prev.bins + 1) / 2;  // 向下取整对齐：奇数 bin 把最后一个单独合并
        next.mins.resize(next.bins);
        next.maxs.resize(next.bins);
        for (int i = 0; i < next.bins; ++i) {
            const int a = i * 2;
            const int b = a + 1;
            const float minA = prev.mins[a];
            const float maxA = prev.maxs[a];
            if (b < prev.bins) {
                const float minB = prev.mins[b];
                const float maxB = prev.maxs[b];
                next.mins[i] = std::min(minA, minB);
                next.maxs[i] = std::max(maxA, maxB);
            } else {
                next.mins[i] = minA;
                next.maxs[i] = maxA;
            }
        }
        m_levels.push_back(std::move(next));
    }
}

double AudioWaveformPeaks::totalSeconds() const
{
    if (m_sampleRate <= 0) return 0.0;
    return static_cast<double>(m_totalSamples) / static_cast<double>(m_sampleRate);
}

bool AudioWaveformPeaks::viewForRange(double startSec, double endSec, int pixelWidth,
                                      int* outLevel, int* outStartBin, int* outBinCount) const
{
    if (!isValid() || pixelWidth <= 0) return false;
    const double total = totalSeconds();
    if (total <= 0.0) return false;

    double s = startSec;
    double e = endSec;
    if (s > e) std::swap(s, e);
    if (s < 0.0) s = 0.0;
    if (e > total) e = total;
    if (e - s <= 1e-9) {
        e = s + 1e-3;
        if (e > total) e = total;
    }
    const double visible = e - s;

    // 期望：每个 pixel 对应若干 bin，但 bin 数量 >= pixelWidth 且尽量接近（避免过多采样）
    // 先反推 level0 需要多少 bin
    const double samplesPerPixel = visible * static_cast<double>(m_sampleRate) / static_cast<double>(pixelWidth);
    const double binsPP_L0 = samplesPerPixel / static_cast<double>(kSamplesPerBin);  // L0 bin 数 / 像素
    // 目标 level：binsPP 应在 1~2 之间（每一像素最多用 2 个 bin 合并，最小1:1）
    // level=0: binsPP = binsPP_L0
    // level=L: binsPP = binsPP_L0 / 2^L
    // 找最小 L 使 binsPP_L0 / 2^L <= 2
    int L = 0;
    double binsPP = binsPP_L0;
    const int maxLvl = static_cast<int>(m_levels.size()) - 1;
    while (L < maxLvl && binsPP > 2.0) {
        binsPP *= 0.5;
        ++L;
    }
    // 如果 binsPP 仍 < 1（缩得太近，L=0 也不够细），那就停留在 L=0
    if (L > maxLvl) L = maxLvl;

    const auto& lv = m_levels[L];
    const double binsInLevelTotal = static_cast<double>(lv.bins);
    const double scale = total > 0.0 ? binsInLevelTotal / total : 0.0;
    const int startBin = std::max(0, static_cast<int>(std::floor(s * scale)));
    int endBin = std::min(lv.bins - 1, static_cast<int>(std::ceil(e * scale)));
    if (endBin < startBin) endBin = startBin;
    const int binCount = endBin - startBin + 1;

    *outLevel = L;
    *outStartBin = startBin;
    *outBinCount = binCount;
    return true;
}
