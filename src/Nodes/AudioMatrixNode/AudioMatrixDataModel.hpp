#pragma once

#include <QtCore/QObject>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QMetaType>

#include <QtNodes/NodeDelegateModel>

#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "AudioMatrixInterface.h"
#include "AudioMatrixWorker.hpp"
#include "PluginDefinition.hpp"
#include "Eigen/Core"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief 音频路由矩阵
     * 行 = In 端口，列 = Out 端口；端口可编辑，矩阵尺寸跟随端口数
     */
    class AudioMatrixDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        AudioMatrixDataModel()
            : _worker(new AudioMatrixWorker())
            , _workerThread(new QThread(this))
        {
            InPortCount = AudioMatrixInterface::kDefaultChannels;
            OutPortCount = AudioMatrixInterface::kDefaultChannels;
            widget = new AudioMatrixInterface(static_cast<int>(InPortCount),
                                             static_cast<int>(OutPortCount));
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;
            Caption = PLUGIN_NAME;

            qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");

            matrix = widget->matrixWidget()->getLinearValuesAsMatrix();
            _worker->initializeBuffers(static_cast<int>(InPortCount),
                                       static_cast<int>(OutPortCount),
                                       matrix);
            _worker->moveToThread(_workerThread);

            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                QMetaObject::invokeMethod(_worker, "updateMatrix",
                                          Qt::QueuedConnection,
                                          Q_ARG(Eigen::MatrixXd, matrix));
            });
            connect(widget->matrixWidget(), &MatrixWidget::valueChanged,
                    this, &AudioMatrixDataModel::setMatrix);
            connect(_worker, &AudioMatrixWorker::processingStatusChanged,
                    this, &AudioMatrixDataModel::onProcessingStatusChanged);

            // 框架直接改 In/OutPortCount，无专用回调；轮询对齐矩阵尺寸
            _portSyncTimer = new QTimer(this);
            _portSyncTimer->setInterval(100);
            connect(_portSyncTimer, &QTimer::timeout, this, &AudioMatrixDataModel::syncMatrixToPorts);
            _portSyncTimer->start();

            _workerThread->start();
        }

        ~AudioMatrixDataModel() override
        {
            if (_portSyncTimer) {
                _portSyncTimer->stop();
            }
            if (_worker) {
                _worker->stopProcessing();
            }
            if (_workerThread && _workerThread->isRunning()) {
                _workerThread->quit();
                _workerThread->wait(3000);
            }
            if (_worker) {
                _worker->deleteLater();
            }
        }

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return AudioData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QStringLiteral("IN%1").arg(portIndex + 1);
            case PortType::Out:
                return QStringLiteral("OUT%1").arg(portIndex + 1);
            default:
                return {};
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port >= OutPortCount) {
                return std::make_shared<AudioData>();
            }

            auto outputData = std::make_shared<AudioData>();
            auto outputBuffer = _worker->getOutputBuffer(static_cast<int>(port));
            if (outputBuffer) {
                outputData->setSharedAudioBuffer(outputBuffer);
            }
            return outputData;
        }

        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (port >= InPortCount) {
                return;
            }

            auto audioData = std::dynamic_pointer_cast<AudioData>(nodeData);
            std::shared_ptr<AudioTimestampRingQueue> audioBuffer;
            if (audioData && audioData->isConnectedToSharedBuffer()) {
                audioBuffer = audioData->getSharedAudioBuffer();
            }

            QMetaObject::invokeMethod(_worker, "setInputBuffer",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, static_cast<int>(port)),
                                      Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, audioBuffer));
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();

            QJsonObject matrixJson;
            const Eigen::MatrixXd dbMatrix = widget->matrixWidget()->getValuesAsMatrix();
            matrixJson.insert(QStringLiteral("rows"), static_cast<int>(dbMatrix.rows()));
            matrixJson.insert(QStringLiteral("cols"), static_cast<int>(dbMatrix.cols()));

            QJsonArray data;
            for (int i = 0; i < dbMatrix.rows(); ++i) {
                for (int j = 0; j < dbMatrix.cols(); ++j) {
                    data.append(dbMatrix(i, j));
                }
            }
            matrixJson.insert(QStringLiteral("data"), data);
            modelJson.insert(QStringLiteral("matrix"), matrixJson);
            return modelJson;
        }

        void load(QJsonObject const &jsonObj) override
        {
            NodeDelegateModel::load(jsonObj);

            // 端口数由场景 json 的 input-count/output-count 恢复；此处只恢复增益
            // 若 internal 里带有行列，在端口尚未写入前先按矩阵数据尺寸对齐一次
            if (!jsonObj.contains(QStringLiteral("matrix"))) {
                syncMatrixToPorts();
                return;
            }

            const QJsonObject matrixJson = jsonObj.value(QStringLiteral("matrix")).toObject();
            const int rows = qMax(AudioMatrixInterface::kMinChannels,
                                  matrixJson.value(QStringLiteral("rows")).toInt(static_cast<int>(InPortCount)));
            const int cols = qMax(AudioMatrixInterface::kMinChannels,
                                  matrixJson.value(QStringLiteral("cols")).toInt(static_cast<int>(OutPortCount)));

            widget->setChannelCounts(rows, cols);

            Eigen::MatrixXd dbMatrix = Eigen::MatrixXd::Constant(rows, cols, -60.0);
            const QJsonArray data = matrixJson.value(QStringLiteral("data")).toArray();
            int index = 0;
            for (int i = 0; i < rows && index < data.size(); ++i) {
                for (int j = 0; j < cols && index < data.size(); ++j) {
                    dbMatrix(i, j) = data.at(index).toDouble();
                    ++index;
                }
            }

            widget->matrixWidget()->setValuesFromMatrix(dbMatrix);
            matrix = widget->matrixWidget()->getLinearValuesAsMatrix();
            QMetaObject::invokeMethod(_worker, "initializeBuffers",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, rows),
                                      Q_ARG(int, cols),
                                      Q_ARG(Eigen::MatrixXd, matrix));

            // 随后场景会写入最终端口数，定时器会再对齐
            _lastSyncedIn = static_cast<unsigned int>(rows);
            _lastSyncedOut = static_cast<unsigned int>(cols);
        }

    public slots:
        void setMatrix(Eigen::MatrixXd mat)
        {
            matrix = std::move(mat);
            QMetaObject::invokeMethod(_worker, "updateMatrix",
                                      Qt::QueuedConnection,
                                      Q_ARG(Eigen::MatrixXd, matrix));
        }

        void onProcessingStatusChanged(bool isProcessing)
        {
            Q_UNUSED(isProcessing);
        }

        void syncMatrixToPorts()
        {
            // 矩阵尺寸跟随端口数（无硬上限；过大时 UI 控件数量会先成为瓶颈）
            const unsigned int inCount = qMax(
                static_cast<unsigned int>(AudioMatrixInterface::kMinChannels),
                InPortCount);
            const unsigned int outCount = qMax(
                static_cast<unsigned int>(AudioMatrixInterface::kMinChannels),
                OutPortCount);

            if (inCount == _lastSyncedIn && outCount == _lastSyncedOut
                && widget->inputCount() == static_cast<int>(inCount)
                && widget->outputCount() == static_cast<int>(outCount)) {
                return;
            }

            _lastSyncedIn = inCount;
            _lastSyncedOut = outCount;

            widget->setChannelCounts(static_cast<int>(inCount), static_cast<int>(outCount));
            matrix = widget->matrixWidget()->getLinearValuesAsMatrix();
            QMetaObject::invokeMethod(_worker, "initializeBuffers",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, static_cast<int>(inCount)),
                                      Q_ARG(int, static_cast<int>(outCount)),
                                      Q_ARG(Eigen::MatrixXd, matrix));

            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(static_cast<PortIndex>(i));
            }
            Q_EMIT embeddedWidgetSizeUpdated();
        }

    private:
        AudioMatrixWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        AudioMatrixInterface *widget = nullptr;
        Eigen::MatrixXd matrix;
        QTimer *_portSyncTimer = nullptr;
        unsigned int _lastSyncedIn = 0;
        unsigned int _lastSyncedOut = 0;
    };
}
