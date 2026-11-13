// ============================================================================
//  IAudioOutput.hpp —— 音频输出抽象（引擎↔输出的彻底解偶缝，2026-11-10）
//
//  设计依据：《audio-output-implementation-plan.md》决策 3 + 用户定案：
//    * AudioEngine 对"声音最终去哪"零感知——只认本接口（write 交错
//      float32 混合样本）；后续无论 WASAPI/文件/NDI 音频再发布/虚拟输出，
//      各自实现 IAudioOutput 接入，引擎零改动；
//    * 本轮验证实现 = NullAudioOutput（丢弃样本，仅供电平链路验证）——
//      用户明确"暂时只做一个验证"，真实输出由后续方案接入；
//    * 线程约定：write 由引擎调用线程串行调用（当前 = 编辑器 UI 拍线程；
//      未来 WASAPI 回调线程接入时，接口天然承载——实现在 write 内自管
//      环形缓冲即可，引擎侧零改动）。
// ============================================================================

#pragma once

#include <cstdint>

#include "audio/AudioTypes.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// IAudioOutput —— 混音输出抽象（解偶缝的"输出"侧）
// ---------------------------------------------------------------------------
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    /// 打开输出流（失败返回 false——引擎保持静默运行，电平链路不受影响）
    virtual bool open(const AudioSpec& spec) = 0;
    /// 写入一段混合样本（交错 float32，frames × spec.channels；阻塞语义
    /// 由实现自定——验证实现恒立即返回）
    virtual void write(const float* interleaved, std::uint32_t frames) = 0;
    /// 关闭输出流（幂等）
    virtual void close() = 0;
    /// 丢弃输出侧未播样本（seek/跳变重对位：环内旧位置音频若不清，
    /// 会按旧内容继续出声 ~1-2 窗——跳转后串音/音画错位的直接来源）。
    /// 默认空实现（无缓冲的实现零成本）。
    virtual void flush() {}
    /// 实现名（诊断/状态栏显示）
    virtual const char* name() const = 0;
};

// ---------------------------------------------------------------------------
// NullAudioOutput —— 验证实现：丢弃样本（无声音卡依赖；单测/CI 可用）
// 统计累计写入帧数供测试断言混音确实发生。
// ---------------------------------------------------------------------------
class NullAudioOutput final : public IAudioOutput {
public:
    bool open(const AudioSpec& spec) override {
        spec_ = spec;
        opened_ = true;
        return true;
    }
    void write(const float* /*interleaved*/, std::uint32_t frames) override {
        written_frames_ += frames;
    }
    void close() override { opened_ = false; }
    const char* name() const override { return "null（验证）"; }

    // ---- 测试观测 ----
    bool          opened() const noexcept { return opened_; }
    std::uint64_t written_frames() const noexcept { return written_frames_; }
    const AudioSpec& spec() const noexcept { return spec_; }

private:
    AudioSpec     spec_{};
    bool          opened_ = false;
    std::uint64_t written_frames_ = 0;
};

} // namespace ms
