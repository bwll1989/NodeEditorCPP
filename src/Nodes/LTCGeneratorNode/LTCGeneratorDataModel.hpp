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
#include "LTCGeneratorInterface.h"
#include <QComboBox>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "LTCGeneratorWorker.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {
    /**
     * 函数级注释：LTC生成节点模型
     * - 职责：使用 LTCGeneratorWorker 生成 48kHz float32 音频波形，并通过输出端口提供共享环形缓冲区
     * - 端口：仅输出端口（AudioData），无输入
     * - UI：显示当前生成的时间码与状态
     */
    class LTCGeneratorDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    
    public:
        /**
         * 函数级注释：构造函数
         * - 初始化工作线程并开始生成
         * - 设置端口数量与节点标题
         */
        LTCGeneratorDataModel()
            : _label(new TimeCodeInterface())
            , _worker(new LTCGeneratorWorker())
            , _workerThread(new QThread(this))
        {
            qRegisterMetaType<TimeCodeType>("TimeCodeType");
            InPortCount = 4;
            OutPortCount = 1;
            CaptionVisible = true;
            WidgetEmbeddable=false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            // 设置工作线程
            _worker->moveToThread(_workerThread);
            
            // 连接信号槽
            // 注意：节点实例化时不自动运行，仅启动线程以便后续通过QueuedConnection安全调用worker
            connect(_workerThread, &QThread::finished, _worker, &LTCGeneratorWorker::stopProcessing);
            connect(_worker, &LTCGeneratorWorker::timeCodeFrameGenerated, this, &LTCGeneratorDataModel::onReceivedTimecodeFrame);
            connect(_worker, &LTCGeneratorWorker::processingStatusChanged, this, &LTCGeneratorDataModel::onProcessingStatusChanged);

            connect(_label, &TimeCodeInterface::startRequested, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            connect(_label, &TimeCodeInterface::stopRequested, this, [this]() {
                QMetaObject::invokeMethod(_worker, "stopProcessing", Qt::QueuedConnection);
            });
            connect(_label, &TimeCodeInterface::resetRequested, this, [this]() {
                QMetaObject::invokeMethod(_worker, "resetTimecode", Qt::QueuedConnection);
            });
            connect(_label, &TimeCodeInterface::timeCodeTypeChanged, this, [this](TimeCodeType type) {
                QMetaObject::invokeMethod(_worker, "setTimeCodeType", Qt::QueuedConnection, Q_ARG(TimeCodeType, type));
            });
            connect(_label, &TimeCodeInterface::volumeChanged, this, [this](float volume) {
                QMetaObject::invokeMethod(_worker, "setVolume", Qt::QueuedConnection, Q_ARG(float, volume));
            });
            
            // 启动工作线程
            _workerThread->start();
        }
    
        /**
         * 函数级注释：析构函数
         * - 安全停止工作线程并释放资源
         */
        ~LTCGeneratorDataModel()
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
         * 函数级注释：获取端口数据类型
         */
        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portIndex);
            switch (portType) {
                case PortType::In:
                    return VariableData().type();
                case PortType::Out:
                    return AudioData().type();
                default:
                    return AudioData().type();
            }
        }
        
        /**
         * 函数级注释：获取端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    if (portIndex == 0) return "START/STOP"; // TOGGLE
                    if (portIndex == 1) return "START";      // Trigger
                    if (portIndex == 2) return "STOP";       // Trigger
                    if (portIndex == 3) return "RESET";      // Trigger
                    return "";
                case PortType::Out:
                    if (portIndex == 0) return "AUDIO";
                    return "";
                default:
                    return "";
            }
        }
        
        /**
         * 函数级注释：获取输出数据
         * - 返回包含共享环形缓冲区的 AudioData，用于与下游节点建立连接
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port != 0) {
                return std::make_shared<AudioData>();
            }
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(_worker->getOutputBuffer());
            return audioData;
        }

        /**
         * 函数级注释：处理输入端口触发
         * - 端口0：TOGGLE（true启动，false停止）
         * - 端口1：START Trigger（true启动，忽略false）
         * - 端口2：STOP Trigger（true停止，忽略false）
         * - 端口3：RESET Trigger（true归零，忽略false）
         */
        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (!nodeData) {
                return;
            }
            auto varData = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!varData) {
                return;
            }
            const bool v = varData->value().toBool();

            switch (port) {
                case 0: // TOGGLE
                    if (v) {
                        QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                    } else {
                        QMetaObject::invokeMethod(_worker, "stopProcessing", Qt::QueuedConnection);
                    }
                    break;
                case 1: // START (Trigger)
                    if (v) QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                    break;
                case 2: // STOP (Trigger)
                    if (v) QMetaObject::invokeMethod(_worker, "stopProcessing", Qt::QueuedConnection);
                    break;
                case 3: // RESET (Trigger)
                    if (v) QMetaObject::invokeMethod(_worker, "resetTimecode", Qt::QueuedConnection);
                    break;
                default:
                    break;
            }
        }

        QWidget *embeddedWidget() override { return _label; }

        /**
         * 函数级注释：保存节点配置
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["offset"] = _label->timeCodeOffsetSpinBox->value();
            modelJson["timecodeType"] = _label->timeCodeTypeComboBox->currentText();
            modelJson["volume"] = _label->volumeSlider->value();
            return modelJson;
        }
        
        /**
         * 函数级注释：加载节点配置
         */
        void load(QJsonObject const& jsonObj) override
        {
            if (jsonObj.contains("offset")) {
                _label->timeCodeOffsetSpinBox->setValue(jsonObj["offset"].toInt());
            }
            if (jsonObj.contains("timecodeType")) {
                const QString label = jsonObj["timecodeType"].toString();
                const int idx = _label->timeCodeTypeComboBox->findText(label);
                if (idx >= 0) {
                    _label->timeCodeTypeComboBox->setCurrentIndex(idx);
                } else {
                    _label->timeCodeTypeComboBox->setCurrentText(timecode_type_to_label(TimeCodeType::PAL));
                }
                QMetaObject::invokeMethod(_worker, "setTimeCodeType", Qt::QueuedConnection, Q_ARG(TimeCodeType, timecode_type_from_label(label, TimeCodeType::PAL)));
            }
            if (jsonObj.contains("volume")) {
                double vol = jsonObj["volume"].toDouble();
                _label->volumeSlider->setValue(vol);
                QMetaObject::invokeMethod(_worker, "setVolume", Qt::QueuedConnection, Q_ARG(float, (float)vol));
            }
        }
        
    public slots:
        /**
         * 函数级注释：接收到生成的时间码帧（在主线程中执行）
         */
        void onReceivedTimecodeFrame(TimeCodeFrame frame)
        {
            // 应用偏移
            _timeCodeFrame = timecode_frame_add(frame, _label->timeCodeOffsetSpinBox->value());
            
            // 更新界面显示（主线程安全）
            _label->setTimeStamp(_timeCodeFrame);
            _label->setStatus(false, "Generating");
            
            // 通知输出端口数据更新
            emit dataUpdated(0);
        }
        
        /**
         * 函数级注释：处理状态变化槽函数
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
        LTCGeneratorWorker* _worker;                                     ///< 工作线程对象
        QThread* _workerThread;                                         ///< 工作线程
    };
}
