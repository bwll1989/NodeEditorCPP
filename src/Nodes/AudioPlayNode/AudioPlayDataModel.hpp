#pragma once

#include "QtNodes/NodeDelegateModel"

#include <QtCore/QObject>

#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "../NodeDataList.hpp"
#include "AudioPlayInterface.hpp"
#include "QFileDialog"
#include "sndfile.h"
#include <memory>
#include "portaudio.h"
#include "QtNodes/Definitions"
#include "sndfile.hh"
#include "QBuffer"
#include "QThread"
#include "QMessageBox"
#include "QTimer"
using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;


typedef struct {
    SNDFILE* sndFile;
    SF_INFO sfInfo;
    sf_count_t currentFrame;
} CallbackData;
//AudioPlayDataModel *AudioPlayDataModel::audioPlayDataModel=NULL;

class AudioPlayDataModel : public NodeDelegateModel
{
    Q_OBJECT
public:
    AudioPlayDataModel():Data(std::make_shared<AudioNodeData>()) {
        InPortCount =3;
        OutPortCount=1;
        CaptionVisible= false;
        Caption="Audio Play";
        WidgetEmbeddable=false;
        PortEditable= false;
        Resizable=false;
        connect(widget->button,&QPushButton::clicked,this,&AudioPlayDataModel::select_audio_file);
        connect(widget->button1,&QPushButton::clicked,this,&AudioPlayDataModel::playAudio);
        connect(widget->button2, &QPushButton::clicked, this, &AudioPlayDataModel::stopAudio);
        connect(widget->button3, &QPushButton::clicked, this, &AudioPlayDataModel::pauseAudio);
        connect(seekUpdateTimer, &QTimer::timeout, this, &AudioPlayDataModel::updateSliderTime);

        // Update every 100 milliseconds
        deviceListInit();

    }

//    bool widgetEmbeddable() const override { return false; }

    ~AudioPlayDataModel(){
        seekUpdateTimer->deleteLater();
        stopAudio();
        sf_close(callbackData.sndFile);
        Pa_Terminate();
        widget->deleteLater();
    }
public:
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = " "+QString::number(portIndex);
        QString out = QString::number(portIndex)+"  ";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }
    unsigned int nPorts(PortType portType) const override
    {

        switch (portType) {
            case PortType::In:
                return InPortCount;

            case PortType::Out:
                return OutPortCount;

            default:
                break;
        }

        return 0;
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
            case PortType::In:
                switch (portIndex) {
                    case 0:
                        return BoolData().type();
                    case 1:
                        return StringData().type();
                    case 2:
                        return FloatData().type();
                }
                break;


            case PortType::Out:
                return AudioNodeData().type();
                break;

            case PortType::None:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return Data->type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        Q_UNUSED(port)
        return Data;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        switch (portIndex) {
            case 0: {
                bool play = false;
                if (auto d = std::dynamic_pointer_cast<VariantData>(data)) {
                    if (d->NodeValues.canConvert<bool>()) {
                        play = d->NodeValues == true;
                    }
                } else if (auto d = std::dynamic_pointer_cast<BoolData>(data)) {
                    play = d->NodeValues;
                }

                if (play) {
                    Data->NodeValues.play = true;
                    this->playAudio();
                } else {
                    this->stopAudio();
                    Data->NodeValues.play = false;
                }
                return;
            }
            case 1: {
                auto d = std::dynamic_pointer_cast<StringData>(data);
                if (d != nullptr) {
                   Data->NodeValues.path=d->NodeValues;
                }
            }
            case 2:{
                auto d = std::dynamic_pointer_cast<FloatData>(data);
                if (d != nullptr) {
                    Data->NodeValues.gain=d->NodeValues;
                }
            }
        }

        Q_EMIT dataUpdated(0);
    }
    QWidget *embeddedWidget() override
    {
        widget->namelabel->setText(Data->NodeValues.name);
//        widget->setMinimumSize(100, 50);
//        widget->setMaximumSize(200,100);
        return widget;
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["name"] = Data->NodeValues.name;
        modelJson1["path"] = Data->NodeValues.path;
        modelJson1["play"] = Data->NodeValues.play;
        modelJson1["gain"] = Data->NodeValues.gain;

        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {

            Data->NodeValues.name = v["name"].toString();
            Data->NodeValues.path = v["path"].toString();
            Data->NodeValues.play = v["play"].toBool();
            Data->NodeValues.gain = v["gain"].toDouble();


        }

//        更新界面

    }



public
    slots:
    //选择媒体文件，支持WAV和mp3
    void select_audio_file()
    {

        QFileDialog fileDialog;

        QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                tr("Select WAV or MP3 File"), "/home", tr("Audio Files (*.wav *.mp3)"));
        if(fileName!="")
        {
            Data->NodeValues.path=fileName;
            QFileInfo fileInfo(fileName);
            Data->NodeValues.name=fileInfo.fileName();
//            刷新node控件显示
            this->embeddedWidget();
//        准备音频文件
            prepareAudio();
        }
    }

//    准备音频
    bool prepareAudio() {
        std::string str = Data->NodeValues.path.toStdString();
        const char *filePath = str.c_str();  // Replace with your audio file path

        // Open the audio file using libsndfile
        callbackData.sndFile = sf_open(filePath, SFM_READ, &callbackData.sfInfo);

        if (!callbackData.sndFile) {
//            文件打开失败
            widget->statusLabel->setText(QString(sf_strerror(NULL)));
            qDebug() << QString(sf_strerror(NULL));
            return false;
        }
        return true;
    }
//    准备音频设备
    bool prepareDevice(){
        OutPortCount=callbackData.sfInfo.channels;
        if(widget->device_selector->currentIndex()==0)
            outputParameters.device =Pa_GetDefaultOutputDevice(); /* Use the default output device */
        else
        {
            outputParameters.device = widget->device_selector->currentText().split(":").at(0).toInt();
            qDebug()<<widget->device_selector->currentText().split(":").at(0).toInt();
        }

        outputParameters.channelCount = callbackData.sfInfo.channels;
        outputParameters.sampleFormat = paFloat32;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        // Open PortAudio stream
        PaError paError = Pa_OpenStream(&stream,
                                        nullptr,      // No input
                                        &outputParameters,
                                        callbackData.sfInfo.samplerate,
                                        paFramesPerBufferUnspecified,
                                        paNoFlag,
                                        &paCallback,
                                        &callbackData);

        if (paError != paNoError) {
//            打开流失败
            widget->statusLabel->setText(QString(Pa_GetErrorText(paError)));
            qDebug()<<QString(Pa_GetErrorText(paError));
            sf_close(callbackData.sndFile);
            return false;
        }
        return true;
    }

    void deviceListInit(){
//        widget->device_selector->clear();
        PaError paError;
        int     i, numDevices;
        const   PaDeviceInfo *deviceInfo;
        QStringList deviceList;
        paError = Pa_Initialize();
//        widget->device_selector->clear();
        if (paError != paNoError) {
            widget->statusLabel->setText(QString(Pa_GetErrorText(paError)));
            return;
        }
        numDevices = Pa_GetDeviceCount();
        if( numDevices < 0 )
        {
            widget->statusLabel->setText("NO AVAILABLE DEVICES");
            return;
        }
        for( i=0; i<numDevices; i++ )
        {
            deviceInfo = Pa_GetDeviceInfo( i );

            if (deviceInfo != nullptr && deviceInfo->maxOutputChannels>0)
            {
                deviceList << QString("%1: %2").arg(i).arg(deviceInfo->name);
            }
        }
        widget->device_selector->addItems(deviceList);

    }
//    开始播放
    void playAudio() {
        qDebug()<<"play";
        if(!prepareAudio())
            return;
        if(!prepareDevice())
            return;
        PaError paError = Pa_StartStream(stream);
        if (paError != paNoError) {
//            启动流失败
            qDebug()<<QString(Pa_GetErrorText(paError));
            widget->statusLabel->setText(QString(Pa_GetErrorText(paError)));
            Pa_CloseStream(stream);
            sf_close(callbackData.sndFile);
            return;
        }
//        所有流程正常时
        Data->NodeValues.play= true;
        widget->device_selector->setEnabled(false);
        seekUpdateTimer->start(100);
//        启动定时器
//        widget->during->setMaximum(callbackData.sfInfo.frames);
        widget->statusLabel->setText("Playing");
        qDebug()<<"start playing";
    }
//    停止播放
    void stopAudio() {
        if (Data->NodeValues.play) {
            Pa_StopStream(stream);
//            Pa_CloseStream(stream);
            sf_seek(callbackData.sndFile, 0, SEEK_SET);
            Data->NodeValues.play = false;
            widget->during->setValue(0);
            seekUpdateTimer->stop();
            qDebug()<<"stop playing";
            widget->device_selector->setEnabled(true);
        }
    }
//  暂停
    void pauseAudio(){
        Pa_StopStream(stream);
    }
//  更新进度条
    void updateSliderTime() {
        if (Data->NodeValues.play) {
            callbackData.currentFrame= sf_seek(callbackData.sndFile, 0, SEEK_CUR);
            sf_count_t totalFrames = callbackData.sfInfo.frames;
            sf_count_t currentFrame = callbackData.currentFrame;
            widget->during->setValue(callbackData.currentFrame/callbackData.sfInfo.frames);
            int totalSeconds = static_cast<int>(totalFrames / callbackData.sfInfo.samplerate);
            int currentSeconds = static_cast<int>(currentFrame / callbackData.sfInfo.samplerate);

            int totalMinutes = totalSeconds / 60;
            int currentMinutes = currentSeconds / 60;

            totalSeconds %= 60;
            currentSeconds %= 60;

            QString timeString = QString("%1:%2 / %3:%4").arg(currentMinutes, 2, 10, QChar('0')).arg(currentSeconds, 2, 10, QChar('0')).arg(totalMinutes, 2, 10, QChar('0')).arg(totalSeconds, 2, 10, QChar('0'));
            widget->timeLabel->setText(timeString);
        } else {
            widget->timeLabel->setText("0:00 / 0:00");
        }

    }

private:
    //播放回调函数
    static int paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData) {
        (void) inputBuffer;
        CallbackData* data = static_cast<CallbackData*>(userData);
//        定义输出缓存
        float* out = reinterpret_cast<float*>(outputBuffer);
//      临时缓存,申请临时缓存控件
        float *temBuf;
        temBuf = (float *)malloc(data->sfInfo.channels *sizeof(float )*framesPerBuffer);

        // 读取音频到临时缓存中
        sf_count_t readCount = sf_readf_float(data->sndFile, temBuf, framesPerBuffer);

        if (readCount < framesPerBuffer) {
            // If reached the end of the file, rewind for continuous playback
            sf_seek(data->sndFile, 0, SEEK_SET);
//            sf_readf_float(data->sndFile, out + readCount, framesPerBuffer - readCount);
        }
        data->currentFrame += readCount / data->sfInfo.channels;
//        处理音频
        processAudio(temBuf,sizeof(float) * framesPerBuffer*data->sfInfo.channels);
        memcpy(out, temBuf, sizeof(float) * framesPerBuffer*data->sfInfo.channels);
        return paContinue;
    }
//      音频处理
    static void processAudio(const float *buffer, unsigned long framesPerBuffer) {
        // 在这里添加你的音频处理逻辑，例如输出到另一个音频库、文件等
        // 这里只是简单地将音频数据打印到控制台

    }



    std::shared_ptr<AudioNodeData> Data;

    AudioPlayInterface *widget=new AudioPlayInterface();
//    界面控件
    //正在播放标志位
    CallbackData callbackData;
    //回调数据
    PaStream* stream;
//    音频流
    PaStreamParameters outputParameters;
//    portaudio设置
    QTimer *seekUpdateTimer = new QTimer(this);
//    界面更新定时器
};
//==============================================================================

