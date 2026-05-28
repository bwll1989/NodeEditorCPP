#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QTimer>
#include <QDebug>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/qglobal.h>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioAnalysisWorker.hpp"
#include "Gist.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes {
    /**

     */
    // 修改后的AudioAnalysisDataModel类（关键部分）
    class AudioAnalysisDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    
    public:
        /**
         * @brief 音频矩阵路由节点构造函数
         */
        AudioAnalysisDataModel()
            : _worker(new AudioAnalysisWorker())
            , _workerThread(new QThread(this))
            ,_outputData(nullptr)
        {
            InPortCount = 2;
            OutPortCount = 1;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "enabled";
                AbstractDelegateModel::registerExternalBinding("/enable", this, b);
            }
            connect(_workerThread, &QThread::finished, _worker, &AudioAnalysisWorker::stopProcessing);
            connect(_worker, &AudioAnalysisWorker::processingStatusChanged, this, &AudioAnalysisDataModel::onProcessingStatusChanged);
            connect(_worker, &AudioAnalysisWorker::analysisValueChanged, this, &AudioAnalysisDataModel::onGetResult);
            // 启动工作线程
            _workerThread->start();
        }
    
        /**
         * @brief 析构函数
         */
        ~AudioAnalysisDataModel()
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
                    if( portIndex==0)
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
                    if( portIndex==0)
                        return "AUDIO";
                    return  "ENABLE";
                case PortType::Out:
                    return "RES";
                default:
                    return "";
            }
        }

        /**
         * 函数级注释：获取当前音频分析节点的启用状态属性
         */
        bool enabled() const
        {
            return m_enabled;
        }

        /**
         * 函数级注释：设置音频分析节点启用状态属性，触发状态反馈
         */
        void setEnabled(bool enabled)
        {
            if (enabled == m_enabled) {
                return;
            }
            m_enabled = enabled;
            Q_EMIT enabledChanged(enabled);
        }
        
         /**
         * @brief 获取输出数据
         * @param port 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {

            return _outputData;
        }
    
        /**
         * @brief 设置输入数据
         * @param nodeData 输入节点数据
         * @param port 端口索引
         */
        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (port==0) {
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
            }else {
                auto Data = std::dynamic_pointer_cast<VariableData>(nodeData);
                if (Data) {
                    setEnabled(Data->value().toBool());
                }

            }


        }
    
    public slots:

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1 ;
            modelJson1["enable"] = enabled();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
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
            if (!v.isUndefined()&&v.isObject()) {
                qDebug() << "loading audio analysis data"<<v["enable"].toBool(false);
                setEnabled(v["enable"].toBool(false));
            }

        }

    signals:
        /**
         * 函数级注释：当音频分析节点启用状态发生变化时发出的通知信号
         */
        void enabledChanged(bool enabled);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/enable"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        /**
         * 函数级注释：处理来自全局事件总线的外部命令，更新启用状态属性
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/enable")) {
                return;
            }
            setEnabled(ev.payload.toBool());
        }
    public slots:
        /**
         * @brief 处理状态变化槽函数
         * @param isProcessing 是否正在处理
         */
        void onProcessingStatusChanged(bool isProcessing)
        {
            if (isProcessing) {
                // widget->setStatus(false, "Processing");
            } else {
                // widget->setStatus(true, "Idle");
            }
        }
        void onGetResult(QVariantMap const& result) {
            if (enabled()) {
                _outputData=std::make_shared<VariableData>(result);
                dataUpdated(0);
            }

        }


    private:
        AudioAnalysisWorker* _worker;                                      ///< 工作线程对象
        QThread* _workerThread;                                         ///< 工作线程
        // 移除这行：std::vector<std::shared_ptr<AudioTimestampRingQueue>> _audioBuffer;
        std::shared_ptr<VariableData> _outputData;
        bool m_enabled = false;
    };
}
