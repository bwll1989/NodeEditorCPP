// ============================================================================
//  AudioOut.hpp —— PortAudio 真实音频输出（2026-11-11，替代 WASAPI 直连实现）
//
//  实现策略（对照方案决策 1 + 验证阶段范围）：
//    * PortAudio V19.8 静态库（WASAPI host api，MSVC /MD，prebuilt 于
//      3rdparty/portaudio/install）——COM 生命周期由 PA 自理
//      （Pa_Initialize 注册 host api 时 PaWinUtil_CoInitialize，Terminate
//      配对释放）。【根因 2026-11-11】原 WASAPI 直连版在调用线程未初始化
//      COM 时 Activate 直接 CO_E_NOTINITIALIZED → 恒静音回退（当日已补
//      自初始化修复）；换 PA 后该类"输出线程 COM 状态"问题整体消失；
//    * 【推-拉适配】引擎仍是外部拍驱动（echo 33ms mix()）——本类 write()
//      把混合窗推入 PcmRing，PA 回调线程按设备 48k 时钟从环拉取；Qt 定时
//      器与设备时钟的速率差/抖动由环吸收。溢出丢弃、欠载补静音、水位裁齐
//      （P4 漂移纠偏）语义与原实现一一对应；
//    * AUTOCONVERT 语义 = paWinWasapiAutoConvert（设备混音格式非 48k 时
//      Windows 引擎自动重采样——混音域恒 48k 的收口承诺不因设备而破）；
//    * 设备选择：空名 = PA 默认播放设备；非空 = 设备名包含匹配（大小写
//      不敏感；v1 按方案"默认设备 + 按名选择"）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "audio/AudioTypes.hpp"
#include "audio/IAudioOutput.hpp"
#include "audio/PcmRing.hpp"

struct PaStreamCallbackTimeInfo; // 前向声明：thunk 签名须与 PaStreamCallback
                                 // 逐参匹配（timeInfo 为 const 指针）

namespace ms {

class AudioOut final : public IAudioOutput {
public:
    /// \param device_name 空 = 默认播放设备；非空 = 设备名包含匹配
    explicit AudioOut(std::string device_name = "");
    ~AudioOut() override; // close()：回调先停（Pa_CloseStream join）再 Terminate

    bool open(const AudioSpec& spec) override;
    void write(const float* interleaved, std::uint32_t frames) override;
    void close() override;
    void flush() override;
    const char* name() const override {
        return name_.empty() ? "portaudio" : name_.c_str();
    }

    // ---- 诊断观测（P4 漂移纠偏的数据源雏形）----
    std::uint64_t underruns() const noexcept { return underruns_; }
    std::uint64_t overflows() const noexcept { return overflows_; }
    /// 水位裁齐累计丢弃帧数（P4：供给速率 > 设备消费速率时的漂移纠偏量）
    std::uint64_t lateDrops() const noexcept { return late_drops_; }
    const std::string& deviceName() const noexcept { return dev_name_; }

private:
    static int paCallbackTrampoline(const void* input, void* output,
                                    unsigned long frameCount,
                                    const PaStreamCallbackTimeInfo* timeInfo,
                                    unsigned long statusFlags,
                                    void* userData);
    int paCallback(void* output, unsigned long frameCount);

    std::string         dev_req_;   ///< 请求设备名（空 = 默认）
    std::string         dev_name_;  ///< 实际打开的设备名
    std::string         name_;      ///< 实现名（portaudio（WASAPI）等）
    AudioSpec           spec_{};
    std::atomic<bool>   open_{false};

    std::unique_ptr<PcmRing> ring_;      ///< 混音拍 → PA 回调线程交接带
    void*               stream_   = nullptr; ///< PaStream*（避免 pa 头泄入本头）
    bool                pa_inited_ = false;   ///< Pa_Initialize 配对标记
    std::uint32_t       buf_frames_ = 0;      ///< 目标设备缓冲（帧，≈200ms）
    std::uint64_t       underruns_  = 0;      ///< 回调拉取时环欠载次数
    std::uint64_t       overflows_  = 0;      ///< write 推入时环溢出丢帧累计
    std::uint64_t       late_drops_ = 0;      ///< P4 水位裁齐累计丢帧（漂移纠偏）
};

} // namespace ms
