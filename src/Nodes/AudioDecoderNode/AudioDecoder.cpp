#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
#include "Common/DataTypes/AudioData.h"  // 确保包含此头文件
#include "AudioDecoder.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}
#include <iostream>
#include <cmath>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <portaudio.h>
#include <QDateTime>
// #include <Common/Devices/AudioPipe/AudioPipe.h>
static const int SAMPLE_RATE = 48000;
static const int LOOP_INTERVAL = 800;
static const int FIXED_DELAY_FRAMES = 5;

/**
 * @brief 根据当前全局时间戳帧率获取每帧采样数
 * @return 每帧采样数
 */
static int samplesPerChannelForTimestampFrame()
{
    return TimestampGenerator::getInstance()->getSamplesPerFrame(SAMPLE_RATE);
}
// 在构造函数中添加新的成员变量初始化
AudioDecoder::AudioDecoder(QObject *parent)
    : QThread(parent)
    , formatContext(nullptr)
    , codecContext(nullptr)
    , codec(nullptr)
    , audioFrame(nullptr)
    , packet(nullptr)
    , swrContext(nullptr)
    , resampledBuffer(nullptr)
    , audioStreamIndex(-1)
    , isPlaying(false)
    , isLooping(false)
    , volume(0.5f)
    , timestampGenerator_(TimestampGenerator::getInstance())  // 获取全局时间戳生成器实例
    , m_durationSec(0.0)
    , m_startPositionSec(0.0)
    , m_pendingSeekSec(-1.0)
{
    qRegisterMetaType<AudioFrame>("AudioFrame");
    connect(this, &AudioDecoder::audioFrameReady,
            this, &AudioDecoder::handleAudioFrame,
            Qt::DirectConnection);
}

AudioDecoder::~AudioDecoder() {
    stopPlay();
    cleanupFFmpeg();
}

QJsonObject* AudioDecoder::initializeFFmpeg(const QString &filePath){
    // 切换曲目 / 重复初始化前先释放旧上下文，避免泄漏与悬空指针
    cleanupFFmpeg();
    formatContext = nullptr;
    codecContext = nullptr;
    codec = nullptr;
    swrContext = nullptr;
    audioStreamIndex = -1;
    m_durationSec = 0.0;
    avformat_network_init();
    if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
        qDebug()<<"打开文件失败"<<filePath.toStdString().c_str();
        return nullptr;
    }
    //打开文件，并读取格式信息到格式上下文formatContext

    if (avformat_find_stream_info(formatContext, nullptr) != 0) {
        qDebug()<<"找不到流信息";
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
    // 精确时长（秒）：优先使用 AVStream.duration / AVStream.time_base，避免 duration_estimation 可能的空值
    double totalSec = 0.0;
    AVStream* st = formatContext->streams[audioStreamIndex];
    if (st != nullptr && st->duration != AV_NOPTS_VALUE && st->time_base.num > 0 && st->time_base.den > 0) {
        totalSec = static_cast<double>(st->duration) * static_cast<double>(st->time_base.num) / static_cast<double>(st->time_base.den);
    }
    if (totalSec <= 0.0 && formatContext->duration != AV_NOPTS_VALUE) {
        // 回退：容器级 duration（AV_TIME_BASE = 1e-6）
        totalSec = static_cast<double>(formatContext->duration) / static_cast<double>(AV_TIME_BASE);
    }
    if (totalSec < 0.0) totalSec = 0.0;
    res->insert("duration", totalSec);
    m_durationSec = totalSec;
    m_startPositionSec = 0.0;
    m_pendingSeekSec = -1.0;
    return res;
}

bool AudioDecoder::seekFileToSec(double sec)
{
    if (!formatContext || audioStreamIndex < 0) {
        return false;
    }
    if (sec < 0.0) {
        sec = 0.0;
    }
    if (m_durationSec > 0.0 && sec > m_durationSec) {
        sec = m_durationSec;
    }

    AVStream* st = formatContext->streams[audioStreamIndex];
    int64_t timestamp = 0;
    if (st && st->time_base.num > 0 && st->time_base.den > 0) {
        timestamp = static_cast<int64_t>(sec / av_q2d(st->time_base) + 0.5);
    } else {
        timestamp = static_cast<int64_t>(sec * AV_TIME_BASE);
    }

    const int seekRet = av_seek_frame(formatContext, audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
    if (seekRet < 0) {
        // 回退：按时间戳全局 seek
        const int64_t tsGlobal = static_cast<int64_t>(sec * AV_TIME_BASE);
        av_seek_frame(formatContext, -1, tsGlobal, AVSEEK_FLAG_BACKWARD);
    }

    if (codecContext) {
        avcodec_flush_buffers(codecContext);
    }

    if (swrContext) {
        uint8_t* flushBuffer = nullptr;
        const int samplesPerChannel = samplesPerChannelForTimestampFrame();
        const int flushSize = samplesPerChannel * 2 * 4;
        flushBuffer = static_cast<uint8_t*>(av_malloc(flushSize));
        if (flushBuffer) {
            swr_convert(swrContext, &flushBuffer, samplesPerChannel, nullptr, 0);
            av_freep(&flushBuffer);
            swr_init(swrContext);
        }
    }

    pendingInterleavedPcm_.clear();
    pendingSamplesPerChannel_ = 0;
    lastChannels_ = 0;
    return true;
}

void AudioDecoder::clearPcmAndChannelBuffers()
{
    for (auto& pair : channelAudioBuffers) {
        if (pair.second) {
            pair.second->setActive(false);
            pair.second->clear();
            pair.second->setActive(true);
        }
    }
    pendingInterleavedPcm_.clear();
    pendingSamplesPerChannel_ = 0;
    lastChannels_ = 0;
}

void AudioDecoder::seekTo(double sec)
{
    QMutexLocker locker(&mutex);
    if (sec < 0.0) {
        sec = 0.0;
    }
    if (m_durationSec > 0.0 && sec > m_durationSec) {
        sec = m_durationSec;
    }
    m_startPositionSec = sec;
    if (isPlaying) {
        // 播放中：交给解码线程尽快执行，避免在主线程 seek 与解码竞态
        m_pendingSeekSec = sec;
    } else if (formatContext) {
        seekFileToSec(sec);
    }
}

double AudioDecoder::startPositionSec() const
{
    return m_startPositionSec;
}

double AudioDecoder::durationSec() const
{
    return m_durationSec;
}

void AudioDecoder::startPlay(){
    QMutexLocker locker(&mutex);

    // 定位到当前起始位置（支持拖拽后从中间开始播）
    if (formatContext) {
        seekFileToSec(m_startPositionSec);
        m_pendingSeekSec = -1.0;
    }

    isPlaying = true;
    // 若线程仍在收尾，先等其退出再启动，避免 QThread::start 失败导致“假播放”
    if (isRunning()) {
        locker.unlock();
        wait(3000);
        locker.relock();
        isPlaying = true;
    }
    start();  // 启动线程开始播放
}

/**
 * @brief 停止音频播放
 * 确保线程安全地停止播放并清理所有缓冲区
 * @param resetPosition 是否将解码起点复位到 0
 */
void AudioDecoder::stopPlay(bool resetPosition) {
    bool wasPlaying = false;
    {
        QMutexLocker locker(&mutex);
        wasPlaying = isPlaying;
        isPlaying = false;
        m_pendingSeekSec = -1.0;
        if (resetPosition) {
            m_startPositionSec = 0.0;
        }
        condition.wakeAll();
    }

    // 等待线程结束（在锁外进行）；EOF 路径下线程可能仍在收尾
    if (wasPlaying && isRunning()) {
        wait(3000); // 等待最多3秒
    } else if (!wasPlaying && isRunning()) {
        // 播放线程已把 isPlaying 置 false 并正在 emit finished：再等一下收尾
        wait(3000);
    }

    // 线程停止后再清理缓冲区
    {
        QMutexLocker locker(&mutex);
        clearPcmAndChannelBuffers();

        if (swrContext) {
            uint8_t* flushBuffer = nullptr;
            int flushSize = 2048 * 2 * 4;
            flushBuffer = (uint8_t*)av_malloc(flushSize);
            if (flushBuffer) {
                swr_convert(swrContext, &flushBuffer, 2048, nullptr, 0);
                av_freep(&flushBuffer);
            }
        }

        if (formatContext) {
            seekFileToSec(resetPosition ? 0.0 : m_startPositionSec);
        }
        if (resetPosition) {
            m_startPositionSec = 0.0;
        }
        m_pendingSeekSec = -1.0;
    }
}

void AudioDecoder::setVolume(double vol) {
    QMutexLocker locker(&mutex);
    volume = vol;
}
    

float AudioDecoder::getVolume() const {
    return volume;
}
    

void AudioDecoder::setLooping(bool loop) {
    QMutexLocker locker(&mutex);
    isLooping = loop;
}


bool AudioDecoder::getLooping() const {
    return isLooping;
}


bool AudioDecoder::getPlaying() const
{
    return isPlaying;
}
       

void AudioDecoder::handleAudioFrame(AudioFrame frame) {

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


std::shared_ptr<AudioTimestampRingQueue> AudioDecoder::getAudioBuffer(int index)
{
    if (!channelAudioBuffers[index])
        channelAudioBuffers[index] = std::make_shared<AudioTimestampRingQueue>();
    return channelAudioBuffers[index];
}

void AudioDecoder::run()  {
    playAudio();
}


void AudioDecoder::playAudio() {
    AVPacket packet;
    audioFrame = av_frame_alloc();
    uint8_t* outputBuffer = nullptr;
    int outputBufferSize = 0;
    lastTimestamp_=timestampGenerator_->getCurrentFrameCount();

    // 重置累积缓冲区，防止上次播放残留
    pendingInterleavedPcm_.clear();
    pendingSamplesPerChannel_ = 0;
    lastChannels_ = 0;

    // 获取原始音频参数
    int originalSampleRate = codecContext->sample_rate;
    int originalChannels = codecContext->ch_layout.nb_channels;

    // 检查是否需要重采样
    bool needsResampling = (originalSampleRate != SAMPLE_RATE) || (codecContext->sample_fmt != AV_SAMPLE_FMT_FLT);

    // 预缓冲控制变量
    int frameCount = 0;
    double lastEmitTime = -1.0;

    bool endedByEof = false;

    // 循环播放的主循环
    do {
        // 重置文件指针到开始位置（用于循环播放）
        if (frameCount > 0 && isLooping) {
            QThread::msleep(LOOP_INTERVAL);
            {
                QMutexLocker locker(&mutex);
                seekFileToSec(0.0);
                m_startPositionSec = 0.0;
            }
        }

        while (isPlaying) {
            // 处理拖拽 seek（播放中）
            double seekProgressSec = -1.0;
            double seekProgressTotal = 0.0;
            {
                QMutexLocker locker(&mutex);
                if (m_pendingSeekSec >= 0.0) {
                    const double seekSec = m_pendingSeekSec;
                    m_pendingSeekSec = -1.0;
                    seekFileToSec(seekSec);
                    lastEmitTime = -1.0;
                    clearPcmAndChannelBuffers();
                    seekProgressSec = seekSec;
                    seekProgressTotal = m_durationSec > 0.0 ? m_durationSec
                        : (formatContext && formatContext->duration != AV_NOPTS_VALUE
                               ? formatContext->duration / (double)AV_TIME_BASE : 0.0);
                }
            }
            if (seekProgressSec >= 0.0) {
                emit playbackProgress(seekProgressSec, seekProgressTotal);
            }

            if (av_read_frame(formatContext, &packet) < 0) {
                break; // EOF 或读失败
            }

            // 发送播放进度信号
            if (packet.stream_index == audioStreamIndex) {
                 double currentSec = 0.0;
                 if (packet.pts != AV_NOPTS_VALUE) {
                     currentSec = packet.pts * av_q2d(formatContext->streams[packet.stream_index]->time_base);
                 }

                 double totalSec = m_durationSec;
                 if (totalSec <= 0.0 && formatContext->duration != AV_NOPTS_VALUE) {
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

        if (isPlaying && !isLooping) {
            endedByEof = true;
        }
    } while (isPlaying && isLooping); // 循环播放条件

    if (endedByEof) {
        const double totalSec = m_durationSec > 0.0 ? m_durationSec
            : (formatContext && formatContext->duration != AV_NOPTS_VALUE
                   ? formatContext->duration / (double)AV_TIME_BASE : 0.0);
        // 先把进度推到末尾，再通知结束，便于 UI 一致复位
        emit playbackProgress(totalSec, totalSec);
        {
            QMutexLocker locker(&mutex);
            isPlaying = false;
            m_startPositionSec = 0.0;
            m_pendingSeekSec = -1.0;
        }
        emit playbackFinished();
    }

    // 清理资源
    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
    if (outputBuffer) {
        av_freep(&outputBuffer);
    }
}

void AudioDecoder::cleanupFFmpeg(){
    if (swrContext) {
        swr_free(&swrContext);
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
    }
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    if (audioFrame) {
        av_frame_free(&audioFrame);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (resampledBuffer) {
        av_free(resampledBuffer);
        resampledBuffer = nullptr;
    }
    audioStreamIndex = -1;
}

/**
 * @brief 应用音量调整到音频数据（volume为分贝值）
 * @param data 音频数据指针
 * @param sampleCount 采样点数量
 * @param channels 声道数
 */
void AudioDecoder::applyVolume(uint8_t* data, int sampleCount, int channels) {

    if (volume == 0.0f) return;
    // 将分贝值转换为线性增益：gain = 10^(dB/20)
    float linearGain;
    if (volume <= -100.0f) {
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



// 将解码得到的PCM累积并按固定1920采样/声道切片发送
int AudioDecoder::processPcmAndEmitFixedFrames(const uint8_t* interleavedPcm,
                                               int samplesPerChannel,
                                               int channels,
                                               int sampleRate)
{
    const int bytesPerSample = 4; // Float
    const int targetSamplesPerChannel = samplesPerChannelForTimestampFrame();
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



