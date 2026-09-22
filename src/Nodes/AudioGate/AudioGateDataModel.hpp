#pragma once

#include <QtCore/QObject>
#include <QThread>
#include <QtNodes/NodeDelegateModel>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "AudioGateInterface.h"
#include "PluginDefinition.hpp"
#include "AudioGateWorker.hpp"
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
     * @brief QSC 风格 Noise Gate
     * Channels 路 In/Out 一一对应；各通道 RMS ≥ Threshold 时无损通过，否则按 Depth 衰减。
     */
    class AudioGateDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int channels READ channels WRITE setChannels NOTIFY channelsChanged)
        Q_PROPERTY(double thresholdLevel READ thresholdLevel WRITE setThresholdLevel NOTIFY thresholdLevelChanged)
        Q_PROPERTY(double depth READ depth WRITE setDepth NOTIFY depthChanged)
        Q_PROPERTY(double attackTime READ attackTime WRITE setAttackTime NOTIFY attackTimeChanged)
        Q_PROPERTY(double holdTime READ holdTime WRITE setHoldTime NOTIFY holdTimeChanged)
        Q_PROPERTY(double releaseTime READ releaseTime WRITE setReleaseTime NOTIFY releaseTimeChanged)

    public:
        AudioGateDataModel()
            : _worker(new AudioGateWorker())
            , _workerThread(new QThread(this))
        {
            widget = new AudioGateInterface();
            m_channels = AudioGateInterface::kDefaultChannels;
            InPortCount = static_cast<unsigned int>(m_channels);
            OutPortCount = static_cast<unsigned int>(m_channels);
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            m_thresholdLevel = widget->thresholdSpin->value();
            m_depth = widget->depthSpin->value();
            m_attackTime = widget->attackSpin->value();
            m_holdTime = widget->holdSpin->value();
            m_releaseTime = widget->releaseSpin->value();

            registerBindings();

            _worker->initializeBuffers(m_channels);
            _worker->moveToThread(_workerThread);

            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                pushParamsToWorker();
            });

            connect(widget->channelsSpin, &IntDragValueWidget::valueChanged, this, &AudioGateDataModel::setChannels);
            connect(widget->thresholdSpin, &FloatDragValueWidget::valueChanged, this, &AudioGateDataModel::setThresholdLevel);
            connect(widget->depthSpin, &FloatDragValueWidget::valueChanged, this, &AudioGateDataModel::setDepth);
            connect(widget->attackSpin, &FloatDragValueWidget::valueChanged, this, &AudioGateDataModel::setAttackTime);
            connect(widget->holdSpin, &FloatDragValueWidget::valueChanged, this, &AudioGateDataModel::setHoldTime);
            connect(widget->releaseSpin, &FloatDragValueWidget::valueChanged, this, &AudioGateDataModel::setReleaseTime);

            connect(this, &AudioGateDataModel::channelsChanged, this, [this](int value) {
                widget->channelsSpin->setValue(value);
                applyChannelCount(value, true);
            });
            connect(this, &AudioGateDataModel::thresholdLevelChanged, this, [this](double value) {
                widget->thresholdSpin->setValue(value);
                QMetaObject::invokeMethod(_worker, "setThreshold", Qt::QueuedConnection, Q_ARG(double, value));
            });
            connect(this, &AudioGateDataModel::depthChanged, this, [this](double value) {
                widget->depthSpin->setValue(value);
                QMetaObject::invokeMethod(_worker, "setDepth", Qt::QueuedConnection, Q_ARG(double, value));
            });
            connect(this, &AudioGateDataModel::attackTimeChanged, this, [this](double value) {
                widget->attackSpin->setValue(value);
                QMetaObject::invokeMethod(_worker, "setAttack", Qt::QueuedConnection, Q_ARG(double, value));
            });
            connect(this, &AudioGateDataModel::holdTimeChanged, this, [this](double value) {
                widget->holdSpin->setValue(value);
                QMetaObject::invokeMethod(_worker, "setHold", Qt::QueuedConnection, Q_ARG(double, value));
            });
            connect(this, &AudioGateDataModel::releaseTimeChanged, this, [this](double value) {
                widget->releaseSpin->setValue(value);
                QMetaObject::invokeMethod(_worker, "setRelease", Qt::QueuedConnection, Q_ARG(double, value));
            });

            _workerThread->start();
        }

        ~AudioGateDataModel() override
        {
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
            Q_UNUSED(portType);
            Q_UNUSED(portIndex);
            return AudioData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QStringLiteral("In %1").arg(portIndex + 1);
            case PortType::Out:
                return QStringLiteral("Out %1").arg(portIndex + 1);
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

        int channels() const { return m_channels; }
        double thresholdLevel() const { return m_thresholdLevel; }
        double depth() const { return m_depth; }
        double attackTime() const { return m_attackTime; }
        double holdTime() const { return m_holdTime; }
        double releaseTime() const { return m_releaseTime; }

        void setChannels(int value)
        {
            value = qBound(AudioGateInterface::kMinChannels, value, AudioGateInterface::kMaxChannels);
            if (value == m_channels) {
                return;
            }
            m_channels = value;
            Q_EMIT channelsChanged(value);
        }

        void setThresholdLevel(double value)
        {
            value = qBound(-60.0, value, 20.0);
            if (qFuzzyCompare(value + 1.0, m_thresholdLevel + 1.0)) {
                return;
            }
            m_thresholdLevel = value;
            Q_EMIT thresholdLevelChanged(value);
        }

        void setDepth(double value)
        {
            value = qBound(0.0, value, 60.0);
            if (qFuzzyCompare(value + 1.0, m_depth + 1.0)) {
                return;
            }
            m_depth = value;
            Q_EMIT depthChanged(value);
        }

        void setAttackTime(double value)
        {
            value = qBound(0.1, value, 10000.0);
            if (qFuzzyCompare(value + 1.0, m_attackTime + 1.0)) {
                return;
            }
            m_attackTime = value;
            Q_EMIT attackTimeChanged(value);
        }

        void setHoldTime(double value)
        {
            value = qBound(10.0, value, 10000.0);
            if (qFuzzyCompare(value + 1.0, m_holdTime + 1.0)) {
                return;
            }
            m_holdTime = value;
            Q_EMIT holdTimeChanged(value);
        }

        void setReleaseTime(double value)
        {
            value = qBound(10.0, value, 10000.0);
            if (qFuzzyCompare(value + 1.0, m_releaseTime + 1.0)) {
                return;
            }
            m_releaseTime = value;
            Q_EMIT releaseTimeChanged(value);
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values["channels"] = channels();
            values["threshold"] = thresholdLevel();
            values["depth"] = depth();
            values["attack"] = attackTime();
            values["hold"] = holdTime();
            values["release"] = releaseTime();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            NodeDelegateModel::load(p);

            QJsonObject values = p.value(QStringLiteral("values")).toObject();
            if (values.isEmpty()) {
                values = p;
            }

            if (values.contains("channels")) {
                const int count = qBound(AudioGateInterface::kMinChannels,
                                         values["channels"].toInt(m_channels),
                                         AudioGateInterface::kMaxChannels);
                m_channels = count;
                widget->channelsSpin->setValue(count);
                applyChannelCount(count, false);
            }
            if (values.contains("threshold")) {
                setThresholdLevel(values["threshold"].toDouble());
            }
            if (values.contains("depth")) {
                setDepth(values["depth"].toDouble());
            }
            if (values.contains("attack")) {
                setAttackTime(values["attack"].toDouble());
            }
            if (values.contains("hold")) {
                setHoldTime(values["hold"].toDouble());
            }
            if (values.contains("release")) {
                setReleaseTime(values["release"].toDouble());
            }
        }

    signals:
        void channelsChanged(int value);
        void thresholdLevelChanged(double value);
        void depthChanged(double value);
        void attackTimeChanged(double value);
        void holdTimeChanged(double value);
        void releaseTimeChanged(double value);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/channels"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/threshold"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/depth"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/attack"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/hold"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/release"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address == makeFullOscAddress("/channels")) {
                setChannels(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/threshold")) {
                setThresholdLevel(ev.payload.toDouble());
            } else if (ev.address == makeFullOscAddress("/depth")) {
                setDepth(ev.payload.toDouble());
            } else if (ev.address == makeFullOscAddress("/attack")) {
                setAttackTime(ev.payload.toDouble());
            } else if (ev.address == makeFullOscAddress("/hold")) {
                setHoldTime(ev.payload.toDouble());
            } else if (ev.address == makeFullOscAddress("/release")) {
                setReleaseTime(ev.payload.toDouble());
            }
        }

    private:
        void applyPortCount(PortType portType, unsigned int newCount, bool notifyPorts)
        {
            unsigned int &count = (portType == PortType::In) ? InPortCount : OutPortCount;
            const unsigned int oldCount = count;
            if (newCount == oldCount) {
                return;
            }
            if (notifyPorts) {
                if (newCount > oldCount) {
                    Q_EMIT portsAboutToBeInserted(portType, oldCount, newCount - 1);
                    count = newCount;
                    Q_EMIT portsInserted();
                } else {
                    Q_EMIT portsAboutToBeDeleted(portType, newCount, oldCount - 1);
                    count = newCount;
                    Q_EMIT portsDeleted();
                }
            } else {
                count = newCount;
            }
        }

        void applyChannelCount(int channelCount, bool notifyPorts)
        {
            const unsigned int count = static_cast<unsigned int>(
                qBound(AudioGateInterface::kMinChannels,
                       channelCount,
                       AudioGateInterface::kMaxChannels));

            applyPortCount(PortType::In, count, notifyPorts);
            applyPortCount(PortType::Out, count, notifyPorts);

            QMetaObject::invokeMethod(_worker, "initializeBuffers",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, static_cast<int>(count)));

            if (notifyPorts) {
                for (unsigned int i = 0; i < OutPortCount; ++i) {
                    Q_EMIT dataUpdated(static_cast<PortIndex>(i));
                }
                Q_EMIT embeddedWidgetSizeUpdated();
            }
        }

        void registerBindings()
        {
            auto bind = [this](const char *member, const QString &address, QWidget *control) {
                NodeDelegateModel::ExternalBinding b;
                b.member = member;
                b.control = control;
                AbstractDelegateModel::registerExternalBinding(address, this, b);
            };
            bind("channels", QStringLiteral("/channels"), widget->channelsSpin);
            bind("thresholdLevel", QStringLiteral("/threshold"), widget->thresholdSpin);
            bind("depth", QStringLiteral("/depth"), widget->depthSpin);
            bind("attackTime", QStringLiteral("/attack"), widget->attackSpin);
            bind("holdTime", QStringLiteral("/hold"), widget->holdSpin);
            bind("releaseTime", QStringLiteral("/release"), widget->releaseSpin);
        }

        void pushParamsToWorker()
        {
            QMetaObject::invokeMethod(_worker, "setThreshold", Qt::QueuedConnection, Q_ARG(double, m_thresholdLevel));
            QMetaObject::invokeMethod(_worker, "setDepth", Qt::QueuedConnection, Q_ARG(double, m_depth));
            QMetaObject::invokeMethod(_worker, "setAttack", Qt::QueuedConnection, Q_ARG(double, m_attackTime));
            QMetaObject::invokeMethod(_worker, "setHold", Qt::QueuedConnection, Q_ARG(double, m_holdTime));
            QMetaObject::invokeMethod(_worker, "setRelease", Qt::QueuedConnection, Q_ARG(double, m_releaseTime));
        }

        AudioGateWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        AudioGateInterface *widget = nullptr;
        int m_channels = AudioGateInterface::kDefaultChannels;
        double m_thresholdLevel = -40.0;
        double m_depth = 60.0;
        double m_attackTime = 1.0;
        double m_holdTime = 100.0;
        double m_releaseTime = 200.0;
    };
}
