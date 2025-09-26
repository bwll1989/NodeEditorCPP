#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QTimer>
#include <QDebug>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "DataTypes/NodeDataList.hpp"
#include "DataTypes/AudioTimestampRingQueue.h"
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
    class LTCDecoderDataModel : public NodeDelegateModel
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
            OutPortCount = 5;
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

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
            Q_UNUSED(portIndex);
            switch (portType) {
                case PortType::In:
                    return AudioData().type();
                case PortType::Out:
                    return VariableData().type();
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
                _audioBuffer = audioData->getSharedAudioBuffer();
                
                if (_audioBuffer) {
                    // 在工作线程中设置音频缓冲区
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
            // 应用偏移
            _timeCodeFrame = timecode_frame_add(frame, _label->timeCodeOffsetSpinBox->value());
            
            // 更新界面显示（主线程安全）
            _label->setTimeStamp(_timeCodeFrame);
            _label->setStatus(false, "Decoding");
            
            // 通知输出端口数据更新
            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            emit dataUpdated(3);
            emit dataUpdated(4);
        }
        
        /**
         * @brief 处理状态变化槽函数
         * @param isProcessing 是否正在处理
         */
        void onProcessingStatusChanged(bool isProcessing)
        {
            if (isProcessing) {
                _label->setStatus(false, "Processing");
            } else {
                _label->setStatus(true, "Idle");
            }
        }
    
    private:
        TimeCodeInterface* _label;                                      ///< 时间码显示界面
        TimeCodeFrame _timeCodeFrame;                                   ///< 当前时间码帧
        LTCDecoderWorker* _worker;                                      ///< 工作线程对象
        QThread* _workerThread;                                         ///< 工作线程
        std::shared_ptr<AudioTimestampRingQueue> _audioBuffer;          ///< 音频缓冲区
    };
}