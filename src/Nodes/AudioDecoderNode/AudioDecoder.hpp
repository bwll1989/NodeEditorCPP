#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
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
#include <Common/Devices/AudioPipe/AudioPipe.h>
class AudioDecoder : public QThread {
Q_OBJECT

public:

    explicit AudioDecoder(AudioPipe *audio,QObject *parent = nullptr): QThread(parent), packet(nullptr),
                                                     resampledBuffer(nullptr),
                                                        audioProcessor(audio),
    // paStream(nullptr),
                                                     isPlaying(false) {}
    ~AudioDecoder(){
        stopPlay();
        // cleanupPortAudio();
        cleanupFFmpeg();
    }

    // bool loadFile(const QString &filePath){
    //     if(!initializeFFmpeg(filePath))
    //     {
    //         return false;
    //     }
    //     qDebug()<<"initializeFFmpeg OK";
    //     // if(!initializePortAudio())
    //     // {
    //     //     return false;
    //     // }
    //     // qDebug()<<"initializePortAudio OK";
    //
    //     return true;
    // }
    // bool initializePortAudio() {
    //     PaError err = Pa_Initialize();
    //     if (err != paNoError) {
    //         qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
    //         return false;
    //     }
    //     PaStreamParameters outputParameters;
    //     outputParameters.device = Pa_GetDefaultOutputDevice();
    //     outputParameters.channelCount = codecContext->ch_layout.nb_channels;
    //     outputParameters.sampleFormat = paInt16; // 假设输出是16位整数
    //     outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    //     outputParameters.hostApiSpecificStreamInfo = nullptr;
    //     err = Pa_OpenStream(&paStream, nullptr, &outputParameters, 48000, paFramesPerBufferUnspecified, paNoFlag, nullptr, nullptr);
    //     if (err != paNoError) {
    //         qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
    //         return false;
    //     }
    //     err = Pa_StartStream(paStream);
    //     if (err != paNoError) {
    //         qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
    //         return false;
    //     }
    //     return true;
    // }
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

    void startPlay(){
        QMutexLocker locker(&mutex);
        isPlaying = true;
        start();
    }
    void stopPlay() {
        QMutexLocker locker(&mutex);
        isPlaying = false;
        condition.wakeAll();
        wait(); // 等待线程结束
    }
    bool isPlaying;
protected:
    void run() override{
        playAudio();
    }
    void playAudio(){
        AVPacket packet;
        audioFrame = av_frame_alloc();
        uint8_t* outputBuffer = nullptr;
        int outputBufferSize = 0;

        while ( isPlaying && av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == audioStreamIndex) {
                if (avcodec_send_packet(codecContext, &packet) < 0) {
                    qWarning() << "Error sending a packet for decoding.";
                    continue;
                }

                while (avcodec_receive_frame(codecContext, audioFrame) >= 0) {
                    // 计算重采样后输出的缓冲区大小
                    int outputBufferSamples = av_rescale_rnd(swr_get_delay(swrContext, codecContext->sample_rate) + audioFrame->nb_samples, 48000, codecContext->sample_rate, AV_ROUND_UP);

                    // 为输出缓冲区分配空间
                    if (outputBufferSize < outputBufferSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2)
                    {
                        av_freep(&outputBuffer);
                        outputBufferSize = outputBufferSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2;
                        outputBuffer = (uint8_t*)av_malloc(outputBufferSize);
                    }

                    // 重采样
                    int samplesResampled = swr_convert(swrContext,
                                                       &outputBuffer,
                                                       outputBufferSamples,
                                                       (const uint8_t**)audioFrame->data,
                                                       audioFrame->nb_samples);

                    if (samplesResampled < 0) {
                        qWarning() << "Error during resampling.";
                        delete[] outputBuffer; // 确保释放内存
                        continue;
                    }

                    while (audioProcessor->cacheSize() >= audioProcessor->maxQueueSize) {
                        QThread::msleep(10); // 稍微休眠以避免占用 CPU
                    }

                    // 现在可以安全地推送数据

                    audioProcessor->pushAudioData(outputBuffer,samplesResampled * sizeof(int16_t));

                    // try {
                    //     audioProcessor->pushAudioData(outputBuffer,samplesResampled * sizeof(int16_t));
                    // } catch (const std::exception& e) {
                    //     qWarning() << "Exception caught:" << e.what();
                    // }
                    // qDebug()<< " samplesResampled:"<<samplesResampled;
                    // 将重采样的数据传递给 PortAudio
                    // Pa_WriteStream(paStream, outputBuffer, samplesResampled);
                }
            }
            av_packet_unref(&packet);
        }

        // 清理资源
        av_frame_free(&audioFrame);
        swr_free(&swrContext);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        if (outputBuffer) {
            av_freep(&outputBuffer);
        }

        // Pa_StopStream(paStream);
        // Pa_CloseStream(paStream);

    }

private:
    //
    // void cleanupPortAudio(){
    //     if (paStream) {
    //         Pa_StopStream(paStream);
    //         Pa_CloseStream(paStream);
    //         Pa_Terminate();
    //         paStream = nullptr;
    //     }
    // }

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
    // 创建一个音频缓存（假设我们需要缓存所有重采样数据）
    AudioPipe* audioProcessor;
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

};

