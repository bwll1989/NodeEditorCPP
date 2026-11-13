// ============================================================================
//  AudioMediaSource.hpp —— 素材音频解码源（P2 解码副通道，2026-11-10）
//
//  定位（对照方案决策 2）：IAudioSource 的真实媒体实现——demux 选音频流 →
//  解码 → swr 统一 float32/48k（通道 layout 保持源声明原样直传）→ PcmRing；
//  解码线程前置生产，混音拍经 read(media_us) 定点消费。
//
//  时间语义：media_us = 素材内部时间（音频帧与视频帧同一坐标系）——
//  换算样本号 = media_us × 48000 / 1e6（整数四舍五入，避免浮点累积）。
//  read 的游标语义：
//    * 单调前进：环内数据被按序取走（跳前 = drop 追赶，上限内等待解码）；
//    * 回退（loop/scrub）：seek 重灌——代次（gen）握手保证环内不残留
//      旧位置样本（读侧 bump gen + 清环 + 等 ack；解码线程在推送前检查
//      代次，不匹配即丢弃在途帧并执行 av_seek BACKWARD + 解码器冲洗）；
//    * 不足（解码滞后/素材尽头）：返回实取帧数，剩余由调用方补零（静音）。
//
//  线程纪律（"析构先停线程铁律"同款）：stop_ 置位 → join → FFmpeg 句柄
//  释放（成员逆序析构前解码线程必须已死——缓冲/上下文均被其触碰）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "audio/AudioTypes.hpp"
#include "audio/PcmRing.hpp"

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

namespace ms {

class AudioMediaSource final : public IAudioSource {
public:
    /// \param assets_root 素材根目录（空 = path 即可用路径，测试场景）
    /// \param path        素材相对路径（Asset.path 原样）
    AudioMediaSource(std::string assets_root, std::string path);
    ~AudioMediaSource() override;

    // IAudioSource（混音引擎入口）
    std::uint32_t channelCount() const noexcept override { return channels_; }
    std::uint32_t read(float* dst, std::int64_t media_us,
                       std::uint32_t frames) override;

    /// 素材是否含可用音频流（open 失败/无音轨 = false——调用方回退正弦源）
    bool opened() const noexcept { return opened_.load(std::memory_order_relaxed); }
    /// 解码游标（诊断：绝对样本索引）
    std::int64_t cursor() const noexcept { return cursor_; }
    bool eof() const noexcept { return eof_.load(std::memory_order_relaxed); }

private:
    bool openInput();       ///< 容器/解码器/swr 建立（构造期一次）
    void startThread();     ///< 解码线程懒启动（首个 read 前就绪）
    void decodeLoop();      ///< 线程体：解包→解码→swr→推环
    bool decodeOne(std::uint32_t my_gen); ///< 完整解码节拍（false=EOF/错误）
    void pushFrame(std::uint32_t my_gen); ///< 帧→swr 交错→环（代次中止）
    void flushSwrTail(std::uint32_t my_gen); ///< EOF 冲洗 swr 滤波器延迟尾
    void doSeek(std::int64_t target_sample);      ///< 读侧回退重灌（握手）
    void handleSeek(std::int64_t req);            ///< 解码侧执行 seek
    void release() noexcept;                      ///< 线程先停 + FFmpeg 释放

    std::string assets_root_;
    std::string path_;

    // ---- FFmpeg 句柄（仅解码线程触碰；构造/release 于宿主线程）----
    AVFormatContext* fmt_   = nullptr;
    AVCodecContext*  codec_ = nullptr;
    SwrContext*      swr_   = nullptr;
    AVFrame*         frame_ = nullptr;
    AVPacket*        pkt_   = nullptr;
    int              stream_idx_ = -1;
    int              tb_num_  = 0; ///< 音频流 time_base（分子/分母——AVRational
    int              tb_den_  = 1; ///<  是 FFmpeg 值类型，不入头避免 FFmpeg 泄漏）

    // ---- 混音域规格（位置直通：通道数保持源声明）----
    std::uint32_t channels_    = 0;   ///< 0 = 无音频流
    int           sample_rate_ = 48'000; ///< 源采样率（仅供 swr 入端配置；
                                         ///<  环内数据/游标簿记恒为 48k 混音域）

    // ---- 环形缓冲 + 游标/代次 ----
    std::unique_ptr<PcmRing> ring_;
    std::int64_t  cursor_  = 0;  ///< 下一个交付的绝对样本号（读侧独占）
    std::int64_t  next_push_ = 0; ///< 下一个推环的绝对样本号（解码侧独占；
                                  ///<  seek 后按新帧 pts 重定位，间隙补静音）
    std::atomic<std::uint32_t> gen_{0};     ///< 代次（seek 请求递增）
    std::atomic<std::uint32_t> ack_gen_{0}; ///< 解码侧已处理的代次
    std::atomic<std::int64_t>  seek_req_{-1}; ///< 待执行 seek 目标（样本号）
    /// 巨间隙跳过握手（2026-11-11）：解码侧发现 pts 前跳超出环容量时置位
    /// 目标样本号——不再全量补静音（GB 级 vector = bad_alloc → terminate
    /// → 无 dmp 闪退），读侧入口对齐游标（间隙本就无声，语义等价）。
    std::atomic<std::int64_t>  skip_to_{-1};
    std::atomic<bool> opened_{false};
    std::atomic<bool> eof_{false};
    std::atomic<bool> stop_{false};
    std::thread       thread_;
    std::vector<float> scratch_;  ///< swr 交错转换暂存（解码线程复用）
};

} // namespace ms
