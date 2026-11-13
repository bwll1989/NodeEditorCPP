// ============================================================================
//  AudioMediaSource.cpp —— 素材音频解码源实现（FFmpeg demux→decode→swr→ring）
// ============================================================================

#include "audio/AudioMediaSource.hpp"

#include "runtime/AssetPath.hpp" // 素材归集寻址（2026-11-13）

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <system_error>
#include <thread>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/rational.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
}

namespace ms {

namespace {
constexpr std::uint32_t kMaxChannels = 8;      // 与 AudioEngine 同钳制
constexpr std::int64_t kBackwardTolSamples = 2048; // ~42ms 回退容差
constexpr std::uint32_t kPrefetchMs = 400;     // 解码前置量（环容量依据）
constexpr AVRational kSampleTb{1, 48'000};     // 混音域时间基
constexpr std::uint32_t kMixRate = 48'000;     // 混音域采样率（环内数据域）

/// 当前单调时钟毫秒（读侧等待期限用）
std::int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
} // namespace

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------
AudioMediaSource::AudioMediaSource(std::string assets_root, std::string path)
    : assets_root_(std::move(assets_root)), path_(std::move(path)) {
    if (openInput()) {
        // 环容量 = 前置量 + 富余一窗（约 400ms@48k 立体声 ≈ 1.5MB，可控）
        // 环容量按混音域算（环内数据已经 swr 统一到 48k；若按源采样率算，
        // 44.1k 素材容量少 8%，无碍——但读侧 target 域必须同为本域）
        ring_ = std::make_unique<PcmRing>(
            channels_,
            static_cast<std::uint64_t>(kMixRate) * kPrefetchMs / 1000 +
                2'048);
        startThread();
        opened_.store(true, std::memory_order_relaxed);
    }
}

AudioMediaSource::~AudioMediaSource() { release(); }

// ---------------------------------------------------------------------------
// openInput：容器 → 音频流 → 解码器 → swr（float32/48k/源通道数直传）
// 任一步失败：释放半途句柄并保持 opened_=false（调用方回退正弦源）
// ---------------------------------------------------------------------------
bool AudioMediaSource::openInput() {
    // ---- 1. 路径拼接（2026-11-13 归集寻址：FfmpegMediaSource 同款收敛）----
    // assetResolvePath = 归集副本（assets_root/文件名）优先，缺失回退原
    // 绝对路径；相对路径拼 assets_root（.show 语义）。原"盘符/UNC 直通"
    // 裸分支退役（四处手写判定收敛 runtime/AssetPath.hpp 单一真相）。
    const std::string full = ms::assetResolvePath(assets_root_, path_);
    const std::filesystem::path fsp(
        reinterpret_cast<const char8_t*>(full.c_str()));
    std::error_code ec;
    if (!std::filesystem::exists(fsp, ec) || ec) return false;

    // ---- 2. 容器 + 流探测 ----
    if (avformat_open_input(&fmt_, full.c_str(), nullptr, nullptr) < 0)
        return false;
    if (avformat_find_stream_info(fmt_, nullptr) < 0) return release(), false;

    // ---- 3. 最优音频流（无 → 本源无声，调用方回退）----
    const AVCodec* dec = nullptr;
    stream_idx_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_AUDIO, -1, -1,
                                      &dec, 0);
    if (stream_idx_ < 0 || !dec) return release(), false;
    AVStream* st = fmt_->streams[stream_idx_];
    tb_num_ = st->time_base.num;
    tb_den_ = st->time_base.den;

    codec_ = avcodec_alloc_context3(dec);
    if (!codec_ || avcodec_parameters_to_context(codec_, st->codecpar) < 0)
        return release(), false;
    if (avcodec_open2(codec_, dec, nullptr) < 0) return release(), false;

    // ---- 4. 混音域规格：源通道数直传（钳 8）、统一 48k float32 交错 ----
    channels_ = static_cast<std::uint32_t>(
        std::min<std::int64_t>(codec_->ch_layout.nb_channels, kMaxChannels));
    if (channels_ == 0) return release(), false;
    sample_rate_ = codec_->sample_rate ? codec_->sample_rate : 48'000;

    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, static_cast<int>(channels_));
    // 输出率恒为混音域 48k（铁律：此处传 sample_rate_ 会变成源率直通——
    // 44.1k 素材不重采样、按 48k 播放音调偏高 8.8%，且 pts 定位逐帧
    // 偏差靠补静音掩盖；48k 素材 in==out 全绿，极易漏测）
    if (swr_alloc_set_opts2(&swr_, &out_layout, AV_SAMPLE_FMT_FLT,
                            static_cast<int>(kMixRate), &codec_->ch_layout,
                            codec_->sample_fmt, codec_->sample_rate,
                            0, nullptr) < 0 ||
        swr_init(swr_) < 0) {
        av_channel_layout_uninit(&out_layout);
        return release(), false;
    }
    av_channel_layout_uninit(&out_layout);

    frame_ = av_frame_alloc();
    pkt_   = av_packet_alloc();
    if (!frame_ || !pkt_) return release(), false;
    scratch_.reserve(4096u * channels_);
    return true;
}

void AudioMediaSource::startThread() {
    stop_ = false;
    thread_ = std::thread([this] { decodeLoop(); });
}

// ---------------------------------------------------------------------------
// release：线程先停（铁律）→ FFmpeg 句柄逐个释放（析构唯一入口）
// ---------------------------------------------------------------------------
void AudioMediaSource::release() noexcept {
    stop_ = true;
    if (thread_.joinable()) thread_.join(); // 解码线程 ≤50ms 自查停止旗标
    if (pkt_)   { av_packet_free(&pkt_); }
    if (frame_) { av_frame_free(&frame_); }
    if (swr_)   { swr_free(&swr_); }
    if (codec_) { avcodec_free_context(&codec_); }
    if (fmt_)   { avformat_close_input(&fmt_); }
    channels_ = 0;
    opened_.store(false, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// decodeLoop：解码线程体（解包→解码→推环；代次变化 = seek 重灌；EOF 待命）
// ---------------------------------------------------------------------------
void AudioMediaSource::decodeLoop() {
    std::uint32_t my_gen = gen_.load(std::memory_order_relaxed);
    for (;;) {
        if (stop_.load(std::memory_order_relaxed)) return;
        if (gen_.load(std::memory_order_relaxed) != my_gen) {
            // acquire 与 doSeek 的 release store 配对：构成 happens-before，
            // 请求值必然完整可见（不依赖 x86 TSO 的碰巧正确）
            const std::int64_t req =
                seek_req_.exchange(-1, std::memory_order_acquire);
            handleSeek(req < 0 ? 0 : req);
            my_gen = gen_.load(std::memory_order_relaxed);
            ack_gen_.store(my_gen, std::memory_order_relaxed);
            continue;
        }
        if (eof_.load(std::memory_order_relaxed)) {
            // 【2026-11-13 性能】EOF 待命轮询 20ms→200ms——多声源场景下
            // 每秒数千次无谓唤醒；seek 走 gen 握手（decodeLoop 顶部），
            // 不依赖本轮询的及时性
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        if (!decodeOne(my_gen)) eof_.store(true, std::memory_order_relaxed);
    }
}

// ---------------------------------------------------------------------------
// decodeOne：receive 驱动的标准解码节拍（EAGAIN 读包喂入；EOF 冲洗收尾）
// ---------------------------------------------------------------------------
bool AudioMediaSource::decodeOne(std::uint32_t my_gen) {
    const int ret = avcodec_receive_frame(codec_, frame_);
    if (ret == 0) {
        pushFrame(my_gen);
        av_frame_unref(frame_);
        return true;
    }
    if (ret == AVERROR(EAGAIN)) {
        const int rp = av_read_frame(fmt_, pkt_);
        if (rp < 0) {
            // 容器尽头：冲洗解码器收尾帧（后续 receive 给出 AVERROR_EOF）
            avcodec_send_packet(codec_, nullptr);
            return true;
        }
        if (pkt_->stream_index == stream_idx_)
            avcodec_send_packet(codec_, pkt_);
        av_packet_unref(pkt_);
        return true;
    }
    if (ret == AVERROR_EOF) {
        // 解码冲洗完毕：swr 内部还压着重采样滤波器延迟尾（非 48k 素材
        // 不可忽略），一次性排空推环后再进入 EOF 待命
        flushSwrTail(my_gen);
        return false;
    }
    return false; // 其他错误 → EOF 待命
}

// ---------------------------------------------------------------------------
// flushSwrTail：EOF 时排空 swr 内部延迟样本（NULL 输入冲洗）
// ---------------------------------------------------------------------------
void AudioMediaSource::flushSwrTail(std::uint32_t my_gen) {
    if (!swr_) return;
    const int delay = static_cast<int>(swr_get_delay(swr_, 48'000));
    if (delay <= 0) return;
    scratch_.resize(static_cast<std::size_t>(delay + 64) * channels_);
    auto* dst8 = reinterpret_cast<std::uint8_t*>(scratch_.data());
    const int converted =
        swr_convert(swr_, &dst8, delay + 64, nullptr, 0);
    if (converted <= 0) return;
    const auto abort_fn = [this, my_gen] {
        return stop_.load(std::memory_order_relaxed) ||
               gen_.load(std::memory_order_relaxed) != my_gen;
    };
    ring_->pushBlocking(scratch_.data(),
                        static_cast<std::uint64_t>(converted), abort_fn);
    next_push_ += static_cast<std::uint64_t>(converted);
}

// ---------------------------------------------------------------------------
// pushFrame：帧 pts → 绝对样本号 → swr 交错 float32 → 环（代次中止保护）
// ---------------------------------------------------------------------------
void AudioMediaSource::pushFrame(std::uint32_t my_gen) {
    // 代次检查（推送前；推送中由中止谓词再护一道）
    if (gen_.load(std::memory_order_relaxed) != my_gen) return;
    // 时间戳：best_effort 优先（FfmpegMediaSource 同款口径）→ 绝对样本号
    std::int64_t pts = frame_->best_effort_timestamp;
    if (pts == AV_NOPTS_VALUE) pts = frame_->pts;
    std::int64_t frame_start = next_push_;
    if (pts >= 0)
        frame_start =
            av_rescale_q(pts, AVRational{tb_num_, tb_den_}, kSampleTb);

    // swr → 交错 float32 暂存
    // 输出容量必须按重采样后的实际上限取（swr_get_out_samples）：
    // 若钳在输入样本数，44.1k→48k 时每帧约 1/9 输出被截留在 swr 内部
    // 缓冲、后续才漏出 → 时间轴被拉伸（实测 440Hz 正弦前后段呈 480Hz）。
    const int out_max =
        swr_get_out_samples(swr_, frame_->nb_samples);
    if (out_max <= 0) return;
    scratch_.resize(static_cast<std::size_t>(out_max) * channels_);
    auto* dst8 = reinterpret_cast<std::uint8_t*>(scratch_.data());
    const int converted = swr_convert(
        swr_, &dst8, out_max,
        const_cast<const std::uint8_t**>(frame_->extended_data),
        frame_->nb_samples);
    if (converted <= 0) return;

    // 推环（代次不匹配/停止即中止——abort 后由 seek 分支统一清环）
    const auto abort_fn = [this, my_gen] {
        return stop_.load(std::memory_order_relaxed) ||
               gen_.load(std::memory_order_relaxed) != my_gen;
    };
    // 间隙补静音（pts 前跳，保绝对对位）。【巨间隙防爆 2026-11-11】
    // 间隙超出环容量时不再分配/推环（音频起点晚/pts 异常时 gap 可达
    // GB 级 → bad_alloc → std::terminate → 无 dmp 闪退——editor 频繁
    // 跳转闪退主嫌疑）：清环 + skip_to_ 握手，读侧入口对齐游标（间隙
    // 本就无声，与补静音语义等价且零分配）。
    if (frame_start > next_push_) {
        const std::uint64_t gap =
            static_cast<std::uint64_t>(frame_start - next_push_);
        if (gap > ring_->capacityFrames()) {
            ring_->clear(); // 丢旧位置残余（游标将由 skip_to_ 跳过）
            skip_to_.store(frame_start, std::memory_order_relaxed);
        } else if (gap > 0) {
            std::vector<float> silence(
                static_cast<std::size_t>(gap) * channels_, 0.0f);
            ring_->pushBlocking(silence.data(), gap, abort_fn);
        }
    }
    std::uint64_t skip = 0;
    if (frame_start < next_push_) // seek 落点前重叠：跳过前缀
        skip = static_cast<std::uint64_t>(
            std::min<std::int64_t>(next_push_ - frame_start, converted));
    const std::uint64_t usable =
        static_cast<std::uint64_t>(converted) - skip;
    if (usable > 0)
        ring_->pushBlocking(scratch_.data() + skip * channels_, usable,
                            abort_fn);
    next_push_ = frame_start + converted;
}

// ---------------------------------------------------------------------------
// doSeek：读侧回退重灌（代次握手——ack 后环内保证只有新代次数据）
// ---------------------------------------------------------------------------
void AudioMediaSource::doSeek(std::int64_t target_sample) {
    // 先写请求（release）再抬代次：解码线程看到代次变化后以 acquire
    // exchange 取走请求——release/acquire 构成 happens-before，标准语义
    // 下请求必然可见（原 relaxed 版仅在 x86 TSO 下碰巧成立，跨平台/
    // 编译器重排理论上可 exchange 到 -1 → 误 seek 到 0）
    seek_req_.store(target_sample, std::memory_order_release);
    const std::uint32_t g = gen_.fetch_add(1) + 1; // 在途推送全部中止
    ring_->clear(); // 丢弃旧代次样本 + 唤醒阻塞中的 push
    cursor_ = target_sample;
    const std::int64_t deadline = nowMs() + 200;
    while (ack_gen_.load(std::memory_order_relaxed) != g &&
           nowMs() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

void AudioMediaSource::handleSeek(std::int64_t target_sample) {
    avcodec_flush_buffers(codec_);
    av_seek_frame(fmt_, stream_idx_,
                  av_rescale_q(target_sample, kSampleTb,
                               AVRational{tb_num_, tb_den_}),
                  AVSEEK_FLAG_BACKWARD);
    ring_->clear();
    // 解码侧推进游标同步重置到目标：BACKWARD 落点 L ≤ target 的重叠
    // 帧由 pushFrame 的前缀跳过逻辑裁齐（不重置则旧游标把整帧吞掉，
    // 环内数据恒偏差一个包——实测 4096 样本相位错位）。
    next_push_ = static_cast<std::uint64_t>(target_sample);
    eof_.store(false, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// read：media_us 定址 → 回退 seek / 前向 drop 追赶 → 环内取数
// ---------------------------------------------------------------------------
std::uint32_t AudioMediaSource::read(float* dst, std::int64_t media_us,
                                     std::uint32_t frames) {
    if (!opened_.load(std::memory_order_relaxed) || frames == 0) return 0;
    // 目标样本号（混音域 48k——环内数据与 push 侧 next_push_ 皆为本域；
    // 若误用源采样率，48k 域推进的 cursor_ 会持续超前 target，每 ~47ms
    // 误触发一次回退重灌，时间轴被搅乱）（四舍五入——防浮点累积误差）
    std::int64_t target =
        (media_us * static_cast<std::int64_t>(kMixRate) + 500'000) /
        1'000'000;
    if (target < 0) target = 0;

    // 巨间隙跳过握手（解码侧 skip_to_）：cursor 直接对齐到实际数据起点
    //（被跳过区间 [cursor_, sk) 无声）。目标落在间隙内 = 读静音——绝不能
    // 回退 seek（间隙无数据，seek 到起点只会 ping-pong 空转）；目标早于
    // 间隙起点 = 真回退 → 走下方正常 doSeek。
    if (const std::int64_t sk = skip_to_.exchange(-1); sk >= 0 && sk > cursor_) {
        const std::int64_t gap_lo = cursor_;
        cursor_ = sk;
        if (target >= gap_lo && target < sk) return 0; // 间隙内 = 静音拍
    }

    // 回退（超过容差）：seek 重灌（LOOP/scrub 场景）
    if (cursor_ - target > kBackwardTolSamples) doSeek(target);

    // 前向跳变：drop 追赶（等解码供给，30ms 上限——追不平则本拍静音）。
    // 【等待上限评估（[S3] 2026-11-11）】原 200ms 超 33ms 拍预算（冷盘/
    // 供给停滞时单拍卡 UI 半秒级）。等待条件为轮询驱动（read 侧 5ms 睡眠
    // × 解码侧持续推环），非 cv 一次性唤醒——缩短 deadline 无损自愈：
    // cursor_ < target 状态跨拍持久，未追平部分由后续拍分摊消化（seek
    // 风暴下每拍推进一点，不比长等一拍更差，且 UI 不再被单拍钉死）。
    if (target > cursor_) {
        const std::int64_t deadline = nowMs() + 30;
        while (cursor_ < target) {
            const std::uint64_t d = ring_->drop(
                static_cast<std::uint64_t>(target - cursor_));
            cursor_ += static_cast<std::int64_t>(d);
            if (d > 0) continue;
            if (eof_.load(std::memory_order_relaxed) ||
                stop_.load(std::memory_order_relaxed) || nowMs() > deadline)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // 首数据等待（线程启动/seek 后重灌窗口；上限 20ms——混音拍 ≤33ms，
    // 常态首拍即有数据，此等待只覆盖慢盘冷启动）。
    // 【等待上限评估（[S3] 2026-11-11）】原 100ms 同上：轮询驱动、无
    // cv 唤醒依赖——超时仅本拍返回 0（静音拍），游标/环状态不变，下一
    // 拍重试自愈（冷启动慢盘最坏多几个静音拍，UI 不再被单拍钉死）。
    if (ring_->available() == 0 &&
        !eof_.load(std::memory_order_relaxed)) {
        const std::int64_t deadline = nowMs() + 20;
        while (ring_->available() == 0 &&
               !eof_.load(std::memory_order_relaxed) &&
               nowMs() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    const std::uint64_t got =
        ring_->pop(dst, static_cast<std::uint64_t>(frames));
    cursor_ += static_cast<std::int64_t>(got);
    return static_cast<std::uint32_t>(got);
}

} // namespace ms
