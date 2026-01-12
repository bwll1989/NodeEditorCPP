#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "DataTypes/NodeDataList.hpp"
#include "VideoDecoderInterface.hpp"
#include "QFileDialog"
#include <memory>

#include "QtNodes/Definitions"

#include "QTimer"
#include "VideoDecoder.hpp"
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
    class VideoDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    public:
        /**
       * @brief 构造函数，初始化音频解码Node，支持动态多通道分离输出
       */
        VideoDecoderDataModel(){
            InPortCount = 4;
            OutPortCount = 3;  // 初始输出端口数，可动态调整
            CaptionVisible = true;
            Caption = "Video Decoder";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;


            // connect(widget->fileSelectButton,&QPushButton::clicked,this,&VideoDecoderDataModel::select_audio_file,Qt::QueuedConnection);
            connect(widget->fileSelectComboBox,&SelectorComboBox::textChanged,this,&VideoDecoderDataModel::select_audio_file,Qt::QueuedConnection);
            connect(widget->playButton,&QPushButton::clicked,this,&VideoDecoderDataModel::playAudio,Qt::QueuedConnection);
            connect(widget->stopButton,&QPushButton::clicked,this,&VideoDecoderDataModel::stopAudio,Qt::QueuedConnection);
            connect(player, &VideoDecoder::videoFrameReady, this, &VideoDecoderDataModel::onVideoFrameReady, Qt::QueuedConnection);
            // 新增信号连接
            connect(widget->volumeSlider, &QDoubleSpinBox::valueChanged,
                    this, &VideoDecoderDataModel::onVolumeChanged, Qt::QueuedConnection);
            connect(widget->loopCheckBox, &QCheckBox::toggled,
                    this, &VideoDecoderDataModel::onLoopToggled, Qt::QueuedConnection);
            connect(player, &VideoDecoder::playbackProgress,
                    this, &VideoDecoderDataModel::onPlaybackProgress, Qt::QueuedConnection);

            widget->volumeSlider->setValue(-10.0);

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
        ~VideoDecoderDataModel(){
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
                    if (portIndex == 0)
                        return ImageData().type();
                    else
                        return AudioData().type();
                default:
                    return VariableData().type();
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
            if (port == 0) {
                return lastVideoFrame;
            }
            
            // 创建新的AudioData并设置共享环形缓冲区
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(player->getAudioBuffer(port - 1));

            return audioData;
        }


        /**
         * @brief 获取端口显示的标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
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
                    if (portIndex == 0) return "Image";
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
                        QTimer::singleShot(100, this, &VideoDecoderDataModel::playAudio);
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
                
                // 动态更新端口数量
                unsigned int channels = player->getChannels();
                unsigned int newOutPortCount = 1 + channels; // 1 Video + N Audio
                
                if (newOutPortCount != OutPortCount) {
                    if (newOutPortCount > OutPortCount) {
                        emit portsAboutToBeInserted(PortType::Out, OutPortCount, newOutPortCount - OutPortCount);
                        OutPortCount = newOutPortCount;
                        emit portsInserted();
                    } else {
                        emit portsAboutToBeDeleted(PortType::Out, newOutPortCount, OutPortCount - newOutPortCount);
                        OutPortCount = newOutPortCount;
                        emit portsDeleted();
                    }
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

        void onVideoFrameReady(NodeDataTypes::ImageData frame) {
            lastVideoFrame = std::make_shared<NodeDataTypes::ImageData>(frame);
            emit dataUpdated(0);
        }

        void onPlaybackProgress(double currentSec, double totalSec) {
            if (totalSec > 0) {
                int value = static_cast<int>((currentSec / totalSec) * 1000);
                widget->progressSlider->blockSignals(true);
                widget->progressSlider->setValue(value);
                widget->progressSlider->blockSignals(false);
            }
            
            QString currentTimeStr = formatTime(currentSec);
            QString totalTimeStr = formatTime(totalSec);
            widget->timeLabel->setText(currentTimeStr + " / " + totalTimeStr);
        }

    private:
        /**
         * @brief 格式化时间显示 (MM:SS)
         * @param seconds 秒数
         * @return 格式化后的时间字符串
         */
        QString formatTime(double seconds) {
            int m = static_cast<int>(seconds) / 60;
            int s = static_cast<int>(seconds) % 60;
            return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        }

        std::shared_ptr<NodeDataTypes::ImageData> lastVideoFrame;

        float currentVolume = -10.0f;  // 新增当前音量记录

        VideoDecoderInterface *widget=new VideoDecoderInterface();
        //    界面控件
        VideoDecoder *player=new VideoDecoder();
        QString filePath="";
        bool isLoop=false;
        bool autoPlay=false;
        bool isReady= false;

    };
}

