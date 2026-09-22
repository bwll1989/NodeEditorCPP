#pragma once

#include <QtCore/QObject>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaObject>
#include <QMetaType>
#include <QVector>

#include <QtNodes/NodeDelegateModel>

#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "AudioRouterInterface.h"
#include "AudioRouterWorker.hpp"
#include "PluginDefinition.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief 音频路由器：只做 In→Out 通断路由，不做增益混音
     * 行 = In，列 = Out；每列最多一路输入；端口可编辑
     */
    class AudioRouterDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        AudioRouterDataModel()
            : _worker(new AudioRouterWorker())
            , _workerThread(new QThread(this))
        {
            InPortCount = AudioRouterInterface::kDefaultChannels;
            OutPortCount = AudioRouterInterface::kDefaultChannels;
            widget = new AudioRouterInterface(static_cast<int>(InPortCount),
                                             static_cast<int>(OutPortCount));
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;
            Caption = QStringLiteral("Audio Router");

            qRegisterMetaType<QVector<int>>("QVector<int>");

            routing = widget->routerWidget()->routingMap();
            _worker->initializeBuffers(static_cast<int>(InPortCount),
                                       static_cast<int>(OutPortCount),
                                       routing);
            _worker->moveToThread(_workerThread);

            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                QMetaObject::invokeMethod(_worker, "updateRouting",
                                          Qt::QueuedConnection,
                                          Q_ARG(QVector<int>, routing));
            });
            connect(widget->routerWidget(), &RouterMatrixWidget::routingChanged,
                    this, &AudioRouterDataModel::setRouting);
            connect(_worker, &AudioRouterWorker::processingStatusChanged,
                    this, &AudioRouterDataModel::onProcessingStatusChanged);

            _portSyncTimer = new QTimer(this);
            _portSyncTimer->setInterval(100);
            connect(_portSyncTimer, &QTimer::timeout, this, &AudioRouterDataModel::syncRouterToPorts);
            _portSyncTimer->start();

            _workerThread->start();
        }

        ~AudioRouterDataModel() override
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

            QJsonObject routingJson;
            const QVector<int> map = widget->routerWidget()->routingMap();
            routingJson.insert(QStringLiteral("rows"), widget->inputCount());
            routingJson.insert(QStringLiteral("cols"), widget->outputCount());

            QJsonArray data;
            for (int v : map) {
                data.append(v);
            }
            routingJson.insert(QStringLiteral("map"), data);
            modelJson.insert(QStringLiteral("routing"), routingJson);
            return modelJson;
        }

        void load(QJsonObject const &jsonObj) override
        {
            NodeDelegateModel::load(jsonObj);

            if (!jsonObj.contains(QStringLiteral("routing"))) {
                syncRouterToPorts();
                return;
            }

            const QJsonObject routingJson = jsonObj.value(QStringLiteral("routing")).toObject();
            const int rows = qMax(AudioRouterInterface::kMinChannels,
                                  routingJson.value(QStringLiteral("rows")).toInt(static_cast<int>(InPortCount)));
            const int cols = qMax(AudioRouterInterface::kMinChannels,
                                  routingJson.value(QStringLiteral("cols")).toInt(static_cast<int>(OutPortCount)));

            widget->setChannelCounts(rows, cols);

            QVector<int> map(cols, -1);
            const QJsonArray data = routingJson.value(QStringLiteral("map")).toArray();
            for (int i = 0; i < cols && i < data.size(); ++i) {
                const int src = data.at(i).toInt(-1);
                map[i] = (src >= 0 && src < rows) ? src : -1;
            }

            widget->routerWidget()->setRoutingMap(map);
            routing = map;
            QMetaObject::invokeMethod(_worker, "initializeBuffers",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, rows),
                                      Q_ARG(int, cols),
                                      Q_ARG(QVector<int>, routing));

            _lastSyncedIn = static_cast<unsigned int>(rows);
            _lastSyncedOut = static_cast<unsigned int>(cols);
        }

    public slots:
        void setRouting(QVector<int> map)
        {
            routing = std::move(map);
            QMetaObject::invokeMethod(_worker, "updateRouting",
                                      Qt::QueuedConnection,
                                      Q_ARG(QVector<int>, routing));
        }

        void onProcessingStatusChanged(bool isProcessing)
        {
            Q_UNUSED(isProcessing);
        }

        void syncRouterToPorts()
        {
            const unsigned int inCount = qMax(
                static_cast<unsigned int>(AudioRouterInterface::kMinChannels),
                InPortCount);
            const unsigned int outCount = qMax(
                static_cast<unsigned int>(AudioRouterInterface::kMinChannels),
                OutPortCount);

            if (inCount == _lastSyncedIn && outCount == _lastSyncedOut
                && widget->inputCount() == static_cast<int>(inCount)
                && widget->outputCount() == static_cast<int>(outCount)) {
                return;
            }

            _lastSyncedIn = inCount;
            _lastSyncedOut = outCount;

            widget->setChannelCounts(static_cast<int>(inCount), static_cast<int>(outCount));
            routing = widget->routerWidget()->routingMap();
            QMetaObject::invokeMethod(_worker, "initializeBuffers",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, static_cast<int>(inCount)),
                                      Q_ARG(int, static_cast<int>(outCount)),
                                      Q_ARG(QVector<int>, routing));

            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(static_cast<PortIndex>(i));
            }
            Q_EMIT embeddedWidgetSizeUpdated();
        }

    private:
        AudioRouterWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        AudioRouterInterface *widget = nullptr;
        QVector<int> routing;
        QTimer *_portSyncTimer = nullptr;
        unsigned int _lastSyncedIn = 0;
        unsigned int _lastSyncedOut = 0;
    };
}
