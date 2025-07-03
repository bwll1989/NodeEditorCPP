#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
#include "DataTypes/AudioData.h"  // 确保包含此头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}
#include <iostream>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <portaudio.h>
#include <QDateTime>
// #include <Common/Devices/AudioPipe/AudioPipe.h>

class AudioDecoder : public QThread {
Q_OBJECT

public:
    explicit AudioDecoder(QObject *parent = nullptr)
        : QThread(parent)
        , formatContext(nullptr)
        , codecContext(nullptr)
        , codec(nullptr)
        , audioFrame(nullptr)
        , packet(nullptr)
        , swrContext(nullptr)
        , resampledBuffer(nullptr)
        , audioStreamIndex(-1)
        , isPlaying(false) {
    }

    ~AudioDecoder() {
        stopPlay();
        cleanupFFmpeg();
    }
    /**
     * @brief 初始化ffmpeg
     * @param filePath
     * @return 返回音频参数
     */
    QJsonObject* initializeFFmpeg(const QString &filePath){
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
        //查询流信息

        audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamIndex==-1) {
            return nullptr;
        }
        //查找音频流索引

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


        AVChannelLayout outChannelLayout=AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout inChannelLayout;
//        outChannelLayout.nb_channels = codecContext->ch_layout.nb_channels;
        inChannelLayout= codecContext->ch_layout;
        if (swr_alloc_set_opts2(&swrContext,
                                &outChannelLayout,
                                AV_SAMPLE_FMT_S16,
                                48000,
                                &inChannelLayout,
                                codecContext->sample_fmt,
                                codecContext->sample_rate,
                                0,
                                nullptr)!= 0)
        {
            qDebug()<<"swr_alloc_set_opts2 fail";
            return nullptr;
        }
        ret=swr_init(swrContext);
        if(ret<0)
        {
//            swrContext
            qDebug()<<"swrContext fail"<<ret;
            return nullptr;
        };
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

    /**
     * @brief 开始解码
     */
    void startPlay(){
        QMutexLocker locker(&mutex);
        isPlaying = true;
        start();
    }
    /**
     * @brief 停止解码
     */
    void stopPlay() {
        QMutexLocker locker(&mutex);
        isPlaying = false;
        condition.wakeAll();
        wait(); // 等待线程结束


        // 清理解码资源
        // cleanupFFmpeg();

        // // 重置解码器状态
        // formatContext = nullptr;
        // codecContext = nullptr;
        // codec = nullptr;
        // audioFrame = nullptr;
        // packet = nullptr;
        // swrContext = nullptr;
        // audioStreamIndex = -1;
    }
    bool isPlaying;

signals:
    // 发送解码后的音频数据
    void audioFrameReady(AudioFrame frame);

protected:
    void run() override {
        playAudio();
    }

private:
    /**
     * @brief 播放音频
     */
    void playAudio() {
        AVPacket packet;
        audioFrame = av_frame_alloc();
        uint8_t* outputBuffer = nullptr;
        int outputBufferSize = 0;

        // 获取原始音频参数
        int originalSampleRate = codecContext->sample_rate;
        int originalChannels = codecContext->ch_layout.nb_channels;

        // 计算每帧的理论持续时间（基于原始采样率）
        const double frameTime = 1000.0 * BUFFER_SIZE / originalSampleRate; // ms
        qint64 lastFrameTime = QDateTime::currentMSecsSinceEpoch();

        while (isPlaying && av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == audioStreamIndex) {
                if (avcodec_send_packet(codecContext, &packet) < 0) {
                    continue;
                }

                while (avcodec_receive_frame(codecContext, audioFrame) >= 0) {
                    // 计算重采样后的大小
                    int outputBufferSamples = av_rescale_rnd(
                        swr_get_delay(swrContext, originalSampleRate) + audioFrame->nb_samples,
                        48000,  // 输出采样率固定为48000
                        originalSampleRate,
                        AV_ROUND_UP
                    );

                    // 确保缓冲区大小合适（考虑实际声道数）
                    int newSize = outputBufferSamples * originalChannels * 2; // channels * 2 bytes per sample
                    if (outputBufferSize < newSize) {
                        av_freep(&outputBuffer);
                        outputBufferSize = newSize;
                        outputBuffer = (uint8_t*)av_malloc(outputBufferSize);
                    }

                    // 重采样
                    int samplesResampled = swr_convert(
                        swrContext,
                        &outputBuffer,
                        outputBufferSamples,
                        (const uint8_t**)audioFrame->data,
                        audioFrame->nb_samples
                    );

                    if (samplesResampled > 0) {
                        int totalBytes = samplesResampled * originalChannels * 2; // channels * 2 bytes per sample

                        AudioFrame frame;
                        frame.data = QByteArray(reinterpret_cast<const char*>(outputBuffer), totalBytes);
                        frame.sampleRate = 48000;  // 输出采样率固定为48000
                        frame.channels = originalChannels;  // 使用实际声道数
                        frame.bitsPerSample = 16;

                        frame.timestamp = QDateTime::currentMSecsSinceEpoch();

                        // 使用 DirectConnection 而不是 QueuedConnection
                        emit audioFrameReady(frame);

                        // 控制解码速度（基于原始采样率）
                        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
                        qint64 elapsedTime = currentTime - lastFrameTime;
                        qint64 sleepTime = frameTime - elapsedTime;

                        if (sleepTime > 0) {
                            QThread::msleep(sleepTime);
                        }

                        lastFrameTime = QDateTime::currentMSecsSinceEpoch();
                    }
                }
            }
            av_packet_unref(&packet);
        }

        // 清理资源
        if (audioFrame) {
            av_frame_free(&audioFrame);
        }
        if (outputBuffer) {
            av_freep(&outputBuffer);
        }
    }

    void cleanupFFmpeg(){
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

    AVFormatContext *formatContext= nullptr;
    AVCodecContext *codecContext= nullptr;
    const AVCodec *codec;
    AVFrame *audioFrame;
    AVPacket *packet;
    SwrContext *swrContext;

    int audioStreamIndex=-1;
    int64_t decodedFrames;
    uint8_t *resampledBuffer;
    // PaStream *paStream;
    QMutex mutex;
    QWaitCondition condition;

    static const int BUFFER_SIZE = 4096;  // 与 AudioDeviceOut 保持一致
};

