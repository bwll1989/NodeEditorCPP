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
#include "Common/AppConfig/ConfigManager.h"
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

struct GlobalEvent;

namespace Nodes
{
    class AudioDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString file READ filePathProperty WRITE setFilePathProperty NOTIFY filePathChanged)
        Q_PROPERTY(bool loop READ loopProperty WRITE setLoopProperty NOTIFY loopChanged)
        Q_PROPERTY(double volume READ volumeProperty WRITE setVolumeProperty NOTIFY volumeChanged)
        Q_PROPERTY(bool playing READ playingProperty WRITE setPlayingProperty NOTIFY playingChanged)
    public:
        /**
       * @brief 构造函数，初始化音频解码Node，支持动态多通道分离输出
       */
        AudioDecoderDataModel(){
            InPortCount = 3;
            OutPortCount = 2;  // 初始输出端口数，可动态调整
            CaptionVisible = true;
            Caption = "Audio Decoder";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;

            AbstractDelegateModel::registerExternalControl("/file", widget->fileSelectComboBox);
            AbstractDelegateModel::registerExternalControl("/volume", widget->volumeSlider);
            AbstractDelegateModel::registerExternalControl("/loop", widget->loopCheckBox);
            AbstractDelegateModel::registerExternalControl("/play", widget->playButton);

            connect(widget->fileSelectComboBox, &SelectorComboBox::textChanged,
                    this, &AudioDecoderDataModel::setFilePathProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::filePathChanged, this, [this](const QString&){
                {
                    QSignalBlocker blocker(widget->fileSelectComboBox);
                    widget->fileSelectComboBox->setText(m_filePath);
                }
                if (!m_filePath.isEmpty()) {
                    if (player->getPlaying()) {
                        player->stopPlay();
                    }
                    auto res = player->initializeFFmpeg(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + m_filePath);
                    if (!res) {
                        isReady = false;
                        return;
                    }
                    isReady = true;
                }
                AbstractDelegateModel::stateFeedBack("/file", m_filePath);
            });

            connect(widget->volumeSlider, &FloatDragValueWidget::valueChanged,
                    this, &AudioDecoderDataModel::setVolumeProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::volumeChanged, this, [this](double){
                {
                    QSignalBlocker blocker(widget->volumeSlider);
                    widget->volumeSlider->setValue(m_volume);
                }
                player->setVolume(m_volume);
                AbstractDelegateModel::stateFeedBack("/volume", m_volume);
            });

            connect(widget->loopCheckBox, &QCheckBox::toggled,
                    this, &AudioDecoderDataModel::setLoopProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::loopChanged, this, [this](bool){
                {
                    QSignalBlocker blocker(widget->loopCheckBox);
                    widget->loopCheckBox->setChecked(m_loop);
                }
                player->setLooping(m_loop);
                AbstractDelegateModel::stateFeedBack("/loop", m_loop);
            });

            connect(widget->playButton, &QPushButton::toggled,
                    this, &AudioDecoderDataModel::setPlayingProperty, Qt::QueuedConnection);
            connect(this, &AudioDecoderDataModel::playingChanged, this, [this](bool playing){
                {
                    QSignalBlocker blocker(widget->playButton);
                    widget->playButton->setChecked(playing);
                }
                if (playing) {
                    if (!isReady) {
                        return;
                    }
                    player->stopPlay();
                    player->startPlay();
                } else {
                    player->stopPlay();
                }
                AbstractDelegateModel::stateFeedBack("/play", playing);
            });

            connect(player.get(), &AudioDecoder::playbackProgress,
                    this, &AudioDecoderDataModel::onPlaybackProgress, Qt::QueuedConnection);
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
                    return "LOOP";
                case 2:
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
                        setPlayingProperty(d->value().toBool());
                    }
                    return;
            }

            case 1:{
                    auto d = std::dynamic_pointer_cast<VariableData>(data);
                    if (d != nullptr)
                    {
                        widget->loopCheckBox->setChecked(d->value().toBool());
                    }
                    return;
            }
            case 2:{
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
            modelJson["filePath"] = m_filePath;
            modelJson["isLoop"] = loopProperty();
            modelJson["autoPlay"] = autoPlay;
            modelJson["volume"] = static_cast<int>(volumeProperty());
            return modelJson;
        }

        /**
     * @brief 加载节点状态
     */
        void load(const QJsonObject &p) override
        {
            QJsonObject modelJson = p;
            
            if (modelJson.contains("filePath")) {
                setFilePathProperty(modelJson["filePath"].toString());
            }
            
            if (modelJson.contains("isLoop")) {
                setLoopProperty(modelJson["isLoop"].toBool());
            }
            
            if (modelJson.contains("autoPlay")) {
                autoPlay = modelJson["autoPlay"].toBool();
            }
            
            if (modelJson.contains("volume")) {
                int volume = modelJson["volume"].toInt();
                setVolumeProperty(volume);
            }

            // 如果有文件路径，重新初始化解码器
            if (!m_filePath.isEmpty()) {
                auto res = player->initializeFFmpeg(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + m_filePath);
                if (res) {
                    isReady = true;
                    if (autoPlay) {
                        QTimer::singleShot(100, this, &AudioDecoderDataModel::playAudio);
                    }
                }
            }
        }

    public slots:
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

        /**
         * 选则音频文件
         */
        void select_audio_file(QString fileName)
        {
            if(fileName!="")
            {
                setFilePathProperty(fileName);
            }

        }

        /**
         * 播放音频
         */
        void playAudio() {
            setPlayingProperty(true);
        }
        /**
         * 停止播放
         */
        void stopAudio(){
            setPlayingProperty(false);
        }
        /**
         * @brief 音量改变槽函数
         * @param value 音量值 (0-100)
         */
        void onVolumeChanged(double value) {
            setVolumeProperty(value);
        }

        /**
         * @brief 循环播放切换槽函数
         * @param checked 是否启用循环播放
         */
        void onLoopToggled(bool checked) {
            setLoopProperty(checked);
            qDebug() << "循环播放:" << (checked ? "启用" : "禁用");
        }

        QString filePathProperty() const
        {
            return m_filePath;
        }

        void setFilePathProperty(const QString& relative)
        {
            const QString trimmed = relative.trimmed();
            if (trimmed == m_filePath) {
                return;
            }
            m_filePath = trimmed;
            Q_EMIT filePathChanged(trimmed);
        }

        bool loopProperty() const
        {
            return m_loop;
        }

        void setLoopProperty(bool loop)
        {
            if (loop == m_loop) {
                return;
            }
            m_loop = loop;
            Q_EMIT loopChanged(loop);
        }

        double volumeProperty() const
        {
            return m_volume;
        }

        void setVolumeProperty(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_volume + 1.0)) {
                return;
            }
            m_volume = value;
            Q_EMIT volumeChanged(value);
        }

        /**
         * @brief 获取当前播放状态属性
         * @return true 为正在播放，false 为停止
         */
        bool playingProperty() const
        {
            return m_playing;
        }

        /**
         * @brief 设置播放状态属性
         * @param playing true 开始播放，false 停止播放
         */
        void setPlayingProperty(bool playing)
        {
            if (playing == m_playing) {
                return;
            }
            if (playing && !isReady) {
                return;
            }
            m_playing = playing;
            Q_EMIT playingChanged(playing);
        }

    signals:
        void filePathChanged(const QString& path);
        void loopChanged(bool loop);
        void volumeChanged(double volume);
        void playingChanged(bool playing);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/file"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/loop"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/volume"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/play"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addrFile = makeFullOscAddress("/file");
            const QString addrLoop = makeFullOscAddress("/loop");
            const QString addrVolume = makeFullOscAddress("/volume");
            const QString addrPlay = makeFullOscAddress("/play");
            const QString addrStop = makeFullOscAddress("/stop");

            if (ev.address == addrFile) {
                setFilePathProperty(ev.payload.toString());
            } else if (ev.address == addrLoop) {
                setLoopProperty(ev.payload.toBool());
            } else if (ev.address == addrVolume) {
                setVolumeProperty(ev.payload.toDouble());
            } else if (ev.address == addrPlay) {
                setPlayingProperty(ev.payload.toBool());
            } else if (ev.address == addrStop) {
                if (ev.payload.toBool()) {
                    setPlayingProperty(false);
                }
            }
        }


    private:
        QString formatTime(double seconds) {
            int m = static_cast<int>(seconds) / 60;
            int s = static_cast<int>(seconds) % 60;
            return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        }

        AudioDecoderInterface *widget=new AudioDecoderInterface();
        //    界面控件
        std::shared_ptr<AudioDecoder> player=std::make_shared<AudioDecoder>();
        QString m_filePath="";
        bool m_loop=false;
        double m_volume = 0.0;
        bool m_playing = false;
        bool autoPlay=false;
        bool isReady= false;

    };
}
