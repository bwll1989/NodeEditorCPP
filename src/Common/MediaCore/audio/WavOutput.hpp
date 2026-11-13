// ============================================================================
//  WavOutput.hpp —— WAV 文件输出（IAudioOutput 实现，2026-11-10 诊断/验证用）
//
//  用途：音频输出解偶缝的第二个实现（Null 之外）——把混音结果落成标准
//  16-bit PCM WAV，供"音频输出"链路实证（可听/可波形分析/可 ffmpeg 验证）。
//  非实时设备输出：write 追加缓冲、close 时补齐 RIFF 头（长度后知后写）。
//  路径延迟打开：start 时引擎 open(spec) 传入混音域——文件名由构造给出。
// ============================================================================

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "audio/AudioTypes.hpp"
#include "audio/IAudioOutput.hpp"

namespace ms {

class WavOutput final : public IAudioOutput {
public:
    explicit WavOutput(std::string path) : path_(std::move(path)) {}
    ~WavOutput() override { close(); }

    bool open(const AudioSpec& spec) override {
        spec_  = spec;
        file_  = std::fopen(path_.c_str(), "wb");
        if (!file_) return false;
        // 预留 44 字节 RIFF 头（close 时按实际长度回填）
        std::vector<unsigned char> header(44, 0);
        std::fwrite(header.data(), 1, header.size(), file_);
        frames_ = 0;
        return true;
    }

    void write(const float* interleaved, std::uint32_t frames) override {
        if (!file_) return;
        // float32 [-1,1] → int16（削波钳制；超出 ±1 的混合和按满幅记）。
        // 先聚合到缓冲再一次 fwrite——逐样本 fwrite 是每拍 6400 次系统
        // 调用级 I/O（8s 生成 ≈122 万次），聚合后每拍 1 次
        pcm_.resize(static_cast<std::size_t>(frames) * spec_.channels);
        for (std::size_t i = 0; i < pcm_.size(); ++i) {
            float s = interleaved[i];
            if (s >  1.0f) s =  1.0f;
            if (s < -1.0f) s = -1.0f;
            pcm_[i] = static_cast<std::int16_t>(s * 32767.0f);
        }
        std::fwrite(pcm_.data(), sizeof(std::int16_t), pcm_.size(), file_);
        frames_ += frames;
    }

    void close() override {
        if (!file_) return;
        // RIFF 头回填（44.1k/48k 均为标准 PCM 头；尺寸按实际样本数）
        const std::uint32_t data_bytes =
            static_cast<std::uint32_t>(frames_) * spec_.channels * 2;
        unsigned char h[44] = {};
        auto put4 = [&](std::size_t off, const char* s) {
            for (int i = 0; i < 4; ++i)
                h[off + i] = static_cast<unsigned char>(s[i]);
        };
        auto put32 = [&](std::size_t off, std::uint32_t v) {
            h[off] = v & 0xFF; h[off+1] = (v >> 8) & 0xFF;
            h[off+2] = (v >> 16) & 0xFF; h[off+3] = (v >> 24) & 0xFF;
        };
        auto put16 = [&](std::size_t off, std::uint16_t v) {
            h[off] = v & 0xFF; h[off+1] = (v >> 8) & 0xFF;
        };
        put4(0, "RIFF");
        put32(4, 36 + data_bytes);
        put4(8, "WAVE");
        put4(12, "fmt ");
        put32(16, 16);                                   // PCM fmt 块长
        put16(20, 1);                                    // PCM
        put16(22, static_cast<std::uint16_t>(spec_.channels));
        put32(24, static_cast<std::uint32_t>(spec_.sample_rate));
        put32(28, static_cast<std::uint32_t>(spec_.sample_rate) *
                  spec_.channels * 2);                   // byte rate
        put16(32, static_cast<std::uint16_t>(spec_.channels * 2)); // block align
        put16(34, 16);                                   // bits per sample
        put4(36, "data");
        put32(40, data_bytes);
        std::fseek(file_, 0, SEEK_SET);
        std::fwrite(h, 1, sizeof(h), file_);
        std::fclose(file_);
        file_ = nullptr;
    }

    const char* name() const override { return "wav（落盘验证）"; }

    // ---- 测试观测 ----
    std::uint64_t frames_written() const noexcept { return frames_; }

private:
    std::string                path_;
    AudioSpec                  spec_{};
    std::FILE*                 file_  = nullptr;
    std::uint64_t              frames_ = 0;
    std::vector<std::int16_t>  pcm_;   ///< write 聚合缓冲（每拍一次 fwrite）
};

} // namespace ms
