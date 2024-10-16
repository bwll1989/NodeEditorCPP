//
// Created by 吴斌 on 2024/4/19.
//
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <portaudio.h>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

#define OUTPUT_CHANNELS 2
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 1024
class AudioOutput : public QObject {
    Q_OBJECT

public:
    explicit AudioOutput(QObject *parent = nullptr) {}
    bool device_init()
    {
        avformat_network_init();
        PaError err;
        err = Pa_Initialize();
        if (err != paNoError) {
            qWarning() << "PortAudio initialization failed";
            return false;
        }
        PaSampleFormat sample_fmt;
        PaStreamParameters outputParameters;
        outputParameters.device = Pa_GetDefaultOutputDevice();
        outputParameters.channelCount = 2;
        outputParameters.sampleFormat = paInt16;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;
        err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLE_RATE, paFramesPerBufferUnspecified, paClipOff, nullptr, nullptr);
        if (err != paNoError) {
            qWarning() << "PortAudio stream opening failed";
            return false;
        }
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            qWarning() << "PortAudio stream starting failed";
            return false;
        }
        return true;
    }

    void get_condition(QWaitCondition *cv){
        condition=cv;
    }
    void Buffer_init(QQueue<uint8_t *> &Buffer)
    {fifoBuffer=&Buffer;}
public slots:
    void syncData(QQueue<uint8_t *> &Buffer,QWaitCondition *cv)
    {
        Buffer_init(Buffer);
        get_condition(cv);
    };
    void processAudio()
    {
        while (true) {
            uint8_t *buffer;
            QMutexLocker locker(&fifoMutex);
            while (fifoBuffer->isEmpty()) {
                condition->wait(&fifoMutex);
            }
            buffer = fifoBuffer->dequeue();
            locker.unlock();

            // Write audio data to PortAudio stream
            Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);

            // Free buffer memory
            delete buffer;
        }
    }
private:
    PaStream *stream = nullptr;
    QQueue<uint8_t *> *fifoBuffer;
    QMutex fifoMutex;
    QWaitCondition *condition;
};