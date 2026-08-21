#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "NodeDataList.hpp"
#include "VideoDecoderInterface.hpp"
#include "QFileDialog"
#include <memory>

#include "QtNodes/Definitions"

#include "QTimer"
#include "VideoDecoder.hpp"
#include "QThread"
#include "Common/AppConfig/ConfigManager.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "Common/DataTypes/ImageGpuUpload.h"

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
    /**
     * @file VideoDecoderDataModel.hpp
     * @brief QtNodes 视频解码节点模型：UI + 端口 + 与 VideoDecoder 引擎桥接
     *
     * ## 端口
     *
     * | 方向 | 索引 | 类型        | 说明                          |
     * |------|------|-------------|-------------------------------|
     * | In   | 0    | Variable    | PLAY/STOP（bool）             |
     * | In   | 1    | Variable    | STOP（bool，true 则停止）     |
     * | In   | 2    | Variable    | LOOP                          |
     * | In   | 3    | Variable    | GAIN（dB）                    |
     * | Out  | 0    | ImageData   | 视频帧（共享 Image ring buffer）|
     * | Out  | 1..N | AudioData   | 各声道（共享 Audio ring buffer）|
     *
     * ## 视频数据流（与 ImageLoader / Capture 一致）
     *
     * ```
     * VideoDecoder (videoLoop)
     *       → processVideoFrame / uploadPendingVideoFrame (GUI)
     *       → ImageTimestampRingQueue::pushFrame
     * m_outImageData ──共享句柄──→ 下游 WindowDisplay / ImageShow
     *       下游按 TimestampGenerator tick 调用 getLatestFrame()，无需 dataUpdated
     * ```
     *
     * ## 音频数据流
     *
     * VideoDecoder::getAudioBuffer(ch) 提供各声道 AudioTimestampRingQueue；
     * outData(port>0) 每次返回绑定对应 buffer 的 AudioData 共享句柄。
     *
     * ## OSC / 属性绑定
     *
     * ExternalBinding 将 fileName / loop / volume / play 暴露给 GlobalEventBus 与属性树。
     */
    class VideoDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString fileName READ getFileName WRITE setFileName NOTIFY fileNameChanged)
        Q_PROPERTY(bool loop READ getLoop WRITE setLoop NOTIFY loopChanged)
        Q_PROPERTY(double volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
        Q_PROPERTY(bool play READ getPlay WRITE setPlay NOTIFY playChanged)

    public:
        /**
         * @brief 构造节点：UI 绑定、ring buffer 初始化、GL 预热、信号连接
         *
         * - ensureImageDataBuffer：创建 m_outImageData + m_outputBuffer（默认 8 槽）
         * - setVideoImageBuffer：注入 VideoDecoder，供解码后 push 纹理帧
         * - ImageGpuUpload::warmup()：在 GUI 线程预创建 QOffscreenSurface，避免解码首帧报错
         * - playbackProgress / playbackFinished 使用 QueuedConnection 更新 UI
         */
        VideoDecoderDataModel(){
            InPortCount = 4;
            OutPortCount = 3;  ///< 初始 1 视频 + 2 音频；loadVideoFile 后按实际声道数调整
            CaptionVisible = true;
            Caption = "Video Decoder";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;

            // ── 属性 ↔ UI 控件 OSC 绑定 ──
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "fileName";
                b.control=widget->fileSelectComboBox;
                AbstractDelegateModel::registerExternalBinding("/fileName", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "loop";
                b.control=widget->loopCheckBox;
                AbstractDelegateModel::registerExternalBinding("/loop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "volume";
                b.control=widget->volumeSlider;
                AbstractDelegateModel::registerExternalBinding("/volume", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "play";
                b.control=widget->playButton;
                AbstractDelegateModel::registerExternalBinding("/play", this, b);
            }

            // ── UI 事件 → 模型属性 ──
            connect(widget->fileSelectComboBox, &SelectorComboBox::textChanged, this, &VideoDecoderDataModel::setFileName);
            connect(widget->playButton, &QPushButton::clicked, this, &VideoDecoderDataModel::setPlay);
            connect(widget->volumeSlider, &FloatDragValueWidget::valueChanged, this, &VideoDecoderDataModel::setVolume);
            connect(widget->loopCheckBox, &QCheckBox::toggled, this, &VideoDecoderDataModel::setLoop);

            // ── 视频输出：共享 Image ring buffer ──
            ensureImageDataBuffer(m_outImageData, m_outputBuffer);
            player->setVideoImageBuffer(m_outputBuffer);
            ImageGpuUpload::instance().warmup();

            // ── 解码引擎 → UI 反馈（跨线程，Queued）──
            connect(player, &VideoDecoder::playbackProgress, this, &VideoDecoderDataModel::onPlaybackProgress, Qt::QueuedConnection);
            connect(player, &VideoDecoder::playbackFinished, this, [this]() {
                setPlay(false);
            }, Qt::QueuedConnection);

            widget->volumeSlider->setValue(-10.0);
            m_volume = -10.0;
        }

        /** @brief 停止播放并等待 VideoDecoder 线程退出 */
        ~VideoDecoderDataModel(){
            if (player->getPlaying()){
                player->stopPlay();
            }
        }
        
        QString getFileName() const { return m_fileName; }

        /** @brief 设置媒体库相对路径并触发 loadVideoFile */
        void setFileName(const QString& fileName) {
            if (m_fileName == fileName) return;
            m_fileName = fileName;
            {
                QSignalBlocker blocker(widget->fileSelectComboBox);
                widget->fileSelectComboBox->setText(m_fileName);
            }
            loadVideoFile(m_fileName);
            
            emit fileNameChanged(m_fileName);
        }

        bool getLoop() const { return m_loop; }
        void setLoop(bool loop) {
            if (m_loop == loop) return;
            m_loop = loop;
            
            QSignalBlocker blocker(widget->loopCheckBox);
            widget->loopCheckBox->setChecked(m_loop);
            
            player->setLooping(m_loop);
            
            emit loopChanged(m_loop);
        }

        double getVolume() const { return m_volume; }
        void setVolume(double volume) {
            if (qFuzzyCompare(m_volume, volume)) return;
            m_volume = volume;
            
            QSignalBlocker blocker(widget->volumeSlider);
            widget->volumeSlider->setValue(m_volume);
            
            player->setVolume(m_volume);
            
            emit volumeChanged(m_volume);
        }
        
        /** @brief 订阅 GlobalEventBus，支持 OSC 远程控制 file / loop / volume / play */
        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/fileName"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/volume"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

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
         * @brief 返回各输出端口的共享数据句柄
         *
         * - port 0：稳定的 m_outImageData（内含 m_outputBuffer 共享指针）
         * - port ≥1：AudioData，绑定 player->getAudioBuffer(port - 1)
         *
         * 连接建立时调用；下游持有同一 shared_ptr，实时读 ring buffer 最新帧。
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            if (port == 0) {
                return m_outImageData;
            }
            
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(player->getAudioBuffer(port - 1));

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
                    return "PLAY/STOP";
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

        /** @brief 外部 Variable 端口驱动播放控制 */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto d = std::dynamic_pointer_cast<VariableData>(data);
            if (d == nullptr) return;

            switch (portIndex) {
            case 0: 
                setPlay(d->asBool());
                break;
            case 1: 
                if (d->asBool()) setPlay(false);
                break;
            case 2:
                setLoop(d->asBool());
                break;
            case 3:
                setVolume(d->asNumber());
                break;
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["filePath"] = m_fileName;
            modelJson["isLoop"] = m_loop;
            modelJson["volume"] = m_volume;
            modelJson["play"] = isPlaying;
            modelJson["autoPlay"] = isPlaying;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonObject modelJson = p;

            bool shouldPlay = false;
            if (modelJson.contains(QStringLiteral("play"))) {
                shouldPlay = modelJson[QStringLiteral("play")].toBool();
            } else if (modelJson.contains(QStringLiteral("autoPlay"))) {
                shouldPlay = modelJson[QStringLiteral("autoPlay")].toBool();
            }
            autoPlay = shouldPlay;

            if (modelJson.contains(QStringLiteral("isLoop"))) {
                setLoop(modelJson[QStringLiteral("isLoop")].toBool());
            }

            if (modelJson.contains(QStringLiteral("volume"))) {
                setVolume(modelJson[QStringLiteral("volume")].toDouble());
            }

            const QString savedFile = modelJson.value(QStringLiteral("filePath")).toString();
            if (!savedFile.isEmpty() && savedFile != m_fileName) {
                isPlaying = false;
                setFileName(savedFile);
            } else if (!savedFile.isEmpty() && !isReady) {
                isPlaying = false;
                m_fileName = savedFile;
                {
                    QSignalBlocker blocker(widget->fileSelectComboBox);
                    widget->fileSelectComboBox->setText(m_fileName);
                }
                loadVideoFile(m_fileName);
            }

            const auto applyPlayState = [this, shouldPlay]() {
                if (shouldPlay && isReady) {
                    setPlay(true);
                } else {
                    setPlay(false);
                }
            };

            if (!savedFile.isEmpty()) {
                const QString absPath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QStringLiteral("/") + savedFile;
                if (QFile::exists(absPath)) {
                    QTimer::singleShot(100, this, applyPlayState);
                    return;
                }
            }

            applyPlayState();
        }

    public slots:

        /** @brief 处理 OSC Command：/file /loop /volume /play */
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "file") setFileName(ev.payload.toString());
                else if (localPath == "loop") setLoop(ev.payload.toBool());
                else if (localPath == "volume") setVolume(ev.payload.toDouble());
                else if (localPath == "play") setPlay(ev.payload.toBool());

            }
        }

        /**
         * @brief 加载媒体库文件并初始化解码器
         *
         * 1. 拼接绝对路径 MEDIA_LIBRARY_STORAGE_DIR + fileName
         * 2. 清空 m_outputBuffer（换源时不保留旧帧）
         * 3. initializeFFmpeg → 动态增删音频输出端口（1 视频 + N 声道）
         * 4. 若换源前正在播放则 resumePlay
         */
        void loadVideoFile(QString fileName)
        {
            if(fileName != "")
            {
                filePath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + fileName;
                const bool resumePlay = isPlaying || player->getPlaying();

                ensureImageDataBuffer(m_outImageData, m_outputBuffer);
                player->setVideoImageBuffer(m_outputBuffer);
                if (m_outputBuffer) {
                    m_outputBuffer->clear();
                }

                auto res = player->initializeFFmpeg(filePath);

                if(!res){
                    isReady = false;
                    isPlaying = false;
                    emit playChanged(false);
                    return;
                }
                delete res;

                // 按实际声道数调整 Out 端口：port0=视频，port1..N=各声道
                unsigned int channels = player->getChannels();
                unsigned int newOutPortCount = 1 + channels;

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

                if (resumePlay) {
                    setPlay(true);
                }
            }
        }

        /**
         * @brief 开始/停止播放
         *
         * 开始前会先 stopPlay 再 startPlay，确保从文件头重新解码。
         * 未 loadVideoFile 成功（!isReady）时忽略播放请求。
         */
        void setPlay(bool toPlay) {

            if (!isReady) {
                isPlaying = false;
                emit playChanged(false);
                return;
            }
            if (!toPlay) {
                player->stopPlay();
                isPlaying=false;
                emit playChanged(false);
            }else {
                player->stopPlay();
                player->startPlay();
                isPlaying=true;
                emit playChanged(true);
            }
            if (widget->playButton->isChecked()!=isPlaying) {
                widget->playButton->blockSignals(true);
                widget->playButton->setChecked(isPlaying);
                widget->playButton->blockSignals(false);
            }

        }
        bool getPlay() {
            return isPlaying;
        }

        /** @brief 更新进度条与时间标签（由 playbackProgress 信号驱动） */
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
        void playChanged(bool playing);

    private:
        QString formatTime(double seconds) {
            int m = static_cast<int>(seconds) / 60;
            int s = static_cast<int>(seconds) % 60;
            return QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        }

        /** 输出端口 0：ImageData 共享句柄，指向 m_outputBuffer */
        std::shared_ptr<NodeDataTypes::ImageData> m_outImageData;
        /** 视频帧 GPU 纹理 ring buffer，由 VideoDecoder 在 GUI 线程 push */
        std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue> m_outputBuffer;

        VideoDecoderInterface *widget=new VideoDecoderInterface();  ///< 文件选择 / 播放 / 进度 UI
        VideoDecoder *player=new VideoDecoder();                    ///< 解码引擎

        QString filePath="";      ///< 媒体库绝对路径（MEDIA_LIBRARY_STORAGE_DIR + fileName）
        QString m_fileName="";    ///< 媒体库相对文件名（save/load 用）
        bool m_loop=false;
        double m_volume = -10.0;
        bool autoPlay=false;      ///< load 后是否自动播放
        bool isReady= false;      ///< initializeFFmpeg 是否成功
        bool isPlaying= false;    ///< 与 UI playButton 同步的播放状态

    };
}
