// ============================================================================
//  AudioOut.cpp —— PortAudio 输出实现（WASAPI host api + 回调拉环）
//
//  【2026-11-11】由 WASAPI 直连实现改写为 PortAudio 后端：
//    * COM：Pa_Initialize 内部 PaWinUtil_CoInitialize（本类零 COM 代码）；
//    * 设备枚举/按名选择：Pa_GetDeviceCount + 设备名包含匹配；
//    * AUTOCONVERTPCM 语义：paWinWasapiAutoConvert（48k 混音域收口）；
//    * MMCSS 线程特征：paWinWasapiThreadPriority；
//    * 推-拉语义不变：write → PcmRing → PA 回调 pop（欠载静音/溢出丢弃/
//      ringTrimToLevel 水位裁齐全保留）。
// ============================================================================

#include "audio/AudioOut.hpp"

#include <portaudio.h>
#include <pa_win_wasapi.h>

#include <cstdio>
#include <cstring>

namespace ms {

namespace {
// 目标设备缓冲时长（秒）：设备侧延迟直接计入 A/V 同步的固有滞后
//（画面即绘即显，声音晚出 = 永久性不同步分量）——从初版 0.2 收紧到
// 0.1；供给侧漂移已由会话锁定可变窗根治（editor audioTick 同口径），
// 环水位目标 60ms 足以吸收 Qt 拍抖动
constexpr double kTargetLatencySec = 0.1;

std::string toLower(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}
} // namespace

AudioOut::AudioOut(std::string device_name)
    : dev_req_(std::move(device_name)) {}

AudioOut::~AudioOut() { close(); }

// ---------------------------------------------------------------------------
// open：Pa_Initialize → 设备选择 → Pa_OpenStream（float32/回调）→ Start。
// 任何一步失败：释放半途资源并返回 false（引擎回退 Null 输出）。
// ---------------------------------------------------------------------------
bool AudioOut::open(const AudioSpec& spec) {
    close(); // 幂等重开（引擎 attach 运行中替换语义）

    spec_ = spec;

    const PaError err0 = Pa_Initialize();
    if (err0 != paNoError) {
        std::fprintf(stderr, "[audio] open 失败 @Pa_Initialize: %s\n",
                     Pa_GetErrorText(err0));
        return false;
    }
    pa_inited_ = true;

    // ---- 设备选择（空名 = 默认；非空 = 名字包含匹配，大小写不敏感）----
    PaDeviceIndex dev = paNoDevice;
    if (dev_req_.empty()) {
        dev = Pa_GetDefaultOutputDevice();
    } else {
        const std::string req = toLower(dev_req_);
        const int n = Pa_GetDeviceCount();
        for (int i = 0; i < n; ++i) {
            const PaDeviceInfo* di = Pa_GetDeviceInfo(i);
            if (!di || !di->name) continue;
            const std::string low = toLower(di->name);
            if (!low.empty() && low.find(req) != std::string::npos) {
                dev = i;
                break;
            }
        }
        if (dev == paNoDevice) {
            std::fprintf(stderr,
                         "[audio] W-未找到包含 \"%s\" 的播放设备，回退默认设备\n",
                         dev_req_.c_str());
            dev = Pa_GetDefaultOutputDevice();
        }
    }
    const PaDeviceInfo* di = (dev != paNoDevice) ? Pa_GetDeviceInfo(dev)
                                                 : nullptr;
    if (!di) {
        std::fprintf(stderr, "[audio] open 失败 @设备枚举（无可用播放设备）\n");
        Pa_Terminate();
        pa_inited_ = false;
        return false;
    }
    dev_name_ = di->name;

    // ---- 流参数：float32 / spec 采样率 / spec 通道 ----
    PaStreamParameters sp{};
    sp.device             = dev;
    sp.channelCount       = static_cast<int>(spec.channels);
    sp.sampleFormat       = paFloat32;
    sp.suggestedLatency   = kTargetLatencySec;
    sp.hostApiSpecificStreamInfo = nullptr;

    // WASAPI 专属：AutoConvert（设备混音格式非 48k 时 Windows 引擎自动
    // 重采样——混音域恒 48k 的收口承诺）+ MMCSS 线程特征（抗抖动）
    PaWasapiStreamInfo wsi{};
    const PaHostApiInfo* hi = Pa_GetHostApiInfo(di->hostApi);
    if (hi && hi->type == paWASAPI) {
        wsi.size        = sizeof(wsi);
        wsi.hostApiType = paWASAPI;
        wsi.version     = 1;
        wsi.flags = paWinWasapiAutoConvert | paWinWasapiThreadPriority;
        sp.hostApiSpecificStreamInfo = &wsi;
    }

    // 环容量：设备缓冲 + 4 个混音窗（节拍抖动 + 速率差缓震）
    // 水位：高 120ms / 目标 60ms（供给速率已会话锁定，稳态差仅 ppm 级；
    // 高水位只在持续漂移时兑底裁齐，稳态延迟 ≈ 60ms + 设备缓冲）
    buf_frames_ = spec.sample_rate * 3 / 25; // 120ms（水位裁齐的高水位）
    ring_ = std::make_unique<PcmRing>(
        static_cast<std::uint32_t>(spec.channels),
        static_cast<std::uint64_t>(buf_frames_) + spec.frames_per_tick * 4);

    const PaError err1 =
        Pa_OpenStream(&stream_, nullptr, &sp, spec.sample_rate,
                      paFramesPerBufferUnspecified, paNoFlag,
                      &AudioOut::paCallbackTrampoline, this);
    if (err1 != paNoError) {
        std::fprintf(stderr,
                     "[audio] open 失败 @Pa_OpenStream"
                     "（fmt=float32 %dch %dHz dev=%s）: %s\n",
                     sp.channelCount, static_cast<int>(spec.sample_rate),
                     dev_name_.c_str(), Pa_GetErrorText(err1));
        stream_ = nullptr;
        Pa_Terminate();
        pa_inited_ = false;
        return false;
    }
    const PaError err2 = Pa_StartStream(stream_);
    if (err2 != paNoError) {
        std::fprintf(stderr, "[audio] open 失败 @Pa_StartStream: %s\n",
                     Pa_GetErrorText(err2));
        Pa_CloseStream(stream_);
        stream_ = nullptr;
        Pa_Terminate();
        pa_inited_ = false;
        return false;
    }

    underruns_ = overflows_ = late_drops_ = 0;
    open_ = true;
    name_ = "portaudio（";
    if (hi) name_ += hi->name;
    name_ += "）";
    return true;
}

// ---------------------------------------------------------------------------
// write：混音窗推入环（非阻塞——满则丢弃余量计溢出，绝不阻塞 UI 拍）。
// P4 水位纠偏：推入前裁齐——供给速率（会话时钟锁定产出）高于设备消费
// 速率时环水位单调缓涨，超过高水位一次性裁回目标水位；丢量入 late_drops_。
// ---------------------------------------------------------------------------
void AudioOut::write(const float* interleaved, std::uint32_t frames) {
    if (!open_ || !ring_) return;
    // 高水位 = 120ms；目标 = 60ms。阈值带 60ms：短时抖动不触发（环吸收），
    // 仅持续漂移被裁齐（会话锁定供给下正常播放永不触发）。
    const std::uint64_t high   = buf_frames_;
    const std::uint64_t target = buf_frames_ / 2;
    late_drops_ += ringTrimToLevel(*ring_, target, high);
    const auto pushed =
        ring_->tryPush(interleaved, static_cast<std::uint64_t>(frames));
    overflows_ += static_cast<std::uint64_t>(frames) - pushed;
}

// ---------------------------------------------------------------------------
// PA 回调（PA 设备线程）：从环拉取，欠载补静音并计数。
// 线程体纪律：永不阻塞、永不抛出。close() 先 Stop/Close（join 回调）
// 再返回——回调触碰 ring_ 的窗口被 PA 保证收敛在本函数内。
// ---------------------------------------------------------------------------
int AudioOut::paCallbackTrampoline(const void* /*input*/, void* output,
                                   unsigned long frameCount,
                                   const PaStreamCallbackTimeInfo* /*timeInfo*/,
                                   unsigned long /*statusFlags*/,
                                   void* userData) {
    return static_cast<AudioOut*>(userData)->paCallback(output, frameCount);
}

int AudioOut::paCallback(void* output, unsigned long frameCount) {
    auto* out = static_cast<float*>(output);
    const auto want = static_cast<std::uint64_t>(frameCount);
    const auto got  = ring_ ? ring_->pop(out, want) : 0;
    if (got < want) {
        std::memset(out + static_cast<std::size_t>(got) * spec_.channels, 0,
                    static_cast<std::size_t>(want - got) * spec_.channels *
                        sizeof(float));
        ++underruns_; // 欠载：静音填缝（P4 漂移纠偏观测点）
    }
    return open_.load(std::memory_order_relaxed) ? paContinue : paComplete;
}

void AudioOut::flush() {
    // seek/跳变重对位：环内旧位置样本一次清空（PA 回调 pop 持同一把锁，
    // 无竞态；清后回调拉到空环 = 静音，直到新位置混音窗写入）
    if (ring_) ring_->clear();
}

// ---------------------------------------------------------------------------
// close：Stop/Close（PA join 回调线程）→ Pa_Terminate（配对释放 COM）。
// 幂等。
// ---------------------------------------------------------------------------
void AudioOut::close() {
    open_.store(false, std::memory_order_relaxed);
    if (stream_) {
        Pa_StopStream(stream_);  // 停止回调（等待在途回调收敛）
        Pa_CloseStream(stream_); // join 回调线程 + 释放流
        stream_ = nullptr;
    }
    if (pa_inited_) {
        Pa_Terminate();
        pa_inited_ = false;
    }
    // ring 不在此释放：write() 检查 open_ 与 tryPush 之间无锁，若 close
    // 并发释放 ring = 在途 write 悬垂（防御性——当前调用序 stop 均在
    // 生产者静默后，此处兜底）。ring 由析构或下次 open() 重开时回收。
}

} // namespace ms
