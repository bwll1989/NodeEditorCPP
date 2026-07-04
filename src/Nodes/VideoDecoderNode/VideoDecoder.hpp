#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"

#include "NodeDataList.hpp"
#include "Common/Devices/TimestampGenerator/TimestampGenerator.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#include <iostream>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <portaudio.h>
#include <QDateTime>
#include <QByteArray>
#include <thread>
#include <atomic>


/**
 * @file VideoDecoder.hpp
 * @brief 基于 FFmpeg 的音视频文件解码引擎（节点内部使用，非 QtNodes 模型本身）
 *
 * ## 总体架构
 *
 * ```
 *                    ┌──────────────── audioLoop (std::thread) ────────────────┐
 *                    │  读包 → 解码 → 重采样 → 切片 → audioFrameReady 信号    │
 *                    │                          ↓ DirectConnection            │
 *                    │                   handleAudioFrame                     │
 *                    │                          ↓                             │
 *                    │              AudioTimestampRingQueue (每声道)          │
 *                    └────────────────────────────────────────────────────────┘
 *
 *                    ┌──────────────── videoLoop (std::thread) ────────────────┐
 *                    │  读包 → 解码 → PTS 节流 → processVideoFrame              │
 *                    │              YUV → BGRA (sws_scale)                      │
 *                    │              swap(decodeFrame_, pendingVideoFrame_)      │
 *                    │                          ↓ QueuedConnection            │
 *                    └────────────── uploadPendingVideoFrame (GUI 线程) ────────┘
 *                                   BGRA → GPU 纹理 → ImageTimestampRingQueue
 * ```
 *
 * ## 设计要点
 *
 * - **双 AVFormatContext**：同一文件打开两次，音频/视频各一条独立读包链路，互不阻塞。
 * - **三线程模型**：audioLoop / videoLoop 为 std::thread；VideoDecoder 对象驻留 GUI 线程，
 *   GPU 上传通过 QMetaObject::invokeMethod 投递回 GUI 线程（QOffscreenSurface 须在 GUI 创建）。
 * - **视频零拷贝 CPU 路径**：decodeFrame_ 与 pendingVideoFrame_ 双缓冲 swap，无每帧 clone。
 * - **sws 直出 BGRA**：与 ImageReadback::uploadBgra8Mat 对齐，跳过 BGR→BGRA 色彩转换。
 * - **QThread 继承**：run() 已弃用，仅保留空实现以兼容 QThread 接口；实际工作在 std::thread 中。
 */
class VideoDecoder : public QThread {
Q_OBJECT

public:
    explicit VideoDecoder(QObject *parent = nullptr);

    ~VideoDecoder();

    /**
     * @brief 打开媒体文件并初始化 FFmpeg 解码器
     *
     * 会 stopPlay + cleanupFFmpeg 清理旧状态，再分别打开音频/视频 formatContext。
     * 调用方须在 GUI 或节点线程调用；内部会阻塞直至初始化完成。
     *
     * @param filePath 媒体文件绝对路径
     * @return 含 bit_rate / channels / video_width 等字段的 JSON；失败返回 nullptr
     */
    QJsonObject* initializeFFmpeg(const QString &filePath);

    /** @brief 从头 seek 并启动 audioLoop + videoLoop */
    void startPlay();

    /** @brief join 解码线程，清空 pending 帧与各 Audio ring buffer */
    void stopPlay();

    /** @brief 设置音量增益，单位 dB（applyVolume 内转为线性增益） */
    void setVolume(double vol);

    float getVolume() const;

    void setLooping(bool loop);

    bool getLooping() const;

    /** @brief 是否处于播放状态（isPlaying 原子标志） */
    bool getPlaying() const;

    /** @brief 兼容旧接口，等同 startPlay() */
    void playAudio();

    /** @brief 返回音频流声道数；无音频轨或未初始化时返回 0 */
    int getChannels() const {
        if (codecContext) {
            return codecContext->ch_layout.nb_channels;
        }
        return 0;
    }

    /** @brief 是否已成功打开音频轨（纯视频文件为 false） */
    bool hasAudioStream() const {
        return audioStreamIndex >= 0 && codecContext != nullptr && formatContext != nullptr;
    }

    /** @brief 是否已成功打开视频轨 */
    bool hasVideoStream() const {
        return videoStreamIndex >= 0 && videoCodecContext != nullptr && formatContextVideo != nullptr;
    }

    /**
     * @brief 绑定视频输出的共享 ImageTimestampRingQueue
     *
     * 由 VideoDecoderDataModel 在构造/换文件时调用。
     * 解码线程只写 pendingVideoFrame_；pushFrame 在 GUI 线程 uploadPendingVideoFrame 中执行。
     */
    void setVideoImageBuffer(std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue> buffer);

signals:
    /** @brief 音频固定帧块就绪；DirectConnection 到 handleAudioFrame 做声道分离 */
    void audioFrameReady(AudioFrame frame);

    /** @brief 播放进度 (当前秒, 总时长秒)，约 100ms 节流 */
    void playbackProgress(double currentSec, double totalSec);

    /**
     * @brief 非循环模式下自然播放到文件末尾时发出
     * @note 主动 stopPlay() 不会触发
     */
    void playbackFinished();

public slots:
    /**
     * @brief 将交错 PCM 拆分为单声道帧并 push 到各 AudioTimestampRingQueue
     * @param frame 交错 float PCM，channels 为原始声道数
     */
    void handleAudioFrame(AudioFrame frame);

    /** @brief 获取指定声道的共享音频 ring buffer（按需懒创建） */
    std::shared_ptr<AudioTimestampRingQueue> getAudioBuffer(int index);

private slots:
    /**
     * @brief 【GUI 线程】取走 pendingVideoFrame_，上传 BGRA 纹理并 push 到 videoImageBuffer_
     *
     * 若上传期间解码线程又 swap 了新帧（pending 非空），会再次 scheduleVideoUploadIfNeeded，
     * 实现"只保留最新帧"的合并，避免事件队列堆积。
     */
    void uploadPendingVideoFrame();

protected:
    /** @brief QThread 入口（已弃用，空实现） */
    void run() override;

    /**
     * @brief 【videoLoop 线程】单帧后处理
     *
     * 1. ensureVideoDecodeBuffer → sws_scale 输出 BGRA 到 decodeFrame_
     * 2. swap(decodeFrame_, pendingVideoFrame_) 与 GUI 侧交换缓冲
     * 3. scheduleVideoUploadIfNeeded 投递 GPU 上传
     */
    void processVideoFrame();

    /**
     * @brief 合并多次上传请求：同一时刻至多一个 QueuedConnection 在途
     *
     * videoUploadScheduled_ 为 false 时才 invokeMethod，防止 GUI 事件队列无限增长。
     */
    void scheduleVideoUploadIfNeeded();

private:
    /** @brief 【std::thread】音频读包/解码/重采样/切片主循环 */
    void audioLoop();

    /** @brief 【std::thread】视频读包/解码/PTS 同步/processVideoFrame 主循环 */
    void videoLoop();

    /** @brief 将 dB 音量转为线性增益并削波到 [-1, 1] */
    void applyVolume(uint8_t* data, int sampleCount, int channels);

    /**
     * @brief 累积 PCM 至 SAMPLES_PER_CHANNEL 后 emit audioFrameReady
     * @return 本次调用 emit 的帧数
     */
    int processPcmAndEmitFixedFrames(const uint8_t* interleavedPcm, int samplesPerChannel, int channels, int sampleRate);

    /** @brief 释放全部 FFmpeg 上下文与帧缓冲 */
    void cleanupFFmpeg();

    /** @brief 释放 swsContext 与 decodeFrame_（分辨率变化或换文件时调用） */
    void releaseVideoScaler();

    /**
     * @brief 按当前视频尺寸（重新）创建 swsContext 与 decodeFrame_
     *
     * sws 输出 AV_PIX_FMT_BGRA；decodeFrame_ 为 CV_8UC4 连续 Mat。
     * swap 后 decodeFrame_ 可能为空，本函数会在尺寸不变时仅 recreate Mat。
     */
    void ensureVideoDecodeBuffer(int width, int height);

private:
    // ── 线程同步 ──────────────────────────────────────────────────────────
    mutable QMutex mutex;       ///< 保护 videoImageBuffer_ / pending 帧 / 音量 / 循环标志等
    QWaitCondition condition;   ///< 预留：线程间等待唤醒

    // ── FFmpeg：双 formatContext ───────────────────────────────────────────
    AVFormatContext *formatContext;       ///< 音频专用：读包、seek、duration
    AVFormatContext *formatContextVideo;  ///< 视频专用：与音频并行读同一文件

    // ── 音频解码链 ─────────────────────────────────────────────────────────
    AVCodecContext *codecContext;   ///< 音频 AVCodecContext
    const AVCodec *codec;           ///< 音频解码器
    AVFrame *audioFrame;            ///< 解码输出帧（audioLoop 内 alloc）
    SwrContext *swrContext;         ///< 重采样至 48000 Hz / FLT（按需创建）
    int audioStreamIndex;           ///< 音频流在 formatContext 中的索引

    // ── 视频解码链 ─────────────────────────────────────────────────────────
    AVCodecContext *videoCodecContext;  ///< 视频 AVCodecContext
    const AVCodec *videoCodec;          ///< 视频解码器
    AVFrame *videoFrame;                ///< 解码输出帧（videoLoop 内 alloc，YUV）
    SwsContext *swsContext;             ///< YUV → BGRA 色彩空间转换
    int videoStreamIndex;               ///< 视频流索引
    cv::Mat decodeFrame_;               ///< sws 写入目标；与 pendingVideoFrame_ swap 双缓冲
    int videoWidth;                     ///< 当前视频宽（用于 sws 重建判断）
    int videoHeight;                    ///< 当前视频高
    double videoClock;                  ///< 预留：视频时钟

    AVPacket *packet;               ///< 预留 packet 指针
    uint8_t *resampledBuffer;       ///< 预留重采样缓冲

    // ── 播放状态（跨线程原子标志）──────────────────────────────────────────
    std::atomic<bool> isPlaying;     ///< 用户期望的播放状态
    std::atomic<bool> isLooping;     ///< 到 EOF 是否 seek 回起点
    float volume;                    ///< 音量 dB，applyVolume 读取
    std::atomic<bool> audioRunning{false};  ///< audioLoop 是否在运行
    std::atomic<bool> videoRunning{false};  ///< videoLoop 是否在运行
    std::thread audioThread;
    std::thread videoThread;

    // ── 输出缓冲 ───────────────────────────────────────────────────────────
    /** 每声道一个 AudioTimestampRingQueue，handleAudioFrame push */
    std::map<int, std::shared_ptr<AudioTimestampRingQueue>> channelAudioBuffers;

    /** 视频 ImageTimestampRingQueue，由 DataModel 注入；push 在 GUI 线程 */
    std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue> videoImageBuffer_;

    /** 待上传 BGRA 帧（与 decodeFrame_ swap 获得所有权） */
    cv::Mat pendingVideoFrame_;
    /** 对应 pending 帧的 TimestampGenerator 帧号 */
    qint64 pendingVideoTimestamp_ = -1;
    /** 是否已有 uploadPendingVideoFrame 在 Qt 事件队列中 */
    std::atomic<bool> videoUploadScheduled_{false};

    // ── 音频 PCM 切片累积 ───────────────────────────────────────────────────
    QByteArray pendingInterleavedPcm_;  ///< 不足 SAMPLES_PER_CHANNEL 的尾部 PCM
    int pendingSamplesPerChannel_ = 0;
    int lastChannels_ = 0;              ///< 声道数变化时清空 pendingInterleavedPcm_

    // ── 时间戳 ─────────────────────────────────────────────────────────────
    TimestampGenerator* timestampGenerator_;  ///< 全局 tick；视频 pending / 音频 frame 对齐用
    uint64_t lastTimestamp_ = 0;              ///< 音频 emit 递增帧号基准
};
