#pragma once

#include <QtCore/QObject>
#include <QThread>
#include <QtNodes/NodeDelegateModel>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "AudioDelayInterface.h"
#include "PluginDefinition.hpp"
#include "AudioDelayWorker.hpp"
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
     * @brief 多路音频时间戳延时
     * Channels 路 In/Out 一一对应；PCM 透传，输出时间戳 += Delay Frames。
     */
    class AudioDelayDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int channels READ channels WRITE setChannels NOTIFY channelsChanged)
        Q_PROPERTY(int delayFrames READ delayFrames WRITE setDelayFrames NOTIFY delayFramesChanged)

    public:
        AudioDelayDataModel()
            : _worker(new AudioDelayWorker())
            , _workerThread(new QThread(this))
        {
            widget = new AudioDelayInterface();
            m_channels = AudioDelayInterface::kDefaultChannels;
            m_delayFrames = AudioDelayInterface::kDefaultDelayFrames;
            InPortCount = static_cast<unsigned int>(m_channels);
            OutPortCount = static_cast<unsigned int>(m_channels);
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            registerBindings();

            _worker->initializeBuffers(m_channels);
            _worker->moveToThread(_workerThread);

            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                pushParamsToWorker();
            });

            connect(widget->channelsSpin, &IntDragValueWidget::valueChanged, this, &AudioDelayDataModel::setChannels);
            connect(widget->delayFramesSpin, &IntDragValueWidget::valueChanged, this, &AudioDelayDataModel::setDelayFrames);

            connect(this, &AudioDelayDataModel::channelsChanged, this, [this](int value) {
                widget->channelsSpin->setValue(value);
                applyChannelCount(value, true);
            });
            connect(this, &AudioDelayDataModel::delayFramesChanged, this, [this](int value) {
                widget->delayFramesSpin->setValue(value);
                QMetaObject::invokeMethod(_worker, "setDelayFrames", Qt::QueuedConnection, Q_ARG(int, value));
            });

            _workerThread->start();
        }

        ~AudioDelayDataModel() override
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
        int delayFrames() const { return m_delayFrames; }

        void setChannels(int value)
        {
            value = qBound(AudioDelayInterface::kMinChannels, value, AudioDelayInterface::kMaxChannels);
            if (value == m_channels) {
                return;
            }
            m_channels = value;
            Q_EMIT channelsChanged(value);
        }

        void setDelayFrames(int value)
        {
            value = qBound(AudioDelayInterface::kMinDelayFrames, value, AudioDelayInterface::kMaxDelayFrames);
            if (value == m_delayFrames) {
                return;
            }
            m_delayFrames = value;
            Q_EMIT delayFramesChanged(value);
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values["channels"] = channels();
            values["delayFrames"] = delayFrames();

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
                const int count = qBound(AudioDelayInterface::kMinChannels,
                                         values["channels"].toInt(m_channels),
                                         AudioDelayInterface::kMaxChannels);
                m_channels = count;
                widget->channelsSpin->setValue(count);
                applyChannelCount(count, false);
            }
            if (values.contains("delayFrames")) {
                setDelayFrames(values["delayFrames"].toInt());
            } else if (values.contains("delay")) {
                setDelayFrames(values["delay"].toInt());
            }
        }

    signals:
        void channelsChanged(int value);
        void delayFramesChanged(int value);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/channels"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/delay"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/delay_frames"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address == makeFullOscAddress("/channels")) {
                setChannels(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/delay")
                       || ev.address == makeFullOscAddress("/delay_frames")) {
                setDelayFrames(ev.payload.toInt());
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
                qBound(AudioDelayInterface::kMinChannels,
                       channelCount,
                       AudioDelayInterface::kMaxChannels));

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
            bind("delayFrames", QStringLiteral("/delay"), widget->delayFramesSpin);
        }

        void pushParamsToWorker()
        {
            QMetaObject::invokeMethod(_worker, "setDelayFrames", Qt::QueuedConnection, Q_ARG(int, m_delayFrames));
        }

        AudioDelayWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        AudioDelayInterface *widget = nullptr;
        int m_channels = AudioDelayInterface::kDefaultChannels;
        int m_delayFrames = AudioDelayInterface::kDefaultDelayFrames;
    };
}
