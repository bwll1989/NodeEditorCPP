#pragma once

#include "QtNodes/NodeDelegateModel"

#include <QtCore/QObject>

#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "DataTypes/NodeDataList.hpp"
#include "AudioDeviceOutInterface.hpp"
#include <portaudio.h>
#include "QThread"
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include "PluginDefinition.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class AudioDeviceOutDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        AudioDeviceOutDataModel() {
            InPortCount = 1;
            OutPortCount = 0;
            CaptionVisible = true;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = true;
            Resizable = false;

            // 初始化 PortAudio
            initPortAudio();

            // 连接界面信号
            connect(widget->deviceSelector, &QComboBox::currentTextChanged,
                    this, &AudioDeviceOutDataModel::onDeviceChanged);
        }

        ~AudioDeviceOutDataModel() {
            stopAudio();
            if (paStream) {
                Pa_CloseStream(paStream);
                paStream = nullptr;
            }
            Pa_Terminate();
        }

    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                return AudioData().type();
            }
            return NodeDataType{"", ""};
        }

        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            Q_UNUSED(port)
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (portIndex == 0) {
                auto audioData = std::dynamic_pointer_cast<AudioData>(data);
                if (audioData) {
                    handleNewAudioData(audioData->getAudioFrame());
                }
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;

            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;

            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {

            }
        }

    public slots:

        void initPortAudio() {
            PaError err = Pa_Initialize();
            if (err != paNoError) {
                qWarning() << "PortAudio initialization failed:" << Pa_GetErrorText(err);
            }

            // 获取设备列表
            updateDeviceList();
        }

        void updateDeviceList() {
            widget->deviceSelector->clear();

            int numDevices = Pa_GetDeviceCount();
            for (int i = 0; i < numDevices; i++) {
                const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
                if (deviceInfo && deviceInfo->maxOutputChannels > 0) {
                    QString deviceName = QString("%1: %2").arg(i).arg(deviceInfo->name);
                    widget->deviceSelector->addItem(deviceName, i);
                }
            }
        }

        bool startAudio() {
            if (paStream) {
                return true;
            }

            int deviceId = widget->deviceSelector->currentData().toInt();
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceId);
            if (!deviceInfo) {
                return false;
            }

            PaStreamParameters outputParams;
            outputParams.device = deviceId;
            outputParams.channelCount = CHANNELS;
            outputParams.sampleFormat = paInt16;
            outputParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
            outputParams.hostApiSpecificStreamInfo = nullptr;

            PaError err = Pa_OpenStream(
                &paStream,
                nullptr,
                &outputParams,
                SAMPLE_RATE,
                BUFFER_SIZE,  // 使用固定的缓冲区大小
                paClipOff,
                &AudioDeviceOutDataModel::paCallback,
                this
            );

            if (err != paNoError) {
                qWarning() << "Failed to open stream:" << Pa_GetErrorText(err);
                return false;
            }

            err = Pa_StartStream(paStream);
            if (err != paNoError) {
                qWarning() << "Failed to start stream:" << Pa_GetErrorText(err);
                Pa_CloseStream(paStream);
                paStream = nullptr;
                return false;
            }

            isPlaying = true;
            return true;
        }

        void stopAudio() {
            if (paStream) {
                Pa_StopStream(paStream);
                Pa_CloseStream(paStream);
                paStream = nullptr;
            }
        }

        void handleNewAudioData(const AudioFrame& frame) {
            QMutexLocker locker(&mutex);

            // 检查采样率和格式是否匹配
            if (frame.sampleRate != SAMPLE_RATE || frame.channels != CHANNELS) {
                qWarning() << "Audio format mismatch!";
                return;
            }

            // 如果队列已满，丢弃最旧的数据
            if (audioQueue.size() >= MAX_QUEUE_SIZE) {
                audioQueue.dequeue();
            }

            audioQueue.enqueue(frame);

            // 更新缓存数量显示
            updateBufferCount();

            // 当队列中有足够的数据时，开始播放
            if (!isPlaying && audioQueue.size() >= MIN_BUFFERED_FRAMES) {
                isPlaying = true;
                startAudio();
            }
        }

        static int paCallback(const void* inputBuffer,
                             void* outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void* userData) {
            AudioDeviceOutDataModel* self = static_cast<AudioDeviceOutDataModel*>(userData);
            return self->processAudio(outputBuffer, framesPerBuffer);
        }

        int processAudio(void* outputBuffer, unsigned long framesPerBuffer) {
            QMutexLocker locker(&mutex);

            if (!isPlaying) {
                memset(outputBuffer, 0, framesPerBuffer * CHANNELS * BYTES_PER_SAMPLE);
                return paContinue;
            }

            // 如果没有足够的数据，输出静音
            if (audioQueue.size() < MIN_BUFFERED_FRAMES) {
                memset(outputBuffer, 0, framesPerBuffer * CHANNELS * BYTES_PER_SAMPLE);
                return paContinue;
            }

            AudioFrame frame = audioQueue.dequeue();

            // 更新缓存数量显示
            updateBufferCount();

            // 计算要复制的字节数
            size_t bytesPerFrame = CHANNELS * BYTES_PER_SAMPLE;
            size_t bytesToCopy = std::min(
                static_cast<size_t>(frame.data.size()),
                framesPerBuffer * bytesPerFrame
            );

            // 复制数据到输出缓冲区
            memcpy(outputBuffer, frame.data.constData(), bytesToCopy);

            // 如果数据不足，填充静音
            if (bytesToCopy < framesPerBuffer * bytesPerFrame) {
                memset(static_cast<char*>(outputBuffer) + bytesToCopy, 0,
                       (framesPerBuffer * bytesPerFrame) - bytesToCopy);
            }

            return paContinue;
        }

    private slots:
        void onDeviceChanged(const QString& deviceName) {
            if(isPlaying){
                stopAudio();
            }
            startAudio();
        }

    private:
        void updateBufferCount() {
            // 在主线程中更新UI
            QMetaObject::invokeMethod(widget->bufferCountLabel, [this]() {
                widget->bufferCountLabel->setText(QString("Buffered: %1/%2")
                    .arg(audioQueue.size())
                    .arg(MAX_QUEUE_SIZE));
            }, Qt::QueuedConnection);
        }

        static const size_t BUFFER_SIZE = 4096;  // 固定的缓冲区大小
        static const size_t MAX_QUEUE_SIZE = 5;  // 最大队列长度
        static const size_t MIN_BUFFERED_FRAMES = 2;  // 开始播放所需的最小缓冲帧数
        static const int SAMPLE_RATE = 48000;    // 采样率
        static const int CHANNELS = 2;           // 通道数
        static const int BYTES_PER_SAMPLE = 2;   // 每个样本的字节数 (16位)

        bool isPlaying = false;
        AudioDeviceOutInterface* widget = new AudioDeviceOutInterface();
        PaStream* paStream = nullptr;

        QQueue<AudioFrame> audioQueue;
        QMutex mutex;
        QWaitCondition condition;
    };
}
//==============================================================================

