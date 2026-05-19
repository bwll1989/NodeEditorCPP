#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
#include "DataTypes/AudioData.h"  // 确保包含此头文件
#include "DataTypes/ImageData.h"
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
static const int SAMPLE_RATE = 48000;
static const int LOOP_INTERVAL = 800;
static const int FIXED_DELAY_FRAMES = 5;
static const int SAMPLES_PER_CHANNEL = SAMPLE_RATE/TimestampGenerator::getInstance()->getFrameRate();;
// 在构造函数中添加新的成员变量初始化
/**
 * @brief 构造函数
 * 初始化成员变量，注册元数据类型，连接信号槽
 * @param parent 父对象指针
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
    // 初始化视频目标数据指针
    for (int i = 0; i < 4; i++) {
        videoDstData[i] = nullptr;
        videoDstLinesize[i] = 0;
    }

    qRegisterMetaType<AudioFrame>("AudioFrame");
    qRegisterMetaType<NodeDataTypes::ImageData>("NodeDataTypes::ImageData");
    connect(this, &VideoDecoder::audioFrameReady,
            this, &VideoDecoder::handleAudioFrame,
            Qt::DirectConnection);
}

VideoDecoder::~VideoDecoder() {
    stopPlay();
    cleanupFFmpeg();
}

/**
 * @brief 初始化音频与视频的FFmpeg上下文
 * 
 * 为了实现音视频独立解码互不阻塞，这里会分别打开两次文件：
 * 1. formatContext: 用于音频解码
 * 2. formatContextVideo: 用于视频解码
 * 
 * @param filePath 媒体文件路径
 * @return 包含媒体信息的JSON对象
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
    // 打开音频上下文
    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug()<<"打开文件失败"<<filePath.toStdString().c_str();
        return nullptr;
    }
    // 打开视频上下文（独立）
    if (avformat_open_input(&formatContextVideo, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug()<<"打开文件失败(视频)"<<filePath.toStdString().c_str();
        return nullptr;
    }
    //读取格式信息

    if (avformat_find_stream_info(formatContext, nullptr) != 0) {
        qDebug()<<"找不到流信息";
        return nullptr;
    }
    if (avformat_find_stream_info(formatContextVideo, nullptr) != 0) {
        qDebug()<<"找不到视频流信息";
        return nullptr;
    }
    
    audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex==-1) {
        return nullptr;
    }

    codec = avcodec_find_decoder(formatContext->streams[audioStreamIndex]->codecpar->codec_id);//获取codec

    if (!codec) {
        // 找不到解码器
        return nullptr;
    }
    //查找解码器

    codecContext= avcodec_alloc_context3(codec);
    int ret=avcodec_parameters_to_context(codecContext, formatContext->streams[audioStreamIndex]->codecpar);
    if (ret< 0) {
        qDebug()<<"解码器参数设置失败";
        // 解码器参数设置失败
        return nullptr;
    }
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        qDebug()<<"打开解码器失败";
        return nullptr;
    }

        // 初始化视频（独立上下文）
        videoStreamIndex = av_find_best_stream(formatContextVideo, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoStreamIndex != -1) {
            videoCodec = avcodec_find_decoder(formatContextVideo->streams[videoStreamIndex]->codecpar->codec_id);
            if (videoCodec) {
                videoCodecContext = avcodec_alloc_context3(videoCodec);
                if (videoCodecContext) {
                    avcodec_parameters_to_context(videoCodecContext, formatContextVideo->streams[videoStreamIndex]->codecpar);
                    if (avcodec_open2(videoCodecContext, videoCodec, nullptr) >= 0) {
                        videoWidth = videoCodecContext->width;
                        videoHeight = videoCodecContext->height;
                    } else {
                        avcodec_free_context(&videoCodecContext);
                        videoCodecContext = nullptr;
                    }
                }
            }
        }
    
    // 检查是否需要重采样
    bool needsResampling = (codecContext->sample_rate != SAMPLE_RATE) ||
                          (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT);

    // // 只有在需要重采样时才初始化重采样器
    if (needsResampling) {
        // 在initializeFFmpeg中改进通道布局设置
        AVChannelLayout outChannelLayout;
        AVChannelLayout inChannelLayout = codecContext->ch_layout;

        // 保持原始通道数，避免不必要的通道转换
        if (inChannelLayout.nb_channels == 1) {
            outChannelLayout = AV_CHANNEL_LAYOUT_MONO;
        } else if (inChannelLayout.nb_channels == 2) {
            outChannelLayout = AV_CHANNEL_LAYOUT_STEREO;
        } else {
            // 对于多声道音频，保持原始布局
            outChannelLayout = inChannelLayout;
        }

        // 在initializeFFmpeg中改进重采样器配置
        if (swr_alloc_set_opts2(&swrContext,
                                &outChannelLayout,
                                AV_SAMPLE_FMT_FLT,
                                SAMPLE_RATE, // 输出采样率
                                &inChannelLayout,
                                codecContext->sample_fmt,
                                codecContext->sample_rate,
                                0,
                                nullptr)!= 0)
        {
            qDebug()<<"swr_alloc_set_opts2 fail";
            return nullptr;
        }

        // 针对不同采样率优化重采样质量参数
        if (codecContext->sample_rate == 44100) {
            // 44100Hz到48000Hz的特殊优化
            av_opt_set(swrContext, "resampler", "swr", 0);
            av_opt_set_int(swrContext, "filter_size", 64, 0);  // 更大的滤波器
            av_opt_set_int(swrContext, "phase_shift", 12, 0);  // 更高的相位偏移
            av_opt_set_double(swrContext, "cutoff", 0.99, 0);  // 更高的截止频率
            av_opt_set(swrContext, "dither_method", "shibata", 0); // 更好的抖动方法
            av_opt_set_int(swrContext, "linear_interp", 1, 0);
            av_opt_set_int(swrContext, "exact_rational", 1, 0);
        } else {
            // 通用重采样参数
            av_opt_set(swrContext, "resampler", "swr", 0);
            av_opt_set_int(swrContext, "filter_size", 32, 0);
            av_opt_set_int(swrContext, "phase_shift", 10, 0);
            av_opt_set_double(swrContext, "cutoff", 0.98, 0);
            av_opt_set(swrContext, "dither_method", "triangular", 0);
            av_opt_set_int(swrContext, "linear_interp", 1, 0);
            av_opt_set_int(swrContext, "exact_rational", 1, 0);
        }

        ret=swr_init(swrContext);
        if(ret<0)
        {
            qDebug()<<"swrContext fail"<<ret;
            return nullptr;
        }
    }

    auto *res= new QJsonObject();
    res->insert("path",filePath);
    res->insert("bit_rate",QString::number(codecContext->bit_rate));
    res->insert("sample_fmt",codecContext->sample_fmt);
    res->insert("channels",QString::number(codecContext->ch_layout.nb_channels));
    res->insert("sample_rate",QString::number(codecContext->sample_rate));
    res->insert("codec",codec->name);
    res->insert("frame_rate",codec->name);
    
    if (videoStreamIndex != -1) {
        res->insert("has_video", true);
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

    // 重置文件指针到开始位置（音频）
    if (formatContext) {
        // 将音频流定位到起始位置，使用向后搜索标志确保精确定位
        av_seek_frame(formatContext, audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

        // 清空解码器内部缓冲区，移除之前解码但未输出的帧数据
        if (codecContext) {
            avcodec_flush_buffers(codecContext);
        }

        // 清空重采样器内部缓冲区，避免上次播放的残留数据造成杂音
        if (swrContext) {
            uint8_t* flushBuffer = nullptr;
            // 这个大小足够容纳重采样器内部可能残留的数据
            int flushSize = SAMPLES_PER_CHANNEL * 2 * 4;  // samples × channels × bytes_per_sample (float)
            flushBuffer = (uint8_t*)av_malloc(flushSize);
            if (flushBuffer) {
                // 调用swr_convert清空内部缓冲区
                // 输入nullptr和0表示不提供新数据，只是刷新内部缓冲区
                // 输出到临时缓冲区，然后丢弃这些数据
                swr_convert(swrContext, &flushBuffer, SAMPLES_PER_CHANNEL, nullptr, 0);
                av_freep(&flushBuffer);  // 释放临时缓冲区
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
    // 启动音频线程
    if (!audioRunning.load()) {
        if (audioThread.joinable()) audioThread.join();
        audioRunning.store(true);
        audioThread = std::thread([this]() { audioLoop(); });
    }
    // 启动视频线程
    if (!videoRunning.load() && videoCodecContext && formatContextVideo) {
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

    // 线程停止后再清理缓冲区
    {
        QMutexLocker locker(&mutex);
        
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
        
        // 重置文件指针到开始位置
        if (formatContext) {
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

/**
 * @brief 处理视频帧：将解码后的视频帧转换为RGB格式并发送
 */
void VideoDecoder::releaseVideoScaler()
{
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if (videoDstData[0]) {
        av_freep(&videoDstData[0]);
        for (int i = 0; i < 4; ++i) {
            videoDstData[i] = nullptr;
            videoDstLinesize[i] = 0;
        }
    }
}

void VideoDecoder::processVideoFrame() {
    if (!videoFrame || !videoCodecContext) return;

    const int frameW = videoCodecContext->width;
    const int frameH = videoCodecContext->height;
    if (frameW <= 0 || frameH <= 0) {
        return;
    }

    if (!swsContext || videoWidth != frameW || videoHeight != frameH) {
        releaseVideoScaler();
        videoWidth = frameW;
        videoHeight = frameH;

        swsContext = sws_getContext(frameW, frameH,
                                    videoCodecContext->pix_fmt,
                                    frameW, frameH,
                                    AV_PIX_FMT_BGR24,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!swsContext) {
            qDebug() << "Could not initialize swsContext";
            return;
        }

        if (av_image_alloc(videoDstData, videoDstLinesize,
                           frameW, frameH,
                           AV_PIX_FMT_BGR24, 1) < 0) {
            qDebug() << "Could not allocate raw video buffer";
            releaseVideoScaler();
            return;
        }
    }

    sws_scale(swsContext, videoFrame->data, videoFrame->linesize, 0,
              videoCodecContext->height, videoDstData, videoDstLinesize);

    cv::Mat img(videoCodecContext->height, videoCodecContext->width, CV_8UC3,
                videoDstData[0], videoDstLinesize[0]);
    
    // 使用clone确保数据被复制，避免缓冲区重用导致的问题
    NodeDataTypes::ImageData imageData(img.clone());
    emit videoFrameReady(imageData);
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
 * @brief 音频解码主循环
 * 运行在独立线程中，负责：
 * 1. 读取音频包并解码
 * 2. 进行音频重采样（如需）
 * 3. 将PCM数据切片为固定大小的块（SAMPLES_PER_CHANNEL）
 * 4. 控制音频播放进度信号
 */
void VideoDecoder::audioLoop() {
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
                            qDebug() << "Warning: Output samples clamped from" << outputSamples << "to" << maxOutputSamples;
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
 * @brief 视频解码主循环
 * 运行在独立线程中，负责：
 * 1. 使用独立的 AVFormatContext 读取视频包
 * 2. 解码视频帧
 * 3. 根据 PTS (Presentation Time Stamp) 进行帧同步，使用 sleep 控制播放速度
 * 4. 转换 YUV 到 BGR 并发送图像信号
 */
void VideoDecoder::videoLoop() {
    AVPacket packet;
    videoFrame = av_frame_alloc();
    QElapsedTimer timer;
    bool started = false;
    double firstPtsSec = 0.0;
    // 主循环（按PTS节奏输出，避免UI被淹没）
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
                            const qint64 targetMs = static_cast<qint64>((ptsSec - firstPtsSec) * 1000.0);
                            const qint64 nowMs = timer.elapsed();
                            const qint64 sleepMs = targetMs - nowMs;
                            if (sleepMs > 0 && sleepMs < 1000) {
                                QThread::msleep(static_cast<unsigned long>(sleepMs));
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
    videoRunning.store(false);
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

