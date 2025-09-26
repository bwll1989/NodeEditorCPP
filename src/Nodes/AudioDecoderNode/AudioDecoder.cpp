#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
#include "DataTypes/AudioData.h"  // 确保包含此头文件
#include "AudioDecoder.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}
#include <iostream>
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
static const int SAMPLES_PER_CHANNEL = SAMPLE_RATE/TimestampGenerator::getInstance()->getFrameRate();;
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
    formatContext= nullptr;
    codecContext= nullptr;
    codec= nullptr;
    swrContext= nullptr;
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
                          (codecContext->sample_fmt != AV_SAMPLE_FMT_S16);

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
                                AV_SAMPLE_FMT_S16,
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
    return res;
}


void AudioDecoder::startPlay(){
    QMutexLocker locker(&mutex);

    // 重置文件指针到开始位置
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
            int flushSize = SAMPLES_PER_CHANNEL * 2 * 2;  // samples × channels × bytes_per_sample
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

    isPlaying = true;
    start();  // 启动线程开始播放
}

/**
 * @brief 停止音频播放
 * 确保线程安全地停止播放并清理所有缓冲区
 */
void AudioDecoder::stopPlay() {
    {
        QMutexLocker locker(&mutex);
        if (!isPlaying) {
            return; // 已经停止，直接返回
        }
        isPlaying = false;
        condition.wakeAll();
    }

    // 等待线程结束（在锁外进行）
    if (isRunning()) {
        wait(3000); // 等待最多3秒
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
            int flushSize = 2048 * 2 * 2;
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

        // 16位音频的类似优化
        const int16_t* inputData = reinterpret_cast<const int16_t*>(frame.data.constData());
        int16_t* outputData = reinterpret_cast<int16_t*>(channelData.data());

        const int16_t* src = inputData + channel;
        int16_t* dst = outputData;

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
    // 获取原始音频参数
    int originalSampleRate = codecContext->sample_rate;
    int originalChannels = codecContext->ch_layout.nb_channels;

    // 检查是否需要重采样
    bool needsResampling = (originalSampleRate != SAMPLE_RATE) || (codecContext->sample_fmt != AV_SAMPLE_FMT_S16);

    // 预缓冲控制变量
    int frameCount = 0;

    // 循环播放的主循环
    do {
        // 重置文件指针到开始位置（用于循环播放）
        if (frameCount > 0 && isLooping) {
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

        while (isPlaying && av_read_frame(formatContext, &packet) >= 0) {
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
                        int newSize = outputSamples * outputChannels * 2;

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
                        // 不需要重采样的情况（48000Hz且已经是S16格式）
                        if (codecContext->sample_fmt != AV_SAMPLE_FMT_S16) {
                            qDebug() << "Warning: Expected S16 format but got" << codecContext->sample_fmt;
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
    } while (isPlaying && isLooping); // 循环播放条件

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
    }

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
    if (volume <= -40.0f) {
        linearGain = 0.0f; // 静音处理
    } else {
        linearGain = std::pow(10.0f, volume / 20.0f);
    }
    int16_t* samples = reinterpret_cast<int16_t*>(data);
    int totalSamples = sampleCount * channels;

    for (int i = 0; i < totalSamples; i++) {
        // 应用线性增益并进行削波保护
        float scaledSample = samples[i] * linearGain;
        
        // 削波保护，防止溢出
        if (scaledSample > 32767.0f) {
            samples[i] = 32767;
        } else if (scaledSample < -32768.0f) {
            samples[i] = -32768;
        } else {
            samples[i] = static_cast<int16_t>(scaledSample);
        }
    }
}



// 将解码得到的PCM累积并按固定1920采样/声道切片发送
int AudioDecoder::processPcmAndEmitFixedFrames(const uint8_t* interleavedPcmS16,
                                               int samplesPerChannel,
                                               int channels,
                                               int sampleRate)
{
    const int bytesPerSample = 2; // S16
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
    pendingInterleavedPcm_.append(reinterpret_cast<const char*>(interleavedPcmS16), inputBytes);
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
        frame.bitsPerSample = 16;
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




