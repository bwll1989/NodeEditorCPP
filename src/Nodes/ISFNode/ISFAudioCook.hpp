/**
 * @file ISFAudioCook.hpp
 * @brief ISF 音频 CPU 侧预处理：PCM → 波形行 / FFT 频谱行（0–1，供上传纹理）
 *
 * ============================================================================
 * ISF 规范（宿主职责）
 * ============================================================================
 * - TYPE "audio"    ：时域波形纹理；一行一声道、一列一采样；0.5 = 静音中心
 * - TYPE "audioFFT" ：频域幅度纹理；一行一声道、一列一频点；通常 0–1
 * - MAX（可选）     ：希望的纹理宽度（采样数 / bin 数）
 *
 * 本文件只做 CPU 打包；上传 GL 与 setBufferForAudioInputKey 在 ISFRenderer 内完成。
 * 一期仅 mono（高度=1）；多声道可后续按行扩展。
 */

#pragma once

#include "AudioTimestampRingQueue.h"

#include "Gist.h"

#include <QVector>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#ifndef NOMINMAX
// 防止 Windows.h 的 min/max 宏干扰 std::min/max（若已被别处定义则无效，下面用括号调用）
#endif

namespace Nodes
{
namespace IsfAudio
{

template <typename T>
inline T clampIndex(T v, T lo, T hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/** 未声明 MAX 时的默认纹理宽度 */
inline constexpr int kDefaultWaveWidth = 512;
inline constexpr int kDefaultFftWidth = 512;
/** Gist 分析窗长（须为 2 的幂） */
inline constexpr int kFftFrameSize = 2048;

/**
 * @brief 从 AudioFrame 提取 mono float32（与 AudioAnalysisWorker 约定一致）
 * - 1ch：原样
 * - 2ch：左右平均
 * - 多 ch：取第 0 声道
 */
inline std::vector<float> extractMonoSamples(const AudioFrame& frame)
{
    std::vector<float> out;
    if (frame.data.isEmpty() || frame.bitsPerSample != 32) {
        // 工程音频通路事实标准为 float32；其它格式一期忽略
        if (frame.data.isEmpty()) {
            return out;
        }
    }

    const float* samples = reinterpret_cast<const float*>(frame.data.constData());
    const int totalSamples = frame.data.size() / static_cast<int>(sizeof(float));
    if (totalSamples <= 0 || frame.channels <= 0) {
        return out;
    }

    const int samplesPerChannel = totalSamples / frame.channels;
    out.reserve(static_cast<size_t>(samplesPerChannel));

    if (frame.channels == 1) {
        out.assign(samples, samples + samplesPerChannel);
    } else if (frame.channels == 2) {
        for (int i = 0; i < samplesPerChannel; ++i) {
            out.push_back((samples[i * 2] + samples[i * 2 + 1]) * 0.5f);
        }
    } else {
        for (int i = 0; i < samplesPerChannel; ++i) {
            out.push_back(samples[i * frame.channels]);
        }
    }
    return out;
}

/** PCM [-1,1] → ISF 波形像素 [0,1]，0.5 为中心 */
inline float pcmToWavePixel(float sample)
{
    const float clamped = std::max(-1.0f, std::min(1.0f, sample));
    return clamped * 0.5f + 0.5f;
}

/**
 * @brief 滑动缓冲：持续追加 PCM，保留最近 maxKeep 个采样
 * 用于波形 MAX 宽于单 tick、以及 FFT 凑满窗长。
 */
struct SampleRing
{
    std::vector<float> samples;
    size_t maxKeep = 8192;

    void append(const std::vector<float>& chunk)
    {
        if (chunk.empty()) {
            return;
        }
        samples.insert(samples.end(), chunk.begin(), chunk.end());
        if (samples.size() > maxKeep) {
            samples.erase(samples.begin(),
                          samples.begin() + static_cast<std::ptrdiff_t>(samples.size() - maxKeep));
        }
    }

    void clear() { samples.clear(); }
};

/**
 * @brief 取最近 width 个采样打成波形行；不足则左侧静音（0.5）填充
 */
inline QVector<float> cookWaveformRow(const SampleRing& ring, int width)
{
    const int w = std::max(1, width);
    QVector<float> row(w, 0.5f);
    if (ring.samples.empty()) {
        return row;
    }

    const int available = static_cast<int>(ring.samples.size());
    if (available >= w) {
        const size_t start = ring.samples.size() - static_cast<size_t>(w);
        for (int i = 0; i < w; ++i) {
            row[i] = pcmToWavePixel(ring.samples[start + static_cast<size_t>(i)]);
        }
        return row;
    }

    // 样本不足：右对齐，左侧 0.5
    const int offset = w - available;
    for (int i = 0; i < available; ++i) {
        row[offset + i] = pcmToWavePixel(ring.samples[static_cast<size_t>(i)]);
    }
    return row;
}

/**
 * @brief 单路 audioFFT 的 Gist 状态（按 ATTR name 各持一份）
 */
struct FftCooker
{
    std::unique_ptr<Gist<float>> gist;
    SampleRing ring;
    int frameSize = kFftFrameSize;
    int sampleRate = 48000;
    /** 上一帧成功算出的频谱（不够窗长时复用，避免闪烁） */
    QVector<float> lastBins;

    void ensureGist(int sr)
    {
        if (sr > 0 && sr != sampleRate) {
            sampleRate = sr;
            gist.reset();
        }
        if (!gist) {
            gist = std::make_unique<Gist<float>>(frameSize, sampleRate);
            ring.maxKeep = static_cast<size_t>(frameSize) * 4;
        }
    }

    void pushMono(const std::vector<float>& mono, int sr)
    {
        ensureGist(sr > 0 ? sr : sampleRate);
        ring.append(mono);
    }

    /**
     * @brief 尽量消费满窗做 FFT，输出归一化到 0–1、长度为 outWidth 的行
     * 若本帧样本仍不够一个窗，返回 lastBins（可能为空→调用方填静音谱）
     */
    QVector<float> cookSpectrumRow(int outWidth)
    {
        const int w = std::max(1, outWidth);
        ensureGist(sampleRate);

        while (ring.samples.size() >= static_cast<size_t>(frameSize)) {
            std::vector<float> frame(ring.samples.begin(),
                                     ring.samples.begin() + frameSize);
            ring.samples.erase(ring.samples.begin(),
                               ring.samples.begin() + frameSize);

            gist->processAudioFrame(frame);
            const std::vector<float>& mag = gist->getMagnitudeSpectrum();
            if (mag.empty()) {
                continue;
            }

            // 峰值归一化到 0–1（简单稳健；后续可改为 dB 映射）
            float peak = 1.0e-6f;
            for (float v : mag) {
                peak = std::max(peak, v);
            }

            QVector<float> full;
            full.reserve(static_cast<int>(mag.size()));
            for (float v : mag) {
                full.push_back(std::max(0.0f, std::min(1.0f, v / peak)));
            }

            // 缩放到 ISF MAX 宽度（线性采样）
            QVector<float> scaled(w, 0.0f);
            if (full.size() == 1) {
                scaled.fill(full[0]);
            } else {
                for (int i = 0; i < w; ++i) {
                    const float t = (w == 1) ? 0.f
                                             : static_cast<float>(i) / static_cast<float>(w - 1);
                    const float src = t * static_cast<float>(full.size() - 1);
                    const int i0 = static_cast<int>(src);
                    const int last = static_cast<int>(full.size()) - 1;
                    const int i1 = clampIndex(i0 + 1, 0, last);
                    const float frac = src - static_cast<float>(i0);
                    scaled[i] = full[i0] * (1.f - frac) + full[i1] * frac;
                }
            }
            lastBins = scaled;
        }

        if (!lastBins.isEmpty()) {
            if (lastBins.size() == w) {
                return lastBins;
            }
            // 宽度因 Reload/换 MAX 变化时重采样上一帧
            QVector<float> scaled(w, 0.0f);
            for (int i = 0; i < w; ++i) {
                const float t = (w == 1) ? 0.f
                                         : static_cast<float>(i) / static_cast<float>(w - 1);
                const float src = t * static_cast<float>(lastBins.size() - 1);
                const int i0 = static_cast<int>(src);
                const int last = lastBins.size() - 1;
                const int i1 = clampIndex(i0 + 1, 0, last);
                const float frac = src - static_cast<float>(i0);
                scaled[i] = lastBins[i0] * (1.f - frac) + lastBins[i1] * frac;
            }
            lastBins = scaled;
            return lastBins;
        }

        return QVector<float>(w, 0.0f);
    }

    void reset()
    {
        ring.clear();
        lastBins.clear();
        gist.reset();
    }
};

/** 上传给 Renderer 的一行音频纹理 CPU 数据 */
struct AudioRowPayload
{
    QString name;       ///< ATTR NAME
    QVector<float> row; ///< 宽度 = row.size()，值域 0–1
};

inline QVector<float> silenceWave(int width)
{
    return QVector<float>(std::max(1, width), 0.5f);
}

inline QVector<float> silenceSpectrum(int width)
{
    return QVector<float>(std::max(1, width), 0.0f);
}

} // namespace IsfAudio
} // namespace Nodes
