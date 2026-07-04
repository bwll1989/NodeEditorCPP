/**
 * @file VideoDecoder.cpp
 * @brief VideoDecoder 实现：FFmpeg 双路解码 + 视频 BGRA 双缓冲 + GUI 线程 GPU 上传
 *
 * 常量说明：
 *   SAMPLE_RATE           — 系统统一音频采样率 48000 Hz
 *   SAMPLES_PER_CHANNEL   — 每个音频 tick 块包含的采样数（48000 / 帧率）
 *   FIXED_DELAY_FRAMES    — 音频时间戳相对系统 tick 的固定超前量
 *   LOOP_INTERVAL         — 循环播放 seek 回起点前的间隔 ms
 */

#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
#include "NodeDataList.hpp"  // 确保包含此头文件
#include "VideoDecoder.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include <iostream>
#include <cmath>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <portaudio.h>
#include <QDateTime>
#include <QElapsedTimer>
// #include <Common/Devices/AudioPipe/AudioPipe.h>

/** 系统统一输出采样率（Hz） */
static const int SAMPLE_RATE = 48000;
/** 循环播放 seek 回文件头前的等待（ms），避免连续 seek 过于激进 */
static const int LOOP_INTERVAL = 800;
/** 音频帧 timestamp 相对 TimestampGenerator 的固定超前帧数 */
static const int FIXED_DELAY_FRAMES = 5;
/** 每个音频输出块：每声道采样数 = 48000 / 系统 tick 帧率 */
static const int SAMPLES_PER_CHANNEL = SAMPLE_RATE/TimestampGenerator::getInstance()->getFrameRate();;

/**
 * @brief 构造函数
 *
 * - 对象 thread affinity 为创建者线程（通常为 GUI / 节点线程）
 * - audioFrameReady → handleAudioFrame 使用 DirectConnection：
 *   在 audioLoop 线程内同步完成声道分离与 ring buffer push，避免音频队列延迟
 */
VideoDecoder::VideoDecoder(QObject *parent)
    : QThread(parent)
    , formatContext(nullptr)
    , formatContextVideo(nullptr)
    , codecContext(nullptr)
    , codec(nullptr)
    , audioFrame(nullptr)
    , swrContext(nullptr)
    , audioStreamIndex(-1)
    , videoCodecContext(nullptr)
    , videoCodec(nullptr)
    , videoFrame(nullptr)
    , swsContext(nullptr)
    , videoStreamIndex(-1)
    , videoWidth(0)
    , videoHeight(0)
    , videoClock(0.0)
    , packet(nullptr)
    , resampledBuffer(nullptr)
    , isPlaying(false)
    , isLooping(false)
    , volume(-10.0f)
    , timestampGenerator_(TimestampGenerator::getInstance())  // 获取全局时间戳生成器实例
{
    qRegisterMetaType<AudioFrame>("AudioFrame");
    connect(this, &VideoDecoder::audioFrameReady,
            this, &VideoDecoder::handleAudioFrame,
            Qt::DirectConnection);
}

VideoDecoder::~VideoDecoder() {
    stopPlay();
    cleanupFFmpeg();
}

/**
 * @brief 初始化音视频 FFmpeg 上下文
 *
 * 双 AVFormatContext 并行读同一文件；**音频轨可选**（纯视频文件仅初始化视频链）。
 * 至少需存在一条有效视频轨或音频轨，否则 cleanup 后返回 nullptr。
 * 所有失败路径均调用 cleanupFFmpeg()，避免半初始化状态下 av_seek_frame(-1) 崩溃。
 */
QJsonObject* VideoDecoder::initializeFFmpeg(const QString &filePath){
    // 切换媒体前必须停掉解码线程并释放旧上下文，否则播放中换片会崩溃
    stopPlay();
    cleanupFFmpeg();

    pendingInterleavedPcm_.clear();
    pendingSamplesPerChannel_ = 0;
    lastChannels_ = 0;
    lastTimestamp_ = timestampGenerator_->getCurrentFrameCount();

    avformat_network_init();

    // ── 视频上下文（独立读包线程使用）──
    if (avformat_open_input(&formatContextVideo, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug() << "打开文件失败(视频)" << filePath;
        cleanupFFmpeg();
        return nullptr;
    }
    if (avformat_find_stream_info(formatContextVideo, nullptr) != 0) {
        qDebug() << "找不到视频流信息";
        cleanupFFmpeg();
        return nullptr;
    }

    videoStreamIndex = av_find_best_stream(formatContextVideo, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex != -1) {
        videoCodec = avcodec_find_decoder(formatContextVideo->streams[videoStreamIndex]->codecpar->codec_id);
        if (videoCodec) {
            videoCodecContext = avcodec_alloc_context3(videoCodec);
            if (videoCodecContext) {
                if (avcodec_parameters_to_context(videoCodecContext,
                                                  formatContextVideo->streams[videoStreamIndex]->codecpar) >= 0 &&
                    avcodec_open2(videoCodecContext, videoCodec, nullptr) >= 0) {
                    videoWidth = videoCodecContext->width;
                    videoHeight = videoCodecContext->height;
                } else {
                    avcodec_free_context(&videoCodecContext);
                    videoCodecContext = nullptr;
                    videoStreamIndex = -1;
                }
            }
        }
    }

    // ── 音频上下文（可选；纯视频文件无音频轨时跳过）──
    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug() << "打开文件失败(音频)" << filePath;
        if (!hasVideoStream()) {
            cleanupFFmpeg();
            return nullptr;
        }
        // 纯视频：音频容器打开失败可忽略
    } else if (avformat_find_stream_info(formatContext, nullptr) != 0) {
        qDebug() << "找不到音频流信息";
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    } else {
        audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamIndex >= 0) {
            codec = avcodec_find_decoder(formatContext->streams[audioStreamIndex]->codecpar->codec_id);
            if (!codec) {
                qDebug() << "找不到音频解码器";
                audioStreamIndex = -1;
            } else {
                codecContext = avcodec_alloc_context3(codec);
                const int ret = avcodec_parameters_to_context(codecContext,
                                                              formatContext->streams[audioStreamIndex]->codecpar);
                if (ret < 0 || avcodec_open2(codecContext, codec, nullptr) < 0) {
                    qDebug() << "打开音频解码器失败";
                    avcodec_free_context(&codecContext);
                    codecContext = nullptr;
                    codec = nullptr;
                    audioStreamIndex = -1;
                }
            }
        }

        // 无音频轨或音频打开失败：关闭音频 formatContext，避免后续误用
        if (audioStreamIndex < 0) {
            avformat_close_input(&formatContext);
            formatContext = nullptr;
        }
    }

    if (!hasVideoStream() && !hasAudioStream()) {
        qDebug() << "文件中既无有效视频轨也无有效音频轨:" << filePath;
        cleanupFFmpeg();
        return nullptr;
    }

    // ── 音频重采样（仅在有音频轨时）──
    if (hasAudioStream()) {
        const bool needsResampling = (codecContext->sample_rate != SAMPLE_RATE) ||
                                    (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT);

        if (needsResampling) {
            AVChannelLayout outChannelLayout;
            AVChannelLayout inChannelLayout = codecContext->ch_layout;

            if (inChannelLayout.nb_channels == 1) {
                outChannelLayout = AV_CHANNEL_LAYOUT_MONO;
            } else if (inChannelLayout.nb_channels == 2) {
                outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
            } else {
                outChannelLayout = inChannelLayout;
            }

            if (swr_alloc_set_opts2(&swrContext,
                                    &outChannelLayout,
                                    AV_SAMPLE_FMT_FLT,
                                    SAMPLE_RATE,
                                    &inChannelLayout,
                                    codecContext->sample_fmt,
                                    codecContext->sample_rate,
                                    0,
                                    nullptr) != 0) {
                qDebug() << "swr_alloc_set_opts2 fail";
                cleanupFFmpeg();
                return nullptr;
            }

            if (codecContext->sample_rate == 44100) {
                av_opt_set(swrContext, "resampler", "swr", 0);
                av_opt_set_int(swrContext, "filter_size", 64, 0);
                av_opt_set_int(swrContext, "phase_shift", 12, 0);
                av_opt_set_double(swrContext, "cutoff", 0.99, 0);
                av_opt_set(swrContext, "dither_method", "shibata", 0);
                av_opt_set_int(swrContext, "linear_interp", 1, 0);
                av_opt_set_int(swrContext, "exact_rational", 1, 0);
            } else {
                av_opt_set(swrContext, "resampler", "swr", 0);
                av_opt_set_int(swrContext, "filter_size", 32, 0);
                av_opt_set_int(swrContext, "phase_shift", 10, 0);
                av_opt_set_double(swrContext, "cutoff", 0.98, 0);
                av_opt_set(swrContext, "dither_method", "triangular", 0);
                av_opt_set_int(swrContext, "linear_interp", 1, 0);
                av_opt_set_int(swrContext, "exact_rational", 1, 0);
            }

            const int swrRet = swr_init(swrContext);
            if (swrRet < 0) {
                qDebug() << "swrContext fail" << swrRet;
                cleanupFFmpeg();
                return nullptr;
            }
        }
    }

    auto *res = new QJsonObject();
    res->insert("path", filePath);
    res->insert("has_audio", hasAudioStream());
    res->insert("has_video", hasVideoStream());

    if (hasAudioStream()) {
        res->insert("bit_rate", QString::number(codecContext->bit_rate));
        res->insert("sample_fmt", codecContext->sample_fmt);
        res->insert("channels", QString::number(codecContext->ch_layout.nb_channels));
        res->insert("sample_rate", QString::number(codecContext->sample_rate));
        res->insert("codec", codec->name);
    }

    if (hasVideoStream()) {
        res->insert("video_width", videoWidth);
        res->insert("video_height", videoHeight);
        res->insert("video_codec", videoCodec->name);
    }

    return res;
}


/**
 * @brief 启动播放
 * 1. 重置音频和视频的解码状态（seek到0，清空buffer）
 * 2. 启动音频解码线程和视频解码线程
 * 3. 两个线程独立运行，互不阻塞
 */
void VideoDecoder::startPlay(){
    QMutexLocker locker(&mutex);

    // 重置文件指针到开始位置（音频，纯视频文件跳过）
    if (hasAudioStream()) {
        av_seek_frame(formatContext, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

        if (codecContext) {
            avcodec_flush_buffers(codecContext);
        }

        if (swrContext) {
            uint8_t* flushBuffer = nullptr;
            int flushSize = SAMPLES_PER_CHANNEL * 2 * 4;  // samples × channels × bytes_per_sample (float)
            flushBuffer = (uint8_t*)av_malloc(flushSize);
            if (flushBuffer) {
                swr_convert(swrContext, &flushBuffer, SAMPLES_PER_CHANNEL, nullptr, 0);
                av_freep(&flushBuffer);
            }
        }
    }
    // 重置文件指针到开始位置（视频）
    if (formatContextVideo && videoStreamIndex >= 0) {
        av_seek_frame(formatContextVideo, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
        if (videoCodecContext) {
            avcodec_flush_buffers(videoCodecContext);
        }
    }

    isPlaying.store(true);
    // 启动音频线程（纯视频文件无音频轨时不启动）
    if (hasAudioStream() && !audioRunning.load()) {
        if (audioThread.joinable()) audioThread.join();
        audioRunning.store(true);
        audioThread = std::thread([this]() { audioLoop(); });
    }
    // 启动视频线程
    if (!videoRunning.load() && hasVideoStream()) {
        if (videoThread.joinable()) videoThread.join();
        videoRunning.store(true);
        videoThread = std::thread([this]() { videoLoop(); });
    }
}

/**
 * @brief 停止播放
 * 1. 设置停止标志，唤醒等待条件
 * 2. 等待音频和视频线程结束 (join)
 * 3. 清理所有缓冲区和重置解码器状态
 */
void VideoDecoder::stopPlay() {
    {
        QMutexLocker locker(&mutex);
        if (!isPlaying.load() && !audioRunning.load() && !videoRunning.load()) {
            return;
        }
        isPlaying.store(false);
        condition.wakeAll();
    }

    // 停止音频线程
    if (audioRunning.load()) {
        audioRunning.store(false);
        if (audioThread.joinable()) audioThread.join();
    }
    // 停止视频线程
    if (videoRunning.load()) {
        videoRunning.store(false);
        if (videoThread.joinable()) videoThread.join();
    }

    // 线程停止后清理：丢弃未上传的 pending，重置上传调度标志
    {
        QMutexLocker locker(&mutex);

        pendingVideoFrame_.release();
        pendingVideoTimestamp_ = -1;
        videoUploadScheduled_.store(false);

        // 先停用所有音频缓冲区
        for (auto& pair : channelAudioBuffers) {
            if (pair.second) {
                pair.second->setActive(false);
            }
        }
        
        // 清空所有通道的音频缓冲区队列
        for (auto& pair : channelAudioBuffers) {
            if (pair.second) {
                pair.second->clear();
            }
        }
        
        // 刷新重采样器缓冲区（但不处理输出）
        if (swrContext) {
            uint8_t* flushBuffer = nullptr;
            int flushSize = 2048 * 2 * 4;
            flushBuffer = (uint8_t*)av_malloc(flushSize);
            if (flushBuffer) {
                // 刷新但丢弃输出，避免产生额外音频
                swr_convert(swrContext, &flushBuffer, 2048, nullptr, 0);
                av_freep(&flushBuffer);
            }
        }
        
        // 重置文件指针到开始位置（仅在有音频轨时）
        if (hasAudioStream()) {
            av_seek_frame(formatContext, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
            if (codecContext) {
                avcodec_flush_buffers(codecContext);
            }
        }
    }
    
    // 重新激活所有缓冲区（为下次播放做准备）
    {
        QMutexLocker locker(&mutex);
        for (auto& pair : channelAudioBuffers) {
            if (pair.second) {
                pair.second->setActive(true);
            }
        }
    }
}

void VideoDecoder::setVolume(double vol) {
    QMutexLocker locker(&mutex);
    volume = vol;
}
    

float VideoDecoder::getVolume() const {
    return volume;
}
    

void VideoDecoder::setLooping(bool loop) {
    QMutexLocker locker(&mutex);
    isLooping.store(loop);
}


bool VideoDecoder::getLooping() const {
    return isLooping.load();
}


bool VideoDecoder::getPlaying() const
{
    return isPlaying.load();
}
       

/**
 * @brief 音频帧声道分离并 push 到各 AudioTimestampRingQueue
 *
 * 输入为交错 float PCM（audioLoop emit）；此处拆为单声道 planar 块，
 * 每声道独立 ring buffer，供 outData(port) 的 AudioData 共享读取。
 */
void VideoDecoder::handleAudioFrame(AudioFrame frame) {

    int bytesPerSample = frame.bitsPerSample / 8;
    int samplesPerChannel = frame.data.size() / (bytesPerSample * frame.channels);

    for (int channel = 0; channel < frame.channels; channel++) {
        if (!channelAudioBuffers[channel])
            channelAudioBuffers[channel] = std::make_shared<AudioTimestampRingQueue>();

        AudioFrame channelFrame;
        channelFrame.sampleRate = frame.sampleRate;
        channelFrame.channels = 1;
        channelFrame.bitsPerSample = frame.bitsPerSample;
        channelFrame.timestamp = frame.timestamp;

        QByteArray channelData;
        channelData.resize(samplesPerChannel * bytesPerSample);

        const float* inputData = reinterpret_cast<const float*>(frame.data.constData());
        float* outputData = reinterpret_cast<float*>(channelData.data());

        const float* src = inputData + channel;
        float* dst = outputData;

        // Optimization for interleaved to planar conversion
        int unrolledSamples = (samplesPerChannel / 8) * 8;
        for (int sample = 0; sample < unrolledSamples; sample += 8) {
            dst[0] = src[0];
            dst[1] = src[frame.channels];
            dst[2] = src[frame.channels * 2];
            dst[3] = src[frame.channels * 3];
            dst[4] = src[frame.channels * 4];
            dst[5] = src[frame.channels * 5];
            dst[6] = src[frame.channels * 6];
            dst[7] = src[frame.channels * 7];

            src += frame.channels * 8;
            dst += 8;
        }

        for (int sample = unrolledSamples; sample < samplesPerChannel; sample++) {
            outputData[sample] = inputData[sample * frame.channels + channel];
        }
        channelFrame.data = channelData;
        channelAudioBuffers[channel]->pushFrame(channelFrame);
    }
}


std::shared_ptr<AudioTimestampRingQueue> VideoDecoder::getAudioBuffer(int index)
{
    if (!channelAudioBuffers[index])
        channelAudioBuffers[index] = std::make_shared<AudioTimestampRingQueue>();
    return channelAudioBuffers[index];
}

// ═══════════════════════════════════════════════════════════════════════════
// 视频后处理：sws BGRA 双缓冲 + GUI 线程 GPU 上传
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief 释放 sws 上下文与 decodeFrame_ CPU 缓冲
 *
 * 分辨率变化、换文件或 cleanupFFmpeg 时调用；pendingVideoFrame_ 由 stopPlay 单独清理。
 */
void VideoDecoder::releaseVideoScaler()
{
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    decodeFrame_.release();
}

/**
 * @brief 确保 swsContext 与 decodeFrame_ 与当前视频尺寸匹配
 *
 * @param width  视频帧宽（来自 videoCodecContext）
 * @param height 视频帧高
 *
 * 尺寸不变但 decodeFrame_ 因 swap 为空时，仅 recreate Mat，不重建 swsContext。
 */
void VideoDecoder::ensureVideoDecodeBuffer(int width, int height)
{
    if (width <= 0 || height <= 0 || !videoCodecContext) {
        return;
    }

    const bool sizeChanged = videoWidth != width || videoHeight != height || !swsContext;
    if (sizeChanged) {
        if (swsContext) {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }
        videoWidth = width;
        videoHeight = height;

        swsContext = sws_getContext(width, height,
                                    videoCodecContext->pix_fmt,
                                    width, height,
                                    AV_PIX_FMT_BGRA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext) {
            qDebug() << "Could not initialize swsContext";
            videoWidth = 0;
            videoHeight = 0;
            decodeFrame_.release();
            return;
        }
    }

    if (decodeFrame_.empty() || decodeFrame_.cols != width || decodeFrame_.rows != height ||
        decodeFrame_.type() != CV_8UC4) {
        decodeFrame_.create(height, width, CV_8UC4);
    }
}

void VideoDecoder::setVideoImageBuffer(std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue> buffer)
{
    QMutexLocker locker(&mutex);
    videoImageBuffer_ = std::move(buffer);
}

/**
 * @brief 向 GUI 事件队列投递一次 uploadPendingVideoFrame
 *
 * videoUploadScheduled_ 保证同一时刻至多一个上传任务在队列中；
 * 解码快于上传时，pending 会被新帧覆盖（swap），上传完成后 hasPending 检查会补调度。
 */
void VideoDecoder::scheduleVideoUploadIfNeeded()
{
    if (!videoUploadScheduled_.exchange(true)) {
        QMetaObject::invokeMethod(this, "uploadPendingVideoFrame", Qt::QueuedConnection);
    }
}

/**
 * @brief 【GUI 线程槽】BGRA Mat → GPU 纹理 → ImageTimestampRingQueue
 *
 * 1. move 取走 pendingVideoFrame_（释放 mutex 后做 GL 上传，缩短持锁时间）
 * 2. ImageFrame::fromBgra8Mat 走 uploadBgra8 快路径，并保留 CPU 副本供下游 OpenCV 读 frame.image
 * 3. pushFrame 覆盖 ring 最旧槽时，旧纹理经 ImageGpuUpload::destroyTexture 释放
 */
void VideoDecoder::uploadPendingVideoFrame()
{
    videoUploadScheduled_.store(false);

    cv::Mat mat;
    qint64 timestamp = -1;
    std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue> buffer;
    {
        QMutexLocker locker(&mutex);
        mat = std::move(pendingVideoFrame_);
        timestamp = pendingVideoTimestamp_;
        pendingVideoTimestamp_ = -1;
        buffer = videoImageBuffer_;
    }

    if (!buffer || mat.empty() || timestamp < 0) {
        return;
    }

    NodeDataTypes::ImageFrame frame = NodeDataTypes::ImageFrame::fromBgra8Mat(
        std::move(mat), timestamp);
    if (!frame.texture.valid()) {
        return;
    }

    buffer->pushFrame(std::move(frame));

    bool hasPending = false;
    {
        QMutexLocker locker(&mutex);
        hasPending = !pendingVideoFrame_.empty();
    }
    if (hasPending) {
        scheduleVideoUploadIfNeeded();
    }
}

/**
 * @brief 【videoLoop 线程】解码后单帧处理
 *
 * 流程：
 *   sws_scale(YUV → BGRA) 写入 decodeFrame_
 *   → swap(decodeFrame_, pendingVideoFrame_)  零拷贝交给上传侧
 *   → scheduleVideoUploadIfNeeded()           异步触发 GUI 上传
 *
 * timestamp 取 TimestampGenerator 当前帧号，供下游 WindowDisplay 等按 tick 对齐。
 */
void VideoDecoder::processVideoFrame() {
    if (!videoFrame || !videoCodecContext) return;

    const int frameW = videoCodecContext->width;
    const int frameH = videoCodecContext->height;
    if (frameW <= 0 || frameH <= 0) {
        return;
    }

    ensureVideoDecodeBuffer(frameW, frameH);
    if (!swsContext || decodeFrame_.empty()) {
        return;
    }

    // 将 decodeFrame_ 内存布局告知 FFmpeg，作为 sws_scale 输出目标
    uint8_t* dstData[4] = {nullptr};
    int dstLinesize[4] = {0};
    if (av_image_fill_arrays(dstData, dstLinesize, decodeFrame_.data,
                             AV_PIX_FMT_BGRA, frameW, frameH, 1) < 0) {
        return;
    }

    sws_scale(swsContext, videoFrame->data, videoFrame->linesize, 0,
              frameH, dstData, dstLinesize);

    const qint64 timestamp = static_cast<qint64>(timestampGenerator_->getCurrentFrameCount());
    {
        QMutexLocker locker(&mutex);
        if (!videoImageBuffer_) {
            return;
        }
        // 双缓冲交换：decodeFrame_ 交给 pending，原 pending（可能为空或旧缓冲）成为新 decode 目标
        std::swap(decodeFrame_, pendingVideoFrame_);
        pendingVideoTimestamp_ = timestamp;
    }

    scheduleVideoUploadIfNeeded();
}

/**
 * @brief 兼容旧接口：保持空实现或仅用于调试
 */
void VideoDecoder::run()  {
}


/**
 * @brief 启动播放（向后兼容），等同于 startPlay()
 */
void VideoDecoder::playAudio() {
    startPlay();
}

/**
 * @brief 音频解码主循环（std::thread / audioThread）
 *
 * 职责：
 *   1. 从 formatContext 读音频包并解码
 *   2. 按需 swr 重采样至 48000 Hz / FLT
 *   3. processPcmAndEmitFixedFrames 切片为 SAMPLES_PER_CHANNEL 块
 *   4. emit playbackProgress（约 100ms 节流）供 UI 进度条
 *   5. 循环模式下 EOF 后 seek 回起点
 *
 * 自然结束时 emit playbackFinished，并停止 videoRunning。
 */
void VideoDecoder::audioLoop() {
    if (!hasAudioStream()) {
        audioRunning.store(false);
        return;
    }

    AVPacket packet;
    audioFrame = av_frame_alloc();
    uint8_t* outputBuffer = nullptr;
    int outputBufferSize = 0;
    lastTimestamp_=timestampGenerator_->getCurrentFrameCount();
    // 获取原始音频参数
    int originalSampleRate = codecContext->sample_rate;
    int originalChannels = codecContext->ch_layout.nb_channels;

    // 检查是否需要重采样
    bool needsResampling = (originalSampleRate != SAMPLE_RATE) || (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT);

    // 预缓冲控制变量
    int frameCount = 0;
    double lastEmitTime = -1.0;

    // 循环播放的主循环
    do {
        // 重置文件指针到开始位置（用于循环播放）
        if (frameCount > 0 && isLooping.load()) {
            QThread::msleep(LOOP_INTERVAL);
            av_seek_frame(formatContext, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
            if (codecContext) {
                avcodec_flush_buffers(codecContext);
            }

            // 重置待发缓冲
            pendingInterleavedPcm_.clear();
            pendingSamplesPerChannel_ = 0;
            lastChannels_ = 0;
        }

        while (audioRunning.load() && isPlaying.load() && av_read_frame(formatContext, &packet) >= 0) {
            // 发送播放进度信号
            if (packet.stream_index == audioStreamIndex) {
                 double currentSec = 0.0;
                 if (packet.pts != AV_NOPTS_VALUE) {
                     currentSec = packet.pts * av_q2d(formatContext->streams[packet.stream_index]->time_base);
                 }
                 
                 double totalSec = 0.0;
                 if (formatContext->duration != AV_NOPTS_VALUE) {
                     totalSec = formatContext->duration / (double)AV_TIME_BASE;
                 }
                 
                 // 限制发送频率，避免UI刷新过快 (例如每0.1秒刷新一次)
                 if (std::abs(currentSec - lastEmitTime) >= 0.1) {
                     emit playbackProgress(currentSec, totalSec);
                     lastEmitTime = currentSec;
                 }
            }

            if (packet.stream_index == audioStreamIndex) {
                if (avcodec_send_packet(codecContext, &packet) < 0) {
                    continue;
                }

                while (avcodec_receive_frame(codecContext, audioFrame) >= 0) {
                    // qDebug()<<audioFrame->nb_samples;
                    if (needsResampling && swrContext) {
                        // ==================== 重采样输出采样数计算 ====================

                        // 获取重采样器内部延迟的采样数（以原始采样率为单位）
                        int64_t delay = swr_get_delay(swrContext, originalSampleRate);

                        // 添加延迟合理性检查，避免异常延迟值
                        if (delay < 0 || delay > audioFrame->nb_samples * 4) {
                            delay = 0;  // 重置异常延迟
                            qDebug() << "Warning: Abnormal resampler delay detected, reset to 0";
                        }

                        // 计算重采样后的输出采样数
                        int64_t outputSamples = av_rescale_rnd(
                            delay + audioFrame->nb_samples,
                            SAMPLE_RATE,
                            originalSampleRate,
                            AV_ROUND_UP
                        );

                        // 更严格的安全检查：限制输出采样数为合理范围
                        int64_t maxOutputSamples = (audioFrame->nb_samples * SAMPLE_RATE / originalSampleRate) + 64;
                        if (outputSamples > maxOutputSamples) {
                            outputSamples = maxOutputSamples;
                            // qDebug() << "Warning: Output samples clamped from" << outputSamples << "to" << maxOutputSamples;
                        }

                        // 确保最小输出采样数
                        if (outputSamples < audioFrame->nb_samples / 2) {
                            outputSamples = audioFrame->nb_samples;
                        }

                        // ==================== 输出缓冲区大小计算 ====================

                        int outputChannels = originalChannels;
                        int newSize = outputSamples * outputChannels * 4; // float is 4 bytes

                        // 动态调整输出缓冲区大小
                        if (outputBufferSize < newSize) {
                            av_freep(&outputBuffer);
                            outputBufferSize = newSize;
                            outputBuffer = (uint8_t*)av_mallocz(outputBufferSize);
                            if (!outputBuffer) {
                                qDebug() << "Failed to allocate output buffer";
                                break;
                            }
                        }

                        // 清零缓冲区
                        memset(outputBuffer, 0, newSize);

                        // ==================== 执行重采样转换 ====================

                        int samplesResampled = swr_convert(
                            swrContext,
                            &outputBuffer,
                            outputSamples,
                            (const uint8_t**)audioFrame->data,
                            audioFrame->nb_samples
                        );

                        if (samplesResampled < 0) {
                            qDebug() << "swr_convert failed:" << samplesResampled;
                            continue;
                        }

                        if (samplesResampled > 0) {
                            // 应用音量控制
                            applyVolume(outputBuffer, samplesResampled, outputChannels);
                            // 将数据切片为1920*channels并发送
                            frameCount += processPcmAndEmitFixedFrames(outputBuffer,
                                                                       samplesResampled,
                                                                       outputChannels,
                                                                       SAMPLE_RATE);
                        }
                    }
                    else {
                        // 不需要重采样的情况（48000Hz且已经是FLT格式）
                        if (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT) {
                            qDebug() << "Warning: Expected FLT format but got" << codecContext->sample_fmt;
                            continue;
                        }
                        // 直接使用原始数据
                        applyVolume(audioFrame->data[0], audioFrame->nb_samples, originalChannels);
                        frameCount += processPcmAndEmitFixedFrames(audioFrame->data[0],
                                                                   audioFrame->nb_samples,
                                                                   originalChannels,
                                                                   SAMPLE_RATE);
                    }
                }
            }
            av_packet_unref(&packet);
        }
    } while (audioRunning.load() && isPlaying.load() && isLooping.load()); // 循环播放条件

    // 清理资源
    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
    if (outputBuffer) {
        av_freep(&outputBuffer);
    }
    const bool finishedNaturally = audioRunning.load() && isPlaying.load() && !isLooping.load();
    audioRunning.store(false);
    if (finishedNaturally) {
        isPlaying.store(false);
        videoRunning.store(false);
        Q_EMIT playbackFinished();
    }
}

/**
 * @brief 视频解码主循环（std::thread / videoThread）
 *
 * 职责：
 *   1. 从 formatContextVideo 独立读包，与 audioLoop 互不阻塞
 *   2. avcodec 解码得到 YUV AVFrame
 *   3. 按 PTS 与 QElapsedTimer 做 sleep，控制输出帧率接近源视频
 *   4. 调用 processVideoFrame 完成 BGRA 转换并投递 GUI 上传
 *
 * EOF 行为：isLooping 为 true 则 seek 回起点；否则退出循环。
 */
void VideoDecoder::videoLoop() {
    AVPacket packet;
    videoFrame = av_frame_alloc();
    QElapsedTimer timer;
    bool started = false;
    double firstPtsSec = 0.0;
    double lastEmitTime = -1.0;
    // 主循环：按 PTS 节奏输出，避免解码速度远快于实时导致 pending/GPU 积压
    while (videoRunning.load() && isPlaying.load()) {
        int ret = av_read_frame(formatContextVideo, &packet);
        if (ret < 0) {
            if (isLooping.load()) {
                av_seek_frame(formatContextVideo, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
                if (videoCodecContext) {
                    avcodec_flush_buffers(videoCodecContext);
                }
                started = false;
                continue;
            }
            break;
        }

        if (packet.stream_index == videoStreamIndex) {
            if (avcodec_send_packet(videoCodecContext, &packet) == 0) {
                while (videoRunning.load() && isPlaying.load() && avcodec_receive_frame(videoCodecContext, videoFrame) >= 0) {
                    int64_t ts = (videoFrame->best_effort_timestamp != AV_NOPTS_VALUE) ? videoFrame->best_effort_timestamp : videoFrame->pts;
                    if (ts != AV_NOPTS_VALUE) {
                        const AVRational tb = formatContextVideo->streams[videoStreamIndex]->time_base;
                        const double ptsSec = ts * av_q2d(tb);
                        if (!started) {
                            started = true;
                            firstPtsSec = ptsSec;
                            timer.start();
                        } else {
                            // 相对首帧 PTS 计算目标时刻，sleep 追赶播放节奏
                            const qint64 targetMs = static_cast<qint64>((ptsSec - firstPtsSec) * 1000.0);
                            const qint64 nowMs = timer.elapsed();
                            const qint64 sleepMs = targetMs - nowMs;
                            if (sleepMs > 0 && sleepMs < 1000) {
                                QThread::msleep(static_cast<unsigned long>(sleepMs));
                            }
                        }

                        // 纯视频文件：由视频 PTS 驱动进度条（有音频时由 audioLoop 负责）
                        if (!hasAudioStream()) {
                            double totalSec = 0.0;
                            if (formatContextVideo->duration != AV_NOPTS_VALUE) {
                                totalSec = formatContextVideo->duration / static_cast<double>(AV_TIME_BASE);
                            }
                            if (std::abs(ptsSec - lastEmitTime) >= 0.1) {
                                emit playbackProgress(ptsSec, totalSec);
                                lastEmitTime = ptsSec;
                            }
                        }
                    }
                    processVideoFrame();
                }
            }
        }
        av_packet_unref(&packet);
    }
    if (videoFrame) {
        av_frame_free(&videoFrame);
    }
    const bool finishedNaturally = videoRunning.load() && isPlaying.load() && !isLooping.load();
    videoRunning.store(false);
    // 纯视频：播放结束时由 videoLoop 发出 finished（有音频时由 audioLoop 负责）
    if (finishedNaturally && !hasAudioStream()) {
        isPlaying.store(false);
        Q_EMIT playbackFinished();
    }
}

/**
 * @brief 清理FFmpeg资源
 * 释放所有分配的上下文、帧和缓冲区
 */
void VideoDecoder::cleanupFFmpeg(){
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    releaseVideoScaler();
    if (videoCodecContext) {
        avcodec_free_context(&videoCodecContext);
        videoCodecContext = nullptr;
    }
    videoCodec = nullptr;
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    if (formatContextVideo) {
        avformat_close_input(&formatContextVideo);
        formatContextVideo = nullptr;
    }
    if (audioFrame) {
        av_frame_free(&audioFrame);
        audioFrame = nullptr;
    }
    if (videoFrame) {
        av_frame_free(&videoFrame);
        videoFrame = nullptr;
    }
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (resampledBuffer) {
        av_free(resampledBuffer);
        resampledBuffer = nullptr;
    }
    codec = nullptr;
    audioStreamIndex = -1;
    videoStreamIndex = -1;
    videoWidth = 0;
    videoHeight = 0;
}

/**
 * @brief 应用音量调整到音频数据（volume为分贝值）
 * @param data 音频数据指针
 * @param sampleCount 采样点数量
 * @param channels 声道数
 */
void VideoDecoder::applyVolume(uint8_t* data, int sampleCount, int channels) {

    if (volume == 0.0f) return;
    // 将分贝值转换为线性增益：gain = 10^(dB/20)
    float linearGain;
    if (volume <= -40.0f) {
        linearGain = 0.0f; // 静音处理
    } else {
        linearGain = std::pow(10.0f, volume / 20.0f);
    }
    float* samples = reinterpret_cast<float*>(data);
    int totalSamples = sampleCount * channels;

    for (int i = 0; i < totalSamples; i++) {
        // 应用线性增益并进行削波保护
        samples[i] *= linearGain;
        
        // 简单削波保护，防止溢出 [-1.0, 1.0]
        if (samples[i] > 1.0f) {
            samples[i] = 1.0f;
        } else if (samples[i] < -1.0f) {
            samples[i] = -1.0f;
        }
    }
}



/**
 * @brief 处理PCM数据并按固定帧大小发射
 * 
 * 将解码后的PCM数据累积到缓冲区，当达到 SAMPLES_PER_CHANNEL 时切片发送。
 * 包含简单的播放速度控制（通过 sleep）。
 * 
 * @param interleavedPcm 交错的PCM数据指针
 * @param samplesPerChannel 本次输入的每通道采样数
 * @param channels 通道数
 * @param sampleRate 采样率
 * @return 已发射的帧数
 */
int VideoDecoder::processPcmAndEmitFixedFrames(const uint8_t* interleavedPcm,
                                               int samplesPerChannel,
                                               int channels,
                                               int sampleRate)
{
    const int bytesPerSample = 4; // Float
    const int targetSamplesPerChannel = SAMPLES_PER_CHANNEL;
    const int chunkBytes = targetSamplesPerChannel * channels * bytesPerSample;
    const int inputBytes = samplesPerChannel * channels * bytesPerSample;

    // 如果声道变化，重置累积状态
    if (lastChannels_ != 0 && lastChannels_ != channels) {
        pendingInterleavedPcm_.clear();
        pendingSamplesPerChannel_ = 0;
    }
    lastChannels_ = channels;

    // 追加新数据
    pendingInterleavedPcm_.append(reinterpret_cast<const char*>(interleavedPcm), inputBytes);
    pendingSamplesPerChannel_ += samplesPerChannel;

    int emitted = 0;
    double chunkMs = 1000.0 * targetSamplesPerChannel / static_cast<double>(sampleRate);

    while (pendingSamplesPerChannel_ >= targetSamplesPerChannel) {
        QByteArray chunk = pendingInterleavedPcm_.left(chunkBytes);
        pendingInterleavedPcm_.remove(0, chunkBytes);
        pendingSamplesPerChannel_ -= targetSamplesPerChannel;

        AudioFrame frame;
        frame.data = chunk;
        frame.sampleRate = sampleRate;
        frame.channels = channels;
        frame.bitsPerSample = 32;
        frame.timestamp = ++lastTimestamp_+FIXED_DELAY_FRAMES;

        emit audioFrameReady(frame);
        emitted++;

        // 播放速度控制：每个固定块按其持续时间节拍
        if (chunkMs > 0 && chunkMs < 1000.0) {
            // qDebug()<<"frame.timestamp"<<frame.timestamp<<chunkMs;
            qint64 currentSystemTimestamp = timestampGenerator_->getCurrentFrameCount();
            
            // 计算时间戳差异（生成帧时间戳 - 当前系统时间戳）
            qint64 timestampDiff = frame.timestamp - currentSystemTimestamp;
            if (timestampDiff<FIXED_DELAY_FRAMES+1)
            {
                chunkMs=chunkMs*0.5;
            }
            if(timestampDiff>=8)
            {
                chunkMs=chunkMs*1.5;
            }

            QThread::msleep(static_cast<unsigned long>(chunkMs));
        }
    }

    return emitted;
}

