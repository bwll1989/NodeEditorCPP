#pragma once

#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <QTimer>
#include <QDebug>
#include <iostream>
#include <QPushButton>
#include "PluginDefinition.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "AudioDeviceInInterface.hpp"
#include "DataTypes/AudioTimestampRingQueue.h"
#include <portaudio.h>
#include <memory>
#include <map>
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    /**
     * @brief 音频输入设备节点数据模型
     * 负责从指定的音频输入设备录制音频并输出AudioData
     */
    class AudioDeviceInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double gainDb READ gainDb WRITE setGainDb NOTIFY gainDbChanged)

    public:
        /**
         * @brief 构造函数
         */
        AudioDeviceInDataModel() {
            
            InPortCount = 0;
            OutPortCount = 2; // 默认立体声输出
            CaptionVisible = true;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;

            AbstractDelegateModel::registerExternalControl("/gain", widget->volume_spinbox);

            connect(widget->volume_spinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this](double v){ setGainDb(v); });

            connect(widget->device_selector, &QComboBox::currentIndexChanged,
                    this, &AudioDeviceInDataModel::onDeviceChanged);

            get_device_list();
        }

        /**
         * @brief 析构函数
         */
        ~AudioDeviceInDataModel() override {
            stopRecording();
        }
        /**
         * @brief 获取端口数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::Out) {
                return AudioData().type();
            }
            return AudioData().type();
        }

        /**
         * @brief 获取输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            if (!channelAudioBuffers_[port])
            {
                channelAudioBuffers_[port]=std::make_shared<AudioTimestampRingQueue>();
            }
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(channelAudioBuffers_[port]);
            return audioData;
        }

        /**
         * @brief 设置输入数据（此节点无输入）
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(data)
            Q_UNUSED(portIndex)
        }
        
        /**
         * @brief 获取嵌入式界面组件
         */
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
         * @brief 保存节点配置
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson;
            modelJson["selectedDevice"] =widget->device_selector->currentIndex();
            modelJson["volume"] = gainDb();
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         */
        void load(const QJsonObject &p) override
        {
            if (p.contains("selectedDevice")) {
                widget->device_selector->setCurrentIndex(p["selectedDevice"].toInt());
            }
            if (p.contains("volume")) {
                setGainDb(p["volume"].toDouble());
            }
        }

    public slots:
        /**
         * @brief 获取音频输入设备列表
         */
        void get_device_list() {
            PaError err = Pa_Initialize();
            if (err != paNoError) {
                qDebug() << "PortAudio初始化失败:" << Pa_GetErrorText(err);
                return;
            }
            
            widget->device_selector->clear();
            int numDevices = Pa_GetDeviceCount();
            
            if (numDevices < 0) {
                qDebug() << "获取设备数量失败";
                Pa_Terminate();
                return;
            }
            // 获取默认输入设备索引
            PaDeviceIndex defaultDevice = Pa_GetDefaultInputDevice();
            int defaultComboIndex = -1;
            for (int i = 0; i < numDevices; i++) {
                const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
                if (deviceInfo != nullptr && deviceInfo->maxInputChannels > 0) {
                    QString deviceName = QString("%1: %2 (输入通道: %3)")
                                        .arg(i)
                                        .arg(deviceInfo->name)
                                        .arg(deviceInfo->maxInputChannels);
                    widget->device_selector->addItem(deviceName, i);
                     // 记录默认设备在下拉框中的索引
                    if (i == defaultDevice) {
                        defaultComboIndex = widget->device_selector->count() - 1;
                    }
                }
            }
            Pa_Terminate();
            widget->device_selector->setCurrentIndex(defaultComboIndex);
        }
        
        /**
         * @brief 设备选择变化处理
         */
        void onDeviceChanged(int index) {
            if (index >= 0) {
                selectedDeviceIndex_ = widget->device_selector->itemData(index).toInt();
                const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(selectedDeviceIndex_);
                // 更新输出端口数量以匹配通道数
                channels_=deviceInfo->maxInputChannels;
                // 如果正在录制，重启录制
                if (isRecording_) {
                    stopRecording();
                    startRecording();
                }else
                {
                    startRecording();
                }
            }
        }
        
        /**
         * @brief 音量变化处理（分贝值）
         */
        void onVolumeChanged(double dbValue) {
            setGainDb(dbValue);
        }

        /**
         * 函数级注释：获取当前输入设备增益（分贝）属性
         */
        double gainDb() const
        {
            return volumeDb_;
        }

        /**
         * 函数级注释：设置当前输入设备增益（分贝）属性，触发状态反馈
         */
        void setGainDb(double dbValue)
        {
            if (qFuzzyCompare(dbValue + 1.0, volumeDb_ + 1.0)) {
                return;
            }
            {
                QSignalBlocker blocker(widget->volume_spinbox);
                widget->volume_spinbox->setValue(dbValue);
            }
            volumeDb_ = dbValue;
            if (dbValue <= -60.0) {
                volumeGain_ = 0.0f;
            } else {
                volumeGain_ = std::pow(10.0f, dbValue / 20.0f);
            }
            Q_EMIT gainDbChanged(volumeDb_);
            AbstractDelegateModel::stateFeedBack("/gain", volumeDb_);
        }
            
        
        /**
         * @brief 开始录制音频
         */
        void startRecording() {
            if (isRecording_) {
                return;
            }
            
            PaError err = Pa_Initialize();
            if (err != paNoError) {
                qWarning() << "PortAudio初始化失败:" << Pa_GetErrorText(err);
                return;
            }
            
            // 配置输入参数
            PaStreamParameters inputParameters;
            inputParameters.device = selectedDeviceIndex_;
            inputParameters.channelCount = channels_;
            inputParameters.sampleFormat = paFloat32;
            inputParameters.suggestedLatency = Pa_GetDeviceInfo(selectedDeviceIndex_)->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = nullptr;
            
            // 打开音频流
            err = Pa_OpenStream(&paStream_,
                               &inputParameters,
                               nullptr, // 无输出
                               sampleRate_,
                               framesPerBuffer_,
                               paClipOff,
                               &AudioDeviceInDataModel::paCallback,
                               this);
            
            if (err != paNoError) {
                qWarning() << "打开音频流失败:" << Pa_GetErrorText(err);
                Pa_Terminate();
                return;
            }
            
            // 启动音频流
            err = Pa_StartStream(paStream_);
            if (err != paNoError) {
                qWarning() << "启动音频流失败:" << Pa_GetErrorText(err);
                Pa_CloseStream(paStream_);
                Pa_Terminate();
                return;
            }

            isRecording_ = true;
            // qDebug() << "开始录制音频，设备:" << selectedDeviceIndex_;
        }
        
        /**
         * @brief 停止录制音频
         */
        void stopRecording() {
            if (!isRecording_) {
                return;
            }
            
            if (paStream_) {
                Pa_StopStream(paStream_);
                Pa_CloseStream(paStream_);
                paStream_ = nullptr;
            }
            
            Pa_Terminate();
            isRecording_ = false;
            // qDebug() << "停止录制音频";
        }

    signals:
        /**
         * 函数级注释：当输入设备增益属性发生变化时发出的通知信号
         */
        void gainDbChanged(double db);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，实现外部增益控制
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/gain"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        /**
         * 函数级注释：处理来自全局事件总线的增益命令，更新 gainDb 属性
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/gain")) {
                return;
            }
            setGainDb(ev.payload.toDouble());
        }

    private:

        
        /**
         * @brief PortAudio回调函数
         */
        static int paCallback(const void* inputBuffer, void* outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void* userData) {
            Q_UNUSED(outputBuffer)
            Q_UNUSED(timeInfo)
            Q_UNUSED(statusFlags)
            
            AudioDeviceInDataModel* model = static_cast<AudioDeviceInDataModel*>(userData);
            return model->processAudioInput(inputBuffer, framesPerBuffer);
        }
        
        /**
         * @brief 处理音频输入数据
         */
        int processAudioInput(const void* inputBuffer, unsigned long framesPerBuffer) {

            if (!inputBuffer || !isRecording_) {
                return paContinue;
            }
            
            const float* input = static_cast<const float*>(inputBuffer);
            qint64 currentTimestamp = TimestampGenerator::getInstance()->getCurrentFrameCount();
            // 创建音频帧
            AudioFrame frame;
            frame.sampleRate = sampleRate_;
            frame.channels = channels_;
            frame.bitsPerSample = 32;
            frame.timestamp = currentTimestamp+5;
            
            // 复制音频数据并应用音量
            int totalSamples = framesPerBuffer * channels_;
            frame.data.resize(totalSamples * sizeof(float));
            float* output = reinterpret_cast<float*>(frame.data.data());
            
            for (int i = 0; i < totalSamples; i++) {
                float scaledSample = input[i] * volumeGain_;
                // 削波保护
                if (scaledSample > 1.0f) {
                    output[i] = 1.0f;
                } else if (scaledSample < -1.0f) {
                    output[i] = -1.0f;
                } else {
                    output[i] = scaledSample;
                }
            }
            
            // 分离声道并推送到对应缓冲区
            separateChannelsAndPush(frame);
            
            return paContinue;
        }
        
        /**
         * @brief 分离声道并推送到对应的音频缓冲区
         */
        void separateChannelsAndPush(const AudioFrame& frame) {
            int samplesPerChannel = frame.data.size() / (sizeof(float) * frame.channels);
            int bytesPerSample = sizeof(float);

            for (int channel = 0; channel < frame.channels; channel++) {
                if (!channelAudioBuffers_[channel])
                    channelAudioBuffers_[channel] = std::make_shared<AudioTimestampRingQueue>();
                AudioFrame channelFrame;
                channelFrame.sampleRate = frame.sampleRate;
                channelFrame.channels = 1; // 单声道
                channelFrame.bitsPerSample = frame.bitsPerSample;
                double frameDurationMs = (double)framesPerBuffer_ * 1000.0 / sampleRate_;
                channelFrame.timestamp = frame.timestamp;
                
                QByteArray channelData;
                channelData.resize(samplesPerChannel * bytesPerSample);
                
                const float* inputData = reinterpret_cast<const float*>(frame.data.constData());
                float* outputData = reinterpret_cast<float*>(channelData.data());
                
                // 提取指定声道的数据
                for (int sample = 0; sample < samplesPerChannel; sample++) {
                    outputData[sample] = inputData[sample * frame.channels + channel];
                }
                
                channelFrame.data = channelData;
                channelAudioBuffers_[channel]->pushFrame(channelFrame);
            }
        }

    private:
        // 界面组件
        AudioDeviceInInterface *widget = new AudioDeviceInInterface();
        
        // 音频参数
        int sampleRate_ = 48000;
        int framesPerBuffer_ = sampleRate_/TimestampGenerator::getInstance()->getFrameRate();
        int channels_ = 2;
        
        // 设备和状态
        PaDeviceIndex selectedDeviceIndex_ = Pa_GetDefaultInputDevice();
        bool isRecording_ = false;
        PaStream* paStream_ = nullptr;
        
        // 音量控制
        double volumeDb_ = 0.0; // 分贝值
        float volumeGain_ = 1.0f; // 线性增益
        
        // 音频缓冲区
        std::map<int, std::shared_ptr<AudioTimestampRingQueue>> channelAudioBuffers_;
    };
}
