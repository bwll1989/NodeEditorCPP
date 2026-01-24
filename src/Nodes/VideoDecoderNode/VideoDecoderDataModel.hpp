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
#include "StatusContainer/GlobalEventBus.hpp"

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
        Q_PROPERTY(QString fileName READ getFileName WRITE setFileName NOTIFY fileNameChanged)
        Q_PROPERTY(bool loop READ getLoop WRITE setLoop NOTIFY loopChanged)
        Q_PROPERTY(double volume READ getVolume WRITE setVolume NOTIFY volumeChanged)

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


            // UI Connections
            connect(widget->fileSelectComboBox, &SelectorComboBox::textChanged, this, &VideoDecoderDataModel::setFileName);
            connect(widget->playButton, &QPushButton::clicked, this, &VideoDecoderDataModel::play);
            connect(widget->stopButton, &QPushButton::clicked, this, &VideoDecoderDataModel::stop);
            
            connect(widget->volumeSlider, &QDoubleSpinBox::valueChanged, this, &VideoDecoderDataModel::setVolume);
            connect(widget->loopCheckBox, &QCheckBox::toggled, this, &VideoDecoderDataModel::setLoop);

            // Player Connections
            connect(player, &VideoDecoder::videoFrameReady, this, &VideoDecoderDataModel::onVideoFrameReady, Qt::QueuedConnection);
            connect(player, &VideoDecoder::playbackProgress, this, &VideoDecoderDataModel::onPlaybackProgress, Qt::QueuedConnection);

            // Initial State
            widget->volumeSlider->setValue(-10.0);
            AbstractDelegateModel::registerExternalControl("/volume", widget->volumeSlider);
            AbstractDelegateModel::registerExternalControl("/loop", widget->loopCheckBox);
            AbstractDelegateModel::registerExternalControl("/play",widget->playButton);
            AbstractDelegateModel::registerExternalControl("/stop",widget->stopButton);
            AbstractDelegateModel::registerExternalControl("/file",widget->fileSelectComboBox);
            m_volume = -10.0;
        }

        /**
         * @brief 析构函数，释放资源
         */
        ~VideoDecoderDataModel(){
            if (player->getPlaying()){
                player->stopPlay();
            }
        }
        
        QString getFileName() const { return m_fileName; }
        void setFileName(const QString& fileName) {
            if (m_fileName == fileName) return;
            m_fileName = fileName;
            {
                QSignalBlocker blocker(widget->fileSelectComboBox);
                widget->fileSelectComboBox->setText(m_fileName);
            }
            loadVideoFile(m_fileName);
            
            emit fileNameChanged(m_fileName);
            AbstractDelegateModel::stateFeedBack("/file", m_fileName);
        }

        bool getLoop() const { return m_loop; }
        void setLoop(bool loop) {
            if (m_loop == loop) return;
            m_loop = loop;
            
            QSignalBlocker blocker(widget->loopCheckBox);
            widget->loopCheckBox->setChecked(m_loop);
            
            player->setLooping(m_loop);
            
            emit loopChanged(m_loop);
            AbstractDelegateModel::stateFeedBack("/loop", m_loop);
        }

        double getVolume() const { return m_volume; }
        void setVolume(double volume) {
            if (qFuzzyCompare(m_volume, volume)) return;
            m_volume = volume;
            
            QSignalBlocker blocker(widget->volumeSlider);
            widget->volumeSlider->setValue(m_volume);
            
            player->setVolume(m_volume);
            
            emit volumeChanged(m_volume);
            AbstractDelegateModel::stateFeedBack("/volume", m_volume);
        }
        
        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/file"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/volume"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
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
            auto d = std::dynamic_pointer_cast<VariableData>(data);
            if (d == nullptr) return;

            switch (portIndex) {
            case 0: 
                if (d->value().toBool()) play();
                break;
            case 1: 
                if (d->value().toBool()) stop();
                break;
            case 2:
                setLoop(d->value().toBool());
                break;
            case 3:
                setVolume(d->value().toDouble());
                break;
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
            modelJson["filePath"] = m_fileName;
            modelJson["isLoop"] = m_loop;
            modelJson["autoPlay"] = autoPlay;
            modelJson["volume"] = m_volume;
            return modelJson;
        }

        /**
     * @brief 加载节点状态
     */
        void load(const QJsonObject &p) override
        {
            QJsonObject modelJson = p;
            
            if (modelJson.contains("filePath")) {
                setFileName(modelJson["filePath"].toString());
            }
            
            if (modelJson.contains("isLoop")) {
                setLoop(modelJson["isLoop"].toBool());
            }
            
            if (modelJson.contains("autoPlay")) {
                autoPlay = modelJson["autoPlay"].toBool();
            }
            
            if (modelJson.contains("volume")) {
                setVolume(modelJson["volume"].toDouble());
            }

            // 如果有文件路径，重新初始化解码器
            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                // setFileName 已经处理了初始化逻辑
                if (isReady && autoPlay) {
                    QTimer::singleShot(100, this, &VideoDecoderDataModel::play);
                }
            }
        }

    public slots:

        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "file") setFileName(ev.payload.toString());
                else if (localPath == "loop") setLoop(ev.payload.toBool());
                else if (localPath == "volume") setVolume(ev.payload.toDouble());
                else if (localPath == "play") play();
                else if (localPath == "stop") stop();
            }
        }

        void loadVideoFile(QString fileName)
        {
            if(fileName != "")
            {
                filePath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + fileName;
                if (player->getPlaying()){
                    player->stopPlay();
                }
                auto res = player->initializeFFmpeg(filePath);

                if(!res){
                    isReady = false;
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
                
                isReady = true;
            }
        }

        /**
         * 播放音频
         */
        void play() {

            if (!isReady) {
                return;
            }

            player->stopPlay(); // 先停止之前的播放
            player->startPlay();
            AbstractDelegateModel::stateFeedBack("/play", true);
            

        }
        /**
         * 停止播放
         */
        void stop(){
            player->stopPlay();
            AbstractDelegateModel::stateFeedBack("/stop", true);
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

    signals:
        void fileNameChanged(QString fileName);
        void loopChanged(bool loop);
        void volumeChanged(double volume);

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

        VideoDecoderInterface *widget=new VideoDecoderInterface();
        //    界面控件
        VideoDecoder *player=new VideoDecoder();
        QString filePath=""; // Full path
        QString m_fileName=""; // File name (relative)
        bool m_loop=false;
        double m_volume = -10.0;
        bool autoPlay=false;
        bool isReady= false;

    };
}
