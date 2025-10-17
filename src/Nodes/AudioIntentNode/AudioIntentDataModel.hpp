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
#include "AudioIntentInterface.h"
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioIntentWorker.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {
    /**
     * @brief 音频意图识别数据模型类
     * 集成Porcupine唤醒词检测和Rhino意图识别功能
     */
    class AudioIntentDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    
    public:
        /**
         * @brief 音频意图识别节点构造函数
         */
        AudioIntentDataModel()
            : _worker(new AudioIntentWorker())
            , _workerThread(new QThread(this))
            , _outputData(nullptr)
            , _wakewordData(nullptr)
            , _intentData(nullptr)
            , _stateData(nullptr)
        {
            InPortCount = 2;
            OutPortCount = 4;  // 增加输出端口：结果、唤醒词、意图、状态
            widget = new AudioIntentInterface();
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            NodeDelegateModel::registerOSCControl("/enable", widget->mResetButton);
            
            // 配置参数
            QString accessKey = "sYO/yIvE6wETbCc4Qp+bS00Q5Ps2Hn8Z3VFV3x1Cz0DZWwbxfq/lfQ==";
            
            // Porcupine配置
            QString porcupineModelPath = "./plugins/Rhino/porcupine_params_zh.pv";
            QStringList keywordPaths = {
                "./plugins/Rhino/二狗子_zh_windows_v3_0_0.ppn",
                "./plugins/Rhino/你好_windows.ppn"
            };
            QList<float> keywordSensitivities = {0.5f,0.5f};
            
            // Rhino配置
            QString rhinoModelPath = "./plugins/Rhino/rhino_params_zh.pv";
            QString contextPath = "./plugins/Rhino/test.rhn";
            float rhinoSensitivity = 0.5f;

            _worker->moveToThread(_workerThread);
            
            // 连接信号槽
            connect(_workerThread, &QThread::started, this, [this, accessKey, porcupineModelPath, keywordPaths, keywordSensitivities, rhinoModelPath, contextPath, rhinoSensitivity]() {
                // 设置Porcupine配置
                QMetaObject::invokeMethod(_worker, "setPorcupineConfig",
                                        Qt::QueuedConnection,
                                        Q_ARG(QString, accessKey),
                                        Q_ARG(QString, porcupineModelPath),
                                        Q_ARG(QStringList, keywordPaths),
                                        Q_ARG(QList<float>, keywordSensitivities));
                
                // 启动处理
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            
            connect(_workerThread, &QThread::finished, _worker, &AudioIntentWorker::stopProcessing);
            
            // 连接Worker的信号
            connect(_worker, &AudioIntentWorker::processingStatusChanged,
                    this, &AudioIntentDataModel::onProcessingStatusChanged);
            connect(_worker, &AudioIntentWorker::analysisValueChanged,
                    this, &AudioIntentDataModel::onAnalysisValueChanged);
            connect(_worker, &AudioIntentWorker::wakewordDetected,
                    this, &AudioIntentDataModel::onWakewordDetected);
            connect(_worker, &AudioIntentWorker::stateChanged,
                    this, &AudioIntentDataModel::onStateChanged);

            _workerThread->start();
        }
    
        /**
         * @brief 析构函数
         */
        ~AudioIntentDataModel()
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
                    if (portIndex == 0)
                        return AudioData().type();
                    return VariableData().type();
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
                    if (portIndex == 0)
                        return "AUDIO";
                    return "ENABLE";
                case PortType::Out:
                    switch (portIndex) {
                        case 0: return "ANALYSIS";  // 分析结果（RMS等）
                        case 1: return "WAKEWORD";  // 唤醒词检测结果
                        case 2: return "INTENT";    // 意图识别结果
                        case 3: return "STATE";     // 当前状态
                        default: return "";
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
            switch (port) {
                case 0: return _outputData;     // 分析结果
                case 1: return _wakewordData;   // 唤醒词结果
                case 2: return _intentData;     // 意图结果
                case 3: return _stateData;      // 状态信息
                default: return nullptr;
            }
        }
    
        /**
         * @brief 设置输入数据
         * @param nodeData 输入节点数据
         * @param port 端口索引
         */
        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (port == 0) {
                auto audioData = std::dynamic_pointer_cast<AudioData>(nodeData);
                std::shared_ptr<AudioTimestampRingQueue> audioBuffer = nullptr;

                if (audioData && audioData->isConnectedToSharedBuffer()) {
                    audioBuffer = audioData->getSharedAudioBuffer();
                }

                // 直接更新Worker中的指定端口缓冲区
                QMetaObject::invokeMethod(_worker, "setInputBuffer",
                                        Qt::QueuedConnection,
                                        Q_ARG(int, port),
                                        Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, audioBuffer));
            } else {
                auto data = std::dynamic_pointer_cast<VariableData>(nodeData);
                if (data) {
                    widget->mResetButton->setChecked(data->value().toBool());
                }
            }
        }
    
    public slots:
        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override { return widget; }

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["enable"] = widget->mResetButton->isChecked();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         * @param jsonObj JSON对象
         */
        void load(QJsonObject const& jsonObj) override
        {
            // 首先调用基类的load方法
            NodeDelegateModel::load(jsonObj);
            QJsonValue v = jsonObj["values"];
            if (!v.isUndefined() && v.isObject()) {
                qDebug() << "Loading audio analysis data" << v["enable"].toBool(false);
                widget->mResetButton->setChecked(v["enable"].toBool(false));
            }
        }
        
    public slots:
        /**
         * @brief 处理状态变化槽函数
         * @param isProcessing 是否正在处理
         */
        void onProcessingStatusChanged(bool isProcessing)
        {
            if (isProcessing) {
                qDebug() << "Audio processing started";
            } else {
                qDebug() << "Audio processing stopped";
            }
        }
        
        /**
         * @brief 处理分析值变化槽函数
         * @param result 分析结果
         */
        void onAnalysisValueChanged(QVariantMap const& result)
        {
            if (widget->mResetButton->isChecked()) {
                _outputData = std::make_shared<VariableData>(result);
                dataUpdated(0);
            }
        }
        
        /**
         * @brief 处理唤醒词检测槽函数
         * @param keyword 检测到的唤醒词
         */
        void onWakewordDetected(const QString& keyword)
        {
            qDebug() << "Wakeword detected in DataModel:" << keyword;
            
            QVariantMap wakewordResult;
            wakewordResult["keyword"] = keyword;
            wakewordResult["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            wakewordResult["detected"] = true;
            
            _wakewordData = std::make_shared<VariableData>(wakewordResult);
            dataUpdated(1);
        }
        
        /**
         * @brief 处理意图识别槽函数
         * @param intent 识别到的意图
         * @param confidence 置信度
         * @param slots 槽位信息
         */
        void onIntentRecognized(const QString& intent, float confidence, const QVariantMap& slotNames)
        {
            qDebug() << "Intent recognized in DataModel:" << intent << "Confidence:" << confidence;
            
            QVariantMap intentResult;
            intentResult["intent"] = intent;
            intentResult["confidence"] = confidence;
            intentResult["slots"] = slotNames;
            intentResult["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            intentResult["understood"] = true;
            
            _intentData = std::make_shared<VariableData>(intentResult);
            dataUpdated(2);
        }
        
        /**
         * @brief 处理状态变化槽函数
         * @param state 当前状态
         */
        void onStateChanged(AudioIntentWorker::ProcessingState state)
        {
            QString stateStr;
            switch (state) {
                case AudioIntentWorker::STATE_IDLE:
                    stateStr = "IDLE";
                    break;
                case AudioIntentWorker::STATE_WAKE_DETECTION:
                    stateStr = "WAKE_DETECTION";
                    break;

            }
            
            qDebug() << "State changed in DataModel:" << stateStr;
            
            QVariantMap stateResult;
            stateResult["state"] = stateStr;
            stateResult["state_code"] = static_cast<int>(state);
            stateResult["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            
            _stateData = std::make_shared<VariableData>(stateResult);
            dataUpdated(3);
        }
        
    private:
        AudioIntentWorker* _worker;                           ///< 工作线程对象
        QThread* _workerThread;                                 ///< 工作线程
        AudioIntentInterface* widget;                        ///< 界面控件
        
        // 输出数据
        std::shared_ptr<VariableData> _outputData;              ///< 分析结果数据
        std::shared_ptr<VariableData> _wakewordData;            ///< 唤醒词检测数据
        std::shared_ptr<VariableData> _intentData;              ///< 意图识别数据
        std::shared_ptr<VariableData> _stateData;               ///< 状态数据
    };
}