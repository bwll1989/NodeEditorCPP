#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "RtAudio.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}
#include <atomic>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>


class AudioDecoder : public QThread {
Q_OBJECT

public:

    explicit AudioDecoder(QObject *parent = nullptr): QThread(parent), packet(nullptr),m_audio(RtAudio::WINDOWS_WASAPI){}
    ~AudioDecoder(){
       stop();
    }

    bool loadFile(const QString &filePath){
        m_audioFilePath=filePath;
        return true;
    }

    void stop() {
        m_stop = true;
        m_condVar.notify_all(); // ���ѵȴ����߳�
        wait();  // �ȴ��߳̽���

        if (swrContext) {
            swr_free(&swrContext);
        }

        if (codecContext) {
            avcodec_free_context(&codecContext);
        }

        if (formatContext) {
            avformat_close_input(&formatContext);
        }

        if (m_audio.isStreamOpen()) {
            m_audio.stopStream();
            m_audio.closeStream();
        }
    }

protected:
    void run() override{

        if (!initializeFFmpeg(m_audioFilePath) || !initializePortAudio()) {
            return;
        }
        AVPacket packet;
        frame = av_frame_alloc();

        uint8_t* outputBuffer = nullptr;
        int outputBufferSize = 0;
        while (av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == audioStreamIndex) {
                if (avcodec_send_packet(codecContext, &packet) < 0) {
                    qWarning() << "Error sending a packet for decoding.";
                    continue;
                }

                AVFrame* frame = av_frame_alloc();

                while (avcodec_receive_frame(codecContext, frame) >= 0) {
                    // �����ز���������Ļ�������С
                    int outputBufferSamples = av_rescale_rnd(swr_get_delay(swrContext, codecContext->sample_rate) + frame->nb_samples, 44100, codecContext->sample_rate, AV_ROUND_UP);

                    // Ϊ�������������ռ�
                    if (outputBufferSize < outputBufferSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2)
                    {
                        av_freep(&outputBuffer);
                        outputBufferSize = outputBufferSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 2;
                        outputBuffer = (uint8_t*)av_malloc(outputBufferSize);
                    }

                    int samplesResampled = swr_convert(swrContext,
                                                       &outputBuffer,
                                                       outputBufferSamples,
                                                       (const uint8_t**)frame->data,
                                                       frame->nb_samples);

                    if (samplesResampled < 0) {
                        qWarning() << "Error during resampling.";
                        continue;
                    }

                    std::vector<uint8_t> audioData(outputBuffer, outputBuffer + outputBufferSize);
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_audioBufferQueue.push(audioData);
                        qDebug() << "Pushed audio data to buffer queue. Current queue size:" << m_audioBufferQueue.size();
                    }
                    m_condVar.notify_one();

                }
            }
            av_packet_unref(&packet);
        }
        av_frame_free(&frame);
        swr_free(&swrContext);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);

        if (outputBuffer) {
            av_freep(&outputBuffer);
        }
        
        stop();
    }

private:
    bool initializePortAudio() {
        if (m_audio.getDeviceCount() < 1) {
            qDebug() << "No audio devices found!";
            return false;
        }

        RtAudio::StreamParameters parameters;
        parameters.deviceId = m_audio.getDefaultOutputDevice();
        parameters.nChannels = codecContext->ch_layout.nb_channels;
        parameters.firstChannel = 0;

        unsigned int sampleRate = codecContext->sample_rate;
        unsigned int bufferFrames = 512;

        try {
            m_audio.openStream(&parameters, nullptr, RTAUDIO_SINT16, sampleRate, &bufferFrames, &audioCallback, this);
            m_audio.startStream();
            qDebug() << "Audio stream started successfully.";
        } catch (RtAudioErrorType& e) {
            qDebug() << "RtAudio error: " << e;
            return false;
        }
        RtAudio::DeviceInfo info =m_audio.getDeviceInfo( parameters.deviceId);
        qDebug() << "Default Output ID: " << info.ID;
        qDebug() << "Default Output Device: " << info.name.c_str();
        qDebug() << "Sample Rate: " << sampleRate;
        qDebug() << "Channels: " << info.outputChannels;
        qDebug()<<m_audio.isStreamOpen();
        return true;
    }

    void cleanupPortAudio(){

    }

    bool initializeFFmpeg(const QString &filePath){
        formatContext= nullptr;
        codecContext= nullptr;
        codec= nullptr;
        swrContext= nullptr;
        avformat_network_init();
        if (avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr) != 0) {
            qDebug()<<"���ļ�ʧ��"<<filePath.toStdString().c_str();
            return false;
        }
        //���ļ�������ȡ��ʽ��Ϣ����ʽ������formatContext

        if (avformat_find_stream_info(formatContext, nullptr) != 0) {
            qDebug()<<"�Ҳ�������Ϣ";
            return false;
        }
        //��ѯ����Ϣ
        audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamIndex==-1) {
            return false;
        }
        //������Ƶ������
        codec = avcodec_find_decoder(formatContext->streams[audioStreamIndex]->codecpar->codec_id);//获取codec

        if (!codec) {
            // 找不到解码器
            return false;
        }
        //���ҽ�����
        codecContext= avcodec_alloc_context3(codec);
        int ret=avcodec_parameters_to_context(codecContext, formatContext->streams[audioStreamIndex]->codecpar);
        if (ret< 0) {
            qDebug()<<"��������������ʧ��";
            // ��������������ʧ��
            return false;
        }

        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            qDebug()<<"�򿪽�����ʧ��";
            return false;
        }


        AVChannelLayout outChannelLayout=AV_CHANNEL_LAYOUT_STEREO;
        AVChannelLayout inChannelLayout;
//        outChannelLayout.nb_channels = codecContext->ch_layout.nb_channels;
        inChannelLayout= codecContext->ch_layout;
        if (swr_alloc_set_opts2(&swrContext,
                                &outChannelLayout,
                                AV_SAMPLE_FMT_S16,
                                44100,
                                &inChannelLayout,
                                codecContext->sample_fmt,
                                codecContext->sample_rate,
                                0,
                                nullptr)!= 0)
        {
            qDebug()<<"swr_alloc_set_opts2 fail";
            return false;
        }
        ret=swr_init(swrContext);
        if(ret<0)
        {
//            swrContext
            qDebug()<<"swrContext fail"<<ret;
            return false;
        };
        qDebug()<<"����:"<<codecContext->bit_rate;
        qDebug()<<"��ʽ:"<<codecContext->sample_fmt;
        qDebug()<<"ͨ��:"<<codecContext->ch_layout.nb_channels;
        qDebug()<<"������:"<<codecContext->sample_rate;
        qDebug()<<"������:"<<codec->name;
        return true;
    }

    static int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                             double streamTime, RtAudioStreamStatus status, void* userData){
        qDebug() << "Audio callback called with" << nBufferFrames << "frames.";
        auto* player = static_cast<AudioDecoder*>(userData);

        if (status) {
            qDebug() << "Stream underflow detected!";
        }

        std::unique_lock<std::mutex> lock(player->m_mutex);
        if (player->m_audioBufferQueue.empty()) {
            qDebug() << "Buffer is empty. Filling with zeros.";
            memset(outputBuffer, 0, nBufferFrames * 4); // ���������Ϊ�գ������0
        } else {
            auto& audioData = player->m_audioBufferQueue.front();

            // ʹ�� size_t ��������ת��
            size_t dataSize = std::min(audioData.size(), static_cast<size_t>(nBufferFrames * 4));

            memcpy(outputBuffer, audioData.data(), dataSize);

            // �����������ÿ����ݣ���Ӷ������Ƴ�
            player->m_audioBufferQueue.pop();
        }
        lock.unlock();

        return 0;
}
    QString m_audioFilePath="";
    std::atomic<bool> m_stop= false;
    AVFrame *frame;
    AVFormatContext* formatContext= nullptr;
    AVCodecContext* codecContext= nullptr;
    SwrContext* swrContext= nullptr;
    RtAudio m_audio;
    const AVCodec *codec;
    int audioStreamIndex=-1;
    AVPacket *packet= nullptr;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
    std::queue<std::vector<uint8_t>> m_audioBufferQueue;


};

