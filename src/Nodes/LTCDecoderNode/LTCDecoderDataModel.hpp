#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QTimer>
#include <QDebug>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/qglobal.h>
#include "LTCDecoderInterface.h"
#include "../../Common/Devices/LtcDecoder/ltcdecoder.h"
#include <QComboBox>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "LTCDecoderWorker.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {
    /**
     * @brief LTC解码节点模型
     * 接收AudioData输入，解码为时间码并输出小时、分钟、秒、帧
     */
    // 修改后的LTCDecoderDataModel类（关键部分）
    class LTCDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    
    public:
        /**
         * @brief LTC解码节点构造函数
         */
        LTCDecoderDataModel()
            : _label(new TimeCodeInterface())
            , _worker(new LTCDecoderWorker())
            , _workerThread(new QThread(this))
            , _audioBuffer(nullptr)
        {
            InPortCount = 1;
            OutPortCount = 6;
            CaptionVisible = true;
            Resizable = false;
            PortEditable = false;
            WidgetEmbeddable=false;
            Caption = PLUGIN_NAME;

            // 同步源状态：对外以 SyncData（内部秒）发布，图内同步统一走全局时钟
            m_syncState = std::make_shared<SyncSharedState>(QStringLiteral("LTCDecoder"));

            // TC 失联看门狗：>500ms 无新帧则置无效（跟随端据此联动停止）
            m_ltcWatchdog = new QTimer(this);
            m_ltcWatchdog->setInterval(200);
            connect(m_ltcWatchdog, &QTimer::timeout, this, [this]() {
                if (m_lastFrameTickMs > 0
                    && QDateTime::currentMSecsSinceEpoch() - m_lastFrameTickMs > 500) {
                    m_syncState->setActive(false);
                    // 边沿触发：LTC 信号丢失时回落到 Listening（音频仍在喍但解不到帧）
                    if (m_decoding) {
                        m_decoding = false;
                        _label->setStatus(false, "Listening");
                    }
                }
            });
            m_ltcWatchdog->start();

            // 设置工作线程
            _worker->moveToThread(_workerThread);
            
            // 连接信号槽
            connect(_workerThread, &QThread::started, _worker, &LTCDecoderWorker::startProcessing);
            connect(_workerThread, &QThread::finished, _worker, &LTCDecoderWorker::stopProcessing);
            connect(_worker, &LTCDecoderWorker::timeCodeFrameDecoded, this, &LTCDecoderDataModel::onReceivedTimecodeFrame);
            connect(_worker, &LTCDecoderWorker::processingStatusChanged, this, &LTCDecoderDataModel::onProcessingStatusChanged);
            
            // 启动工作线程
            _workerThread->start();
        }
    
        /**
         * @brief 析构函数
         */
        ~LTCDecoderDataModel()
        {
            // 安全停止工作线程
            if (_workerThread && _workerThread->isRunning()) {
                _workerThread->quit();
                _workerThread->wait(3000); // 等待最多3秒
            }
            
            if (_worker) {
                _worker->deleteLater();
            }
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    return AudioData().type();
                case PortType::Out:
                    return (portIndex == 5) ? SyncData().type() : VariableData().type();
                default:
                    return VariableData().type();
            }
        }
        
        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    return "AUDIO";
                case PortType::Out:
                    switch (portIndex)
                    {
                        case 0:
                            return "FRAMES";
                        case 1:
                            return "SECONDS";
                        case 2:
                            return "MINUTES";
                        case 3:
                            return "HOURS";
                        case 4:
                            return "TOTAL";
                        case 5:
                            return "SYNC";
                        default:
                            break;
                        }
                default:
                    return "";
            }
        }
        
        /**
         * @brief 获取输出数据
         * @param port 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port)
            {
            case 5:
                // SYNC 时钟源输出口：TC 已映射为内部秒（含用户偏移）
                return std::make_shared<SyncData>(m_syncState);
            case 0:
                return std::make_shared<VariableData>(_timeCodeFrame.frames);
            case 1:
                return std::make_shared<VariableData>(_timeCodeFrame.seconds);
            case 2:
                return std::make_shared<VariableData>(_timeCodeFrame.minutes);
            case 3:
                return std::make_shared<VariableData>(_timeCodeFrame.hours);
            case 4:
                return std::make_shared<VariableData>(timecode_frame_to_frames(_timeCodeFrame, _timeCodeFrame.type));
            default:
                return std::make_shared<VariableData>();
            }
        }

        /**
         * @brief 设置输入数据
         */
        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            Q_UNUSED(port);
            
            auto audioData = std::dynamic_pointer_cast<AudioData>(nodeData);
            if (audioData && audioData->isConnectedToSharedBuffer()) {
                auto buffer = audioData->getSharedAudioBuffer();

                if (buffer && buffer != _audioBuffer) {
                    // 上游每推一块音频都会触发 setInData：仅在缓冲指针真正变化时
                    // 重建 worker 缓冲并更新状态，避免 "Connected" 反复覆盖 "Decoding"
                    _audioBuffer = buffer;
                    QMetaObject::invokeMethod(_worker, "setAudioBuffer",
                                            Qt::QueuedConnection,
                                            Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, _audioBuffer));

                    _label->setStatus(false, "Connected");
                }
            } else {
                // 断开连接
                if (_audioBuffer) {
                    _audioBuffer.reset();
                    QMetaObject::invokeMethod(_worker, "setAudioBuffer", 
                                            Qt::QueuedConnection,
                                            Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, nullptr));
                    _label->setStatus(true, "Disconnected");
                }
            }
            
            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            emit dataUpdated(3);
        }

        QWidget *embeddedWidget() override { return _label; }

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["offset"] = _label->timeCodeOffsetSpinBox->value();
            return modelJson;
        }
        
        /**
         * @brief 加载节点配置
         * @param jsonObj JSON对象
         */
        void load(QJsonObject const& jsonObj) override
        {
            if (jsonObj.contains("offset")) {
                _label->timeCodeOffsetSpinBox->setValue(jsonObj["offset"].toInt());
            }
        }
        
    public slots:
        /**
         * @brief 接收到时间码帧的槽函数（在主线程中执行）
         * @param frame 解码得到的时间码帧
         */
        void onReceivedTimecodeFrame(TimeCodeFrame frame)
        {
            _label->setTimeStamp(frame);
            _timeCodeFrame = timecode_frame_add(frame, _label->timeCodeOffsetSpinBox->value());
            if (!m_decoding) {
                m_decoding = true;
                _label->setStatus(false, "Decoding");
            }

            // ---- 时钟源发布：TC（含用户偏移）→ 内部秒 → 共享同步状态 ----
            {
                const int fps = timecode_frames_per_sec(_timeCodeFrame.type);
                const double sec = _timeCodeFrame.hours * 3600.0
                    + _timeCodeFrame.minutes * 60.0
                    + _timeCodeFrame.seconds
                    + double(_timeCodeFrame.frames) / double(fps > 0 ? fps : 25);
                m_syncState->setActive(true);
                m_syncState->write(sec,
                                   TimestampGenerator::getInstance()->getCurrentFrameCount(),
                                   true);
                m_lastFrameTickMs = QDateTime::currentMSecsSinceEpoch();
            }

            // 通知输出端口数据更新
            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            emit dataUpdated(3);
            emit dataUpdated(4);
            emit dataUpdated(5);
        }
        
        /**
         * @brief 处理状态变化槽函数
         * @param isProcessing 是否正在处理（音频缓冲已就绪）
         * 注意：这只表示"音频在喍"，不代表已解到 LTC。显示用 Listening，
         * 且不覆盖 Decoding（后者由真实解出的 LTC 帧驱动），避免状态来回跳动
         */
        void onProcessingStatusChanged(bool isProcessing)
        {
            if (isProcessing) {
                if (!m_decoding) {
                    _label->setStatus(false, "Listening");
                }
            } else {
                m_decoding = false;
                _label->setStatus(true, "Idle");
            }
        }
    
    private:
        TimeCodeInterface* _label;                                      ///< 时间码显示界面
        TimeCodeFrame _timeCodeFrame;                                   ///< 当前时间码帧
        LTCDecoderWorker* _worker;                                      ///< 工作线程对象
        QThread* _workerThread;                                         ///< 工作线程
        std::shared_ptr<AudioTimestampRingQueue> _audioBuffer;          ///< 音频缓冲区

        // ---- SyncData 时钟源发布 ----
        std::shared_ptr<SyncSharedState> m_syncState;                   ///< 对外发布的同步状态（TC→内部秒）
        QTimer* m_ltcWatchdog = nullptr;                                ///< TC 失联看门狗
        qint64 m_lastFrameTickMs = 0;                                   ///< 最后收到 TC 帧的时刻（ms）
        bool m_decoding = false;                                        ///< 正在解到 LTC 帧（状态栏边沿去重）
    };
}