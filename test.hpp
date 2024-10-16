#ifndef TEST_H
#define TEST_H

#include <vector>
#include <mutex>
#include <cstring>
#include<QDebug>
#include <QObject>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}
#include <QObject>
#include <portaudio.h>

class AudioBuffer {
public:
    void write(const uint8_t* data, size_t size) {
        std::lock_guard<std::mutex> lock(mutex);
        qDebug()<<"2 write buffer";
        buffer.insert(buffer.end(), data, data + size);

    }

    size_t read(uint8_t* outBuffer, size_t size) {
        std::lock_guard<std::mutex> lock(mutex);
        qDebug()<<"3 read buffer";
        size_t bytesToRead = std::min(size, buffer.size());
        std::memcpy(outBuffer, buffer.data(), bytesToRead);
        buffer.erase(buffer.begin(), buffer.begin() + bytesToRead);
        return bytesToRead;
    }

    size_t available() const {
        std::lock_guard<std::mutex> lock(mutex);
        return buffer.size();
    }

private:
    std::vector<uint8_t> buffer;
    mutable std::mutex mutex;
};


class FFmpegDecoder : public QObject {
Q_OBJECT

public:
    FFmpegDecoder(const QString& filename, QObject* parent = nullptr) : QObject(parent) {

        avformat_network_init();
        if (avformat_open_input(&formatContext, filename.toStdString().c_str(), nullptr, nullptr) != 0) {
            qDebug()<<"打开文件失败"<<filename.toStdString().c_str();
            return ;
        }
        //打开文件，并读取格式信息到格式上下文formatContext

        if (avformat_find_stream_info(formatContext, nullptr) != 0) {
            qDebug()<<"找不到流信息";
            return;
        }
        //查询流信息

        audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamIndex==-1) {
            return;
        }
        //查找音频流索引

        codec = avcodec_find_decoder(formatContext->streams[audioStreamIndex]->codecpar->codec_id);//获取codec

        if (!codec) {
            // 找不到解码器
            return ;
        }
        //查找解码器

        codecContext= avcodec_alloc_context3(codec);
        int ret=avcodec_parameters_to_context(codecContext, formatContext->streams[audioStreamIndex]->codecpar);
        if (ret< 0) {
            qDebug()<<"解码器参数设置失败";
            // 解码器参数设置失败
            return;
        }

        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            qDebug()<<"打开解码器失败";
            return;
        }




        // auto *res= new QJsonObject();
        // res->insert("path",filePath);
        // res->insert("bit_rate",QString::number(codecContext->bit_rate));
        // res->insert("sample_fmt",codecContext->sample_fmt);
        // res->insert("channels",QString::number(codecContext->ch_layout.nb_channels));
        // res->insert("sample_rate",QString::number(codecContext->sample_rate));
        // res->insert("codec",codec->name);
        qDebug()<<filename;

    }


    void startDecoding(){

        AVFrame* frame = av_frame_alloc();
        while (decodeAudioFrame(frame)) {
            qDebug()<<"1 decoding";
            int bufferSize = av_samples_get_buffer_size(nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
            uint8_t* outputBuffer = (uint8_t*)av_malloc(bufferSize);
            emit audioFrameDecoded(outputBuffer, bufferSize);
            av_free(outputBuffer);
        }
        av_frame_free(&frame);

    }
signals:
\

    void audioFrameDecoded(uint8_t* data, int dataSize);  // 解码完成后发送解码后的帧

private:
    bool decodeAudioFrame(AVFrame* frame){
        AVPacket packet;
        if (av_read_frame(formatContext, &packet) >= 0) {
            if (packet.stream_index == audioStreamIndex) {
                avcodec_send_packet(codecContext, &packet);
                if (avcodec_receive_frame(codecContext, frame) == 0) {
                    av_packet_unref(&packet);
                    return true;
                }
            }
            av_packet_unref(&packet);
        }
        return false;
    }

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    const AVCodec* codec = nullptr;
    int audioStreamIndex = -1;
};

class PortAudioPlayer : public QObject {
    Q_OBJECT

public:
    PortAudioPlayer(QObject* parent = nullptr) : QObject(parent){
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
            return;
        }
        PaStreamParameters outputParameters;
        outputParameters.device = Pa_GetDefaultOutputDevice();
        outputParameters.channelCount = 2;
        outputParameters.sampleFormat = paInt16; // 假设输出是16位整数
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;
        qDebug()<< Pa_GetDeviceInfo(outputParameters.device)->defaultSampleRate;
        err = Pa_OpenStream(&stream, nullptr, &outputParameters, 44100, paFramesPerBufferUnspecified, paNoFlag, &PortAudioPlayer::paCallback, this);

        if (err != paNoError) {
            qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
            return;
        }
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
            return;
        }
        qDebug()<<"portaudio init success";
    };

    ~PortAudioPlayer(){
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    }

public slots:
    void playAudioFrame(uint8_t* data, int dataSize){
        // 将解码后的 PCM 数据写入缓冲区
        // qDebug()<<"channels:"<<frame->nb_samples;
        // int bytesPerSample = 2;  // 16-bit 音频
        // int channels = 2;  // 立体声
        // size_t dataSize = frame->nb_samples * bytesPerSample * channels;
        // size_t dataSize =av_samples_get_buffer_size(NULL, frame->ch_layout.nb_channels, frame->nb_samples,
        //                            static_cast<AVSampleFormat>(frame->format), 0);
        // qDebug()<<dataSize;
        qDebug() << "Received audio data of size:" << dataSize;
        audioBuffer.write(data, dataSize);
    }

private:
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData) {
        qDebug()<<"1111111111 callback";
        auto* player = static_cast<PortAudioPlayer*>(userData);
        return player->fillOutputBuffer(outputBuffer, framesPerBuffer);
        // return paContinue;
    }
    int fillOutputBuffer(void* outputBuffer, unsigned long framesPerBuffer){
        size_t bytesPerSample = 2; // 16-bit 音频，每样本占2字节
        size_t bytesToFill = framesPerBuffer * bytesPerSample * 2; // 2个声道

        size_t availableBytes = audioBuffer.available();
        if (availableBytes < bytesToFill) {
            // 如果数据不足，用静音填充
            qDebug()<<"buffer zero";
            std::memset(outputBuffer, 0, bytesToFill);
            if (availableBytes > 0) {
                audioBuffer.read(reinterpret_cast<uint8_t*>(outputBuffer), availableBytes);
            }
        } else {
            qDebug()<<"read buffer";
            audioBuffer.read(reinterpret_cast<uint8_t*>(outputBuffer), bytesToFill);
        }

        return paContinue;
    }

    PaStream* stream;
    AudioBuffer audioBuffer;
};
#endif

