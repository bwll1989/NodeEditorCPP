#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "DataTypes/NodeDataList.hpp"
#include "AudioDecoderInterface.hpp"
#include "QFileDialog"
#include <memory>

#include "QtNodes/Definitions"

#include "QTimer"
#include "AudioDecoder.hpp"
#include "QThread"
// #include "Common/GUI/QJsonModel/QJsonModel.hpp"

using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;



//AudioPlayDataModel *AudioPlayDataModel::audioPlayDataModel=NULL;
using namespace NodeDataTypes;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///
namespace Nodes
{
    class AudioDecoderDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    public:
        /**
       * @brief 构造函数，初始化音频解码Node
       */
        AudioDecoderDataModel():audioData(std::make_shared<AudioData>()){
            InPortCount =3;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Audio Decoder";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= true;
            // AudioData->pipe=pi;
            qRegisterMetaType<AudioFrame>("AudioFrame");
            connect(player, &AudioDecoder::audioFrameReady,
                    this, &AudioDecoderDataModel::handleAudioFrame,
                    Qt::DirectConnection);
            connect(widget->button,&QPushButton::clicked,this,&AudioDecoderDataModel::select_audio_file,Qt::QueuedConnection);
            connect(widget->button1,&QPushButton::clicked,this,&AudioDecoderDataModel::playAudio,Qt::QueuedConnection);
            connect(widget->button2,&QPushButton::clicked,this,&AudioDecoderDataModel::stopAudio,Qt::QueuedConnection);
            registerOSCControl("/play",widget->button1);
            registerOSCControl("/stop",widget->button2);
        }

        /**
         * @brief 析构函数，释放资源
         */
        ~AudioDecoderDataModel(){
            if (player->isPlaying){
                player->stopPlay();
            }
        }

        /**
     * @brief 获取端口标题
     * @param portType 端口类型（输入/输出）
     * @param portIndex 端口索引
     * @return 端口标题字符串
     */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return AudioData().type();
            case PortType::None:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return VariableData().type();
        }
        /**
         * @brief 获取端口输出
         * @param portType 端口类型（输入/输出）
         * @return 端口数量
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            Q_UNUSED(port)
            return audioData;
        }
        /**
         * @brief 设置端口输入
         * @param data 输入数据
         * @param portIndex 端口索引
         */
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

    public slots:
        /**
         * 选则音频文件
         */
        void select_audio_file()
        {

            QFileDialog *fileDialog=new QFileDialog();

            QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Select WAV or MP3 File"), "/home", tr("Audio Files (*.wav *.mp3)"));
            if(fileName!="")
            {
                filePath=fileName;
                if (player->isPlaying){
                    player->stopPlay();
                }
                auto res=player->initializeFFmpeg(filePath);
                if(!res){
                    isReady= false;
                    return;
                }
                isReady= true;
                widget->fileDisplay->setText(filePath.split("/").last());
                // QJsonModel *resout=new QJsonModel(*res);
                // widget->treeWidget->setModel(resout);
                //        准备音频文件
            }
            delete fileDialog;
        }

        /**
         * 播放音频
         */
        void playAudio() {
            if (!isReady) {
                return;
            }

            if (audioData) {
                player->stopPlay(); // 先停止之前的播放
                player->startPlay();
                Q_EMIT dataUpdated(0);
            }
        }
        /**
         * 停止播放
         */
        void stopAudio(){
            player->stopPlay();
        }

    private slots:
        /**
         * 处理音频帧
         * @param frame 音频帧
         */
        void handleAudioFrame(AudioFrame frame) {
            if (audioData) {
                audioData->updateAudioFrame(frame);
                Q_EMIT dataUpdated(0);
            }
        }

    private:
        // AudioPipe *pi=new AudioPipe;

        std::shared_ptr<AudioData> audioData;

        AudioDecoderInterface *widget=new AudioDecoderInterface();
        //    界面控件
        AudioDecoder *player=new AudioDecoder();
        QString filePath="";
        bool isLoop=false;
        bool autoPlay=false;
        bool isReady= false;


    };
}
//==============================================================================

