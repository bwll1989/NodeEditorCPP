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
#include "AudioMatrixInterface.h"
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioMatrixWorker.hpp"
#include "Eigen/Core"
#include "opencv2/flann/matrix.h"
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
    // 修改后的AudioMatrixDataModel类（关键部分）
    class AudioMatrixDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    
    public:
        /**
         * @brief 音频矩阵路由节点构造函数
         */
        AudioMatrixDataModel()
            : _worker(new AudioMatrixWorker())
            , _workerThread(new QThread(this))
        {
            InPortCount = 8;
            OutPortCount = 8;
            widget = new AudioMatrixInterface(InPortCount, OutPortCount);
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            
            // 初始化矩阵
            int rows = widget->mMatrixWidget->getRows();
            int cols = widget->mMatrixWidget->getCols();
            matrix = Eigen::MatrixXd::Zero(rows, cols);
            _worker->initializeBuffers(InPortCount,OutPortCount,matrix);
            // 设置工作线程
            _worker->moveToThread(_workerThread);
            for (int i = 0; i < widget->mMatrixWidget->getRows(); i++) {
                for (int j = 0; j < widget->mMatrixWidget->getCols(); j++) {
                    NodeDelegateModel::registerOSCControl(QString("/%1-%2").arg(i).arg(j), widget->mMatrixWidget->getMatrixElement(i*widget->mMatrixWidget->getCols()+j));
                }
            }




            // 连接信号槽
            connect(_workerThread, &QThread::started, this, [this]() {
                // 确保在缓冲区初始化后启动处理
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                // 更新矩阵数据
                QMetaObject::invokeMethod(_worker, "updateMatrix",
                                       Qt::QueuedConnection,
                                       Q_ARG(Eigen::MatrixXd, matrix));
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioMatrixWorker::stopProcessing);
            connect(widget->mMatrixWidget, &MatrixWidget::valueChanged, this, &AudioMatrixDataModel::setMatrix);
            connect(_worker, &AudioMatrixWorker::processingStatusChanged, this, &AudioMatrixDataModel::onProcessingStatusChanged);
            
            // 启动工作线程
            _workerThread->start();
        }
    
        /**
         * @brief 析构函数
         */
        ~AudioMatrixDataModel()
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
                    return AudioData().type();
                default:
                    return AudioData().type();
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
                    return QString("IN%1").arg(portIndex + 1);
                case PortType::Out:
                    return QString("OUT%1").arg(portIndex + 1);
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
            if (port >= OutPortCount) {
                return std::make_shared<AudioData>();
            }

            // 从Worker获取输出数据
            std::shared_ptr<AudioData> outputData = std::make_shared<AudioData>();
            // 直接访问Worker的输出缓冲区
            std::shared_ptr<AudioTimestampRingQueue> outputBuffer = _worker->getOutputBuffer(port);
            if (outputBuffer) {
                outputData->setSharedAudioBuffer(outputBuffer);
            }
            return outputData;
        }
    
        /**
         * @brief 设置输入数据
         * @param nodeData 输入节点数据
         * @param port 端口索引
         */
        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (port >= InPortCount) {
                return;
            }
            
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

        }
    
    public slots:
        /**
         * @brief 设置矩阵数据
         * @param mat 新的矩阵数据
         */
        void setMatrix(Eigen::MatrixXd mat) {
            matrix = mat;
            // 更新Worker中的矩阵
            QMetaObject::invokeMethod(_worker, "updateMatrix", 
                                    Qt::QueuedConnection,
                                    Q_ARG(Eigen::MatrixXd, matrix));
        }

        QWidget *embeddedWidget() override { return widget; }

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();

            // 保存矩阵数据
            QJsonObject matrixJson;
            auto matrixData=widget->mMatrixWidget->getValuesAsMatrix();
            matrixJson["rows"] = static_cast<int>(matrixData.rows());
            matrixJson["cols"] = static_cast<int>(matrixData.cols());
            
            // 将矩阵数据序列化为数组
            QJsonArray Data;
            for (int i = 0; i < matrixData.rows(); ++i) {
                for (int j = 0; j < matrixData.cols(); ++j) {
                    Data.append(matrixData(i, j));

                }
            }
            matrixJson["data"] = Data;
            modelJson["matrix"] = matrixJson;
            

            
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

            // 加载矩阵数据
            if (jsonObj.contains("matrix")) {
                QJsonObject matrixJson = jsonObj["matrix"].toObject();
                int rows = matrixJson["rows"].toInt();
                int cols = matrixJson["cols"].toInt();
                
                if (rows > 0 && cols > 0) {
                    // 重新创建矩阵
                    matrix = Eigen::MatrixXd::Zero(rows, cols);

                    // 从数组中恢复矩阵数据
                    QJsonArray matrixData = matrixJson["data"].toArray();
                    int index = 0;
                    for (int i = 0; i < rows && index < matrixData.size(); ++i) {
                        for (int j = 0; j < cols && index < matrixData.size(); ++j) {
                            matrix(i, j) = matrixData[index].toDouble();

                            ++index;

                        }
                    }
                }

                widget->mMatrixWidget->setValuesFromMatrix(matrix);

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
    
    private:
        AudioMatrixWorker* _worker;                                      ///< 工作线程对象
        QThread* _workerThread;                                         ///< 工作线程
        // 移除这行：std::vector<std::shared_ptr<AudioTimestampRingQueue>> _audioBuffer;
        AudioMatrixInterface* widget;
        Eigen::MatrixXd matrix;                                         ///< 矩阵数据
        // 移除这行：std::vector<std::shared_ptr<AudioData>> _outputData;
    };
}