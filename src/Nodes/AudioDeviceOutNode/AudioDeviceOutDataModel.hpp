#pragma once

#include "QtNodes/NodeDelegateModel"

#include <QtCore/QObject>

#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "DataTypes/NodeDataList.hpp"
#include "AudioDeviceOutInterface.hpp"
#include <portaudio.h>
#include "Common/GUI/QJsonModel/QJsonModel.hpp"
#include "QThread"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;




class AudioDeviceOutDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    AudioDeviceOutDataModel():AudioData(std::make_shared<AudioData2>()) {
        InPortCount =2;
        OutPortCount=0;
        CaptionVisible= true;
        Caption="Audio Device Out";
        WidgetEmbeddable=true;
        PortEditable= true;
        Resizable=false;
        getDeviceList();
        connect(widget->device_selector,&QComboBox::currentTextChanged,this, &AudioDeviceOutDataModel::selectDevice);
    }

    ~AudioDeviceOutDataModel() override= default;

public:

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
            case PortType::In:
                return AudioData->type();
            case PortType::Out:
                break;
            case PortType::None:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return VariantData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        Q_UNUSED(port)
        return AudioData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        auto d = std::dynamic_pointer_cast<AudioData2>(data);

        if(d!= nullptr){
            AudioData = d;
            startProcessing();
        }


        // Q_EMIT dataUpdated(0);

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

    void getDeviceList(){
        PaError err;
        int     i, numDevices;
        QStringList deviceList;
        err = Pa_Initialize();
//        widget->device_selector->clear();
        if( err != paNoError )
        {
            widget->namelabel->setText("INIT ERROR");
            Pa_Terminate();
        }
        numDevices = Pa_GetDeviceCount();
        if( numDevices < 0 )
        {
            widget->namelabel->setText("NO DEVICES");
            Pa_Terminate();
        }
        for( i=0; i<numDevices; i++ )
        {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );

            if (deviceInfo != nullptr && deviceInfo->maxOutputChannels>0)
            {
                deviceList << QString("%1: %2").arg(i).arg(deviceInfo->name);

            }
        }
        widget->device_selector->addItems(deviceList);

    }
    void selectDevice(const QString device)
    {
        int deviceIndex=device.section(':', 0, 0).toInt();
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
        PaDeviceIndex defaultInputDevice = Pa_GetDefaultInputDevice();
        PaDeviceIndex defaultOutputDevice = Pa_GetDefaultOutputDevice();
        auto res= new QJsonObject();
        res->insert("Device Name",deviceInfo->name);
        res->insert("Driver Type",hostApiInfo->name);
        res->insert("Is Default Output Device",(deviceIndex == defaultOutputDevice) ? "Yes" : "No");
        res->insert("Max Output Channels",deviceInfo->maxOutputChannels);
        res->insert("Default Sample Rate",deviceInfo->defaultSampleRate);
        res->insert("Is Default Output Device",(deviceIndex == defaultOutputDevice) ? "Yes" : "No");
        res->insert("Is Default Input Device",(deviceIndex == defaultInputDevice) ? "Yes" : "No");
        // res->insert("Initialize PortAudio OK",initializePortAudio()?true:false);
        info=new QJsonModel(*res);
        widget->treeWidget->setModel(info);
        initializePortAudio();
    }

    bool initializePortAudio() {
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            qWarning() << "PortAudio error: " << Pa_GetErrorText(err);
            return false;
        }
        outputParameters.device = Pa_GetDefaultOutputDevice();
        outputParameters.channelCount = 2;
        outputParameters.sampleFormat = paInt16; // 假设输出是16位整数
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;
        return true;
    }

    bool startProcessing() {
        PaError err;

        err = Pa_OpenStream(&paStream,
                            nullptr, // 输入流
                            &outputParameters,
                            48000, // 采样率
                            paFramesPerBufferUnspecified, // 帧缓冲区大小
                            paClipOff, // 不进行剪切
                            audioCallback, // 回调函数
                            AudioData->pipe); // 用户数据
        if (err != paNoError) {
            qWarning() << "Error opening stream:" << Pa_GetErrorText(err);
            return false;
        }

        err = Pa_StartStream(paStream);
        if (err != paNoError) {
            qWarning() << "Error starting stream:" << Pa_GetErrorText(err);
            return false;
        }

        return true;
    }
private:
    PaStreamParameters outputParameters;
    bool isRunning=false;
    std::shared_ptr<AudioData2> AudioData;
    PaStream *paStream;
    AudioDeviceOutInterface *widget=new AudioDeviceOutInterface();
    QJsonModel *info;
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    /// static int audioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    static int audioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
        AudioPipe* audioProcessor = static_cast<AudioPipe*>(userData);
        std::vector<uint8_t> audioData = audioProcessor->popAudioData();

        // 如果有数据，拷贝到输出缓冲区
        if (!audioData.empty()) {
            size_t numSamples = audioData.size() / sizeof(int16_t); // 16-bit 整数
            memcpy(outputBuffer, audioData.data(), numSamples);
            return paContinue; // 继续流
        }

        // 如果没有数据，可以填充零以避免播放噪音
        memset(outputBuffer, 0, framesPerBuffer * sizeof(int16_t)); // 假设是16-bit
        return paContinue; // 继续流
    }
};

//==============================================================================

