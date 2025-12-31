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
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"

using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;

using namespace NodeDataTypes;

namespace Nodes
{
    class AudioDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    public:
        /**
       * @brief 构造函数，初始化音频解码Node，支持动态多通道分离输出
       */
        AudioDecoderDataModel(){
            InPortCount = 4;
            OutPortCount = 2;  // 初始输出端口数，可动态调整
            CaptionVisible = true;
            Caption = "Audio Decoder";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;


            // connect(widget->fileSelectButton,&QPushButton::clicked,this,&AudioDecoderDataModel::select_audio_file,Qt::QueuedConnection);
            connect(widget->fileSelectComboBox,&SelectorComboBox::textChanged,this,&AudioDecoderDataModel::select_audio_file,Qt::QueuedConnection);
            connect(widget->playButton,&QPushButton::clicked,this,&AudioDecoderDataModel::playAudio,Qt::QueuedConnection);
            connect(widget->stopButton,&QPushButton::clicked,this,&AudioDecoderDataModel::stopAudio,Qt::QueuedConnection);
            // 新增信号连接
            connect(widget->volumeSlider, &QDoubleSpinBox::valueChanged,
                    this, &AudioDecoderDataModel::onVolumeChanged, Qt::QueuedConnection);
            connect(widget->loopCheckBox, &QCheckBox::toggled,
                    this, &AudioDecoderDataModel::onLoopToggled, Qt::QueuedConnection);

            // 注册OSC控制
            AbstractDelegateModel::registerOSCControl("/volume", widget->volumeSlider);
            AbstractDelegateModel::registerOSCControl("/loop", widget->loopCheckBox);
            AbstractDelegateModel::registerOSCControl("/play",widget->playButton);
            AbstractDelegateModel::registerOSCControl("/stop",widget->stopButton);
            AbstractDelegateModel::registerOSCControl("/file",widget->fileSelectComboBox);
        }

        /**
         * @brief 析构函数，释放资源
         */
        ~AudioDecoderDataModel(){
            if (player->getPlaying()){
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

            return VariableData().type();
        }

        /**
         * @brief 获取指定端口的输出数据（建立连接时调用）
         * @param port 端口索引 (0-N对应不同声道)
         * @return 包含共享环形缓冲区的音频数据
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {

            
            // 创建新的AudioData并设置共享环形缓冲区
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(player->getAudioBuffer(port));

            return audioData;

        }


        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "PLAY";
                case 1:
                    return "STOP";
                case 2:
                    return "LOOP";
                case 3:
                    return "GAIN";
                default:
                    return "";
                }
            case PortType::Out:
                    return "CH "+QString::number(portIndex);
            default:
                return "";
            }


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
                        }
                    }
                    return;
            }
            case 1: {
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr)
                        if (d->value().toBool() == true) {
                            this->stopAudio();
                        }
                    return;
                }
            case 2:{
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr)
                    {
                        widget->loopCheckBox->setChecked(d->value().toBool());
                    }
                    return;
            }
            case 3:{
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr)
                    {
                        widget->volumeSlider->setValue(d->value().toDouble());
                    }
                    return;
            }
            }
        }

        QWidget *embeddedWidget() override
        {

            return widget;
        }

        /**
     * @brief 保存节点状态
     */
        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["filePath"] = widget->fileSelectComboBox->text();
            modelJson["isLoop"] = isLoop;
            modelJson["autoPlay"] = autoPlay;
            modelJson["volume"] = static_cast<int>(widget->volumeSlider->value());
            return modelJson;
        }

        /**
     * @brief 加载节点状态
     */
        void load(const QJsonObject &p) override
        {
            QJsonObject modelJson = p;
            
            if (modelJson.contains("filePath")) {
                widget->fileSelectComboBox->setText(modelJson["filePath"].toString());
            }
            
            if (modelJson.contains("isLoop")) {
                isLoop = modelJson["isLoop"].toBool();
                widget->loopCheckBox->setChecked(isLoop);
                player->setLooping(isLoop);
            }
            
            if (modelJson.contains("autoPlay")) {
                autoPlay = modelJson["autoPlay"].toBool();
            }
            
            if (modelJson.contains("volume")) {
                int volume = modelJson["volume"].toInt();
                widget->volumeSlider->setValue(volume);
                player->setVolume(volume / 100.0f);
            }

            // 如果有文件路径，重新初始化解码器
            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                auto res = player->initializeFFmpeg(filePath);
                if (res) {
                    isReady = true;
                    if (autoPlay) {
                        QTimer::singleShot(100, this, &AudioDecoderDataModel::playAudio);
                    }
                }
            }
        }

    public slots:
        /**
         * 选则音频文件
         */
        void select_audio_file(QString fileName)
        {

            // QFileDialog *fileDialog=new QFileDialog();
            //
            // QString fileName = QFileDialog::getOpenFileName(nullptr,
            //                                         tr("Select WAV or MP3 File"), "/home", tr("Audio Files (*.wav *.mp3)"));
            if(fileName!="")
            {
                filePath= AppConstants::MEDIA_LIBRARY_STORAGE_DIR+"/"+fileName;
                if (player->getPlaying()){
                    player->stopPlay();
                }
                auto res=player->initializeFFmpeg(filePath);

                if(!res){
                    isReady= false;
                    return;
                }
                isReady= true;
            }

        }

        /**
         * 播放音频
         */
        void playAudio() {

            if (!isReady) {
                return;
            }

            player->stopPlay(); // 先停止之前的播放
            player->startPlay();

        }
        /**
         * 停止播放
         */
        void stopAudio(){
            player->stopPlay();
        }
        /**
         * @brief 音量改变槽函数
         * @param value 音量值 (0-100)
         */
        void onVolumeChanged(double value) {
            player->setVolume(value);
        }

        /**
         * @brief 循环播放切换槽函数
         * @param checked 是否启用循环播放
         */
        void onLoopToggled(bool checked) {
            isLoop = checked;
            player->setLooping(checked);
            qDebug() << "循环播放:" << (checked ? "启用" : "禁用");
        }


    private:

        float currentVolume = 0.5f;  // 新增当前音量记录

        AudioDecoderInterface *widget=new AudioDecoderInterface();
        //    界面控件
        AudioDecoder *player=new AudioDecoder();
        QString filePath="";
        bool isLoop=false;
        bool autoPlay=false;
        bool isReady= false;

    };
}

