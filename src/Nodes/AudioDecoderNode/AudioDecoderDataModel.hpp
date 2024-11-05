#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "DataTypes/NodeDataList.hpp"
#include "AudioDecoderInterface.hpp"
#include "QFileDialog"
#include <memory>
#include "portaudio.h"
#include "QtNodes/Definitions"
#include "QMessageBox"
#include "QTimer"
#include "AudioDecoder.hpp"
#include "QThread"
#include "Common/GUI/QJsonModel/QJsonModel.hpp"
using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;



//AudioPlayDataModel *AudioPlayDataModel::audioPlayDataModel=NULL;

class AudioDecoderDataModel : public NodeDelegateModel
{
    Q_OBJECT
public:
    AudioDecoderDataModel():AudioData(std::make_shared<AudioData2>()){
        InPortCount =3;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Audio Decoder";
        WidgetEmbeddable= false;
        Resizable=false;
        PortEditable= true;
        AudioData->pipe=pi;
        connect(widget->button,&QPushButton::clicked,this,&AudioDecoderDataModel::select_audio_file,Qt::QueuedConnection);
        connect(widget->button1,&QPushButton::clicked,this,&AudioDecoderDataModel::playAudio,Qt::QueuedConnection);
        connect(widget->button2,&QPushButton::clicked,this,&AudioDecoderDataModel::stopAudio,Qt::QueuedConnection);

    }


public:

    ~AudioDecoderDataModel(){
        if (player->isPlaying){
            player->stopPlay();
        }
    }


    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return AudioData2().type();
            case PortType::None:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        Q_UNUSED(port)
        return AudioData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        switch (portIndex) {
            case 0: {
                auto d = std::dynamic_pointer_cast<VariableData>(data);
                if (d != nullptr) {
                    if (d->value().toBool() == true) {
                        this->playAudio();
                    } else {

                    }
                }
                return;
            }
            case 1: {
                auto d = std::dynamic_pointer_cast<VariableData>(data);
                if (d != nullptr) {
                   filePath=d->value().toString();
                }
            }
            case 2:{
                auto d = std::dynamic_pointer_cast<VariableData>(data);
                if (d != nullptr) {

                }
            }
        }

        Q_EMIT dataUpdated(0);
    }

    QWidget *embeddedWidget() override
    {

        return widget;
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;

        modelJson1["path"] = filePath;
        modelJson1["isLoop"] = isLoop;
        modelJson1["autoPlay"] = autoPlay;

        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {

            filePath = v["path"].toString();
            isLoop = v["isLoop"].toBool();
            autoPlay = v["autoPlay"].toBool();
        }
    }

public
    slots:
    //选择媒体文件，支持WAV和mp3
    void select_audio_file()
    {

        QFileDialog *fileDialog=new QFileDialog();

        QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                tr("Select WAV or MP3 File"), "/home", tr("Audio Files (*.wav *.mp3)"));
        if(fileName!="")
        {
            filePath=fileName;

            auto res=player->initializeFFmpeg(filePath);
            if(!res){
                isReady= false;
                return;
            }
            isReady= true;

            QJsonModel *resout=new QJsonModel(*res);
            widget->treeWidget->setModel(resout);
//        准备音频文件
        }
        delete fileDialog;
    }

//    开始播放
    void playAudio() {

        if(isReady)
            // if(isReady&&!player->initializePortAudio())
        {
            player->startPlay();
            Q_EMIT dataUpdated(0);
        }

    }

    void stopAudio(){
        player->stopPlay();
    }

private:
    AudioPipe *pi=new AudioPipe;

    std::shared_ptr<AudioData2> AudioData;

    AudioDecoderInterface *widget=new AudioDecoderInterface();
//    界面控件
    AudioDecoder *player=new AudioDecoder(pi);
    QString filePath="";
    bool isLoop=false;
    bool autoPlay=false;
    bool isReady= false;


};
//==============================================================================

