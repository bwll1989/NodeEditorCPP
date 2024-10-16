#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "../NodeDataList.hpp"
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
    AudioDecoderDataModel():Data(std::make_shared<AudioNodeData2>()){
        InPortCount =3;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Audio Decoder";
        WidgetEmbeddable= false;
        Resizable=false;
        PortEditable= false;

        connect(widget->button,&QPushButton::clicked,this,&AudioDecoderDataModel::select_audio_file,Qt::QueuedConnection);
        connect(widget->button1,&QPushButton::clicked,this,&AudioDecoderDataModel::playAudio,Qt::QueuedConnection);
        connect(widget->button2,&QPushButton::clicked,this,&AudioDecoderDataModel::stopAudio,Qt::QueuedConnection);

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

    ~AudioDecoderDataModel(){
        if (player->isPlaying){
            player->stopPlay();
        }
    }

    unsigned int nPorts(PortType portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }
        return result;
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
                return AudioNodeData2().type();
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
        return Data;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        switch (portIndex) {
            case 0: {
                auto d = std::dynamic_pointer_cast<BoolData>(data);
                if (d != nullptr) {
                    if (d->NodeValues == true) {
                        this->playAudio();
                    } else {

                    }
                }
                return;
            }
            case 1: {
                auto d = std::dynamic_pointer_cast<StringData>(data);
                if (d != nullptr) {
                   filePath=d->NodeValues;
                }
            }
            case 2:{
                auto d = std::dynamic_pointer_cast<FloatData>(data);
                if (d != nullptr) {

                }
            }
        }

        Q_EMIT dataUpdated(0);
    }

    QWidget *embeddedWidget() override
    {

        QFileInfo *fileInfo=new QFileInfo(filePath);
        widget->namelabel->setText(fileInfo->fileName());

//        widget->setMinimumSize(100, 50);
//        widget->setMaximumSize(200,100);
        delete fileInfo;
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
//            for(int i=0;i<res->count();i++){
//
//                nameItem->setText(0,res->keys()[i]);
//                nameItem->setText(1,res->value(res->keys()[i]).toString());
//            }
            widget->treeWidget->setModel(resout);
//        准备音频文件
        }
        delete fileDialog;
    }

//    开始播放
    void playAudio() {



        if(isReady&&!player->initializePortAudio())
        {
            return;
        }
        player->startPlay();
    }

    void stopAudio(){
        player->stopPlay();
    }

private:
    std::shared_ptr<AudioNodeData2> Data;
    AudioDecoderInterface *widget=new AudioDecoderInterface();
//    界面控件
    AudioDecoder *player=new AudioDecoder();
    QString filePath="";
    bool isLoop=false;
    bool autoPlay=false;
    bool isReady= false;


};
//==============================================================================

