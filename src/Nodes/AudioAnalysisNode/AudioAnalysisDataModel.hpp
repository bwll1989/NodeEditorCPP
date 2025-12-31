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
#include "AudioAnalysisInterface.h"
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioAnalysisWorker.hpp"
#include "Gist.h"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {
    /**

     */
    // 修改后的AudioAnalysisDataModel类（关键部分）
    class AudioAnalysisDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    
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
            widget = new AudioAnalysisInterface();
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            AbstractDelegateModel::registerOSCControl("/enable",widget->mResetButton);
            // 设置工作线程
            _worker->moveToThread(_workerThread);
            // 连接信号槽
            connect(_workerThread, &QThread::started, this, [this]() {
                // 确保在缓冲区初始化后启动处理
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
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
                    widget->mResetButton->setChecked(Data->value().toBool());
                }

            }


        }
    
    public slots:
        /**
        */
        QWidget *embeddedWidget() override { return widget; }

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson1 ;
            modelJson1["enable"] = widget->mResetButton->isChecked();
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
                // widget->setStatus(false, "Processing");
            } else {
                // widget->setStatus(true, "Idle");
            }
        }
        void onGetResult(QVariantMap const& result) {
            if (widget->mResetButton->isChecked()) {
                _outputData=std::make_shared<VariableData>(result);
                dataUpdated(0);
            }

        }


    private:
        AudioAnalysisWorker* _worker;                                      ///< 工作线程对象
        QThread* _workerThread;                                         ///< 工作线程
        // 移除这行：std::vector<std::shared_ptr<AudioTimestampRingQueue>> _audioBuffer;
        AudioAnalysisInterface* widget;
        std::shared_ptr<VariableData> _outputData;
    };
}