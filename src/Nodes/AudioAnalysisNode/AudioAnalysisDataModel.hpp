#pragma once

#include <QtCore/QObject>
#include <QSignalBlocker>
#include <QDebug>

#include <QtNodes/NodeDelegateModel>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioAnalysisWorker.hpp"
#include "AudioAnalysisInterface.hpp"
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
    class AudioAnalysisDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(double lowMinHz READ lowMinHz WRITE setLowMinHz NOTIFY lowMinHzChanged)
        Q_PROPERTY(double lowMaxHz READ lowMaxHz WRITE setLowMaxHz NOTIFY lowMaxHzChanged)
        Q_PROPERTY(double midMinHz READ midMinHz WRITE setMidMinHz NOTIFY midMinHzChanged)
        Q_PROPERTY(double midMaxHz READ midMaxHz WRITE setMidMaxHz NOTIFY midMaxHzChanged)
        Q_PROPERTY(double highMinHz READ highMinHz WRITE setHighMinHz NOTIFY highMinHzChanged)
        Q_PROPERTY(double highMaxHz READ highMaxHz WRITE setHighMaxHz NOTIFY highMaxHzChanged)
        Q_PROPERTY(int frameSize READ frameSize WRITE setFrameSize NOTIFY frameSizeChanged)
        Q_PROPERTY(int beatIntervalMs READ beatIntervalMs WRITE setBeatIntervalMs NOTIFY beatIntervalMsChanged)
        Q_PROPERTY(double bandAttackMs READ bandAttackMs WRITE setBandAttackMs NOTIFY bandAttackMsChanged)
        Q_PROPERTY(double bandReleaseMs READ bandReleaseMs WRITE setBandReleaseMs NOTIFY bandReleaseMsChanged)

        enum OutputPort : PortIndex {
            LowPort = 0,
            MidPort = 1,
            HighPort = 2,
            LevelPort = 3,
            BeatPort = 4
        };

    public:
        AudioAnalysisDataModel()
            : _worker(new AudioAnalysisWorker())
            , _workerThread(new QThread(this))
            , m_lowOutput(std::make_shared<VariableData>(0.0))
            , m_midOutput(std::make_shared<VariableData>(0.0))
            , m_highOutput(std::make_shared<VariableData>(0.0))
            , m_levelOutput(std::make_shared<VariableData>(0.0))
            , m_beatOutput(std::make_shared<VariableData>(false))
        {
            InPortCount = 2;
            OutPortCount = 5;
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            syncWidgetFromProperties();

            connect(widget->enableButton, &QPushButton::toggled, this, &AudioAnalysisDataModel::setEnabled);
            connect(widget->lowMinHz, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setLowMinHz);
            connect(widget->lowMaxHz, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setLowMaxHz);
            connect(widget->midMinHz, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setMidMinHz);
            connect(widget->midMaxHz, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setMidMaxHz);
            connect(widget->highMinHz, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setHighMinHz);
            connect(widget->highMaxHz, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setHighMaxHz);
            connect(widget->frameSize, &IntDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setFrameSize);
            connect(widget->beatIntervalMs, &IntDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setBeatIntervalMs);
            connect(widget->bandAttackMs, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setBandAttackMs);
            connect(widget->bandReleaseMs, &FloatDragValueWidget::valueChanged, this, &AudioAnalysisDataModel::setBandReleaseMs);

            registerBinding("/enable", "enabled", widget->enableButton);
            registerBinding("/lowMinHz", "lowMinHz", widget->lowMinHz);
            registerBinding("/lowMaxHz", "lowMaxHz", widget->lowMaxHz);
            registerBinding("/midMinHz", "midMinHz", widget->midMinHz);
            registerBinding("/midMaxHz", "midMaxHz", widget->midMaxHz);
            registerBinding("/highMinHz", "highMinHz", widget->highMinHz);
            registerBinding("/highMaxHz", "highMaxHz", widget->highMaxHz);
            registerBinding("/frameSize", "frameSize", widget->frameSize);
            registerBinding("/beatIntervalMs", "beatIntervalMs", widget->beatIntervalMs);
            registerBinding("/bandAttackMs", "bandAttackMs", widget->bandAttackMs);
            registerBinding("/bandReleaseMs", "bandReleaseMs", widget->bandReleaseMs);

            _worker->moveToThread(_workerThread);
            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                pushConfigToWorker();
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioAnalysisWorker::stopProcessing);
            connect(_worker, &AudioAnalysisWorker::processingStatusChanged, this, &AudioAnalysisDataModel::onProcessingStatusChanged);
            connect(_worker, &AudioAnalysisWorker::analysisOutputsChanged, this, &AudioAnalysisDataModel::onGetResult);
            _workerThread->start();
        }

        ~AudioAnalysisDataModel()
        {
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
            switch (portType) {
            case PortType::In:
                if (portIndex == 0) {
                    return AudioData().type();
                }
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            default:
                return VariableData().type();
            }
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                if (portIndex == 0) {
                    return QStringLiteral("AUDIO");
                }
                return QStringLiteral("ENABLE");
            case PortType::Out:
                switch (portIndex) {
                case LowPort:
                    return QStringLiteral("LOW");
                case MidPort:
                    return QStringLiteral("MID");
                case HighPort:
                    return QStringLiteral("HIGH");
                case LevelPort:
                    return QStringLiteral("LEVEL");
                case BeatPort:
                    return QStringLiteral("BEAT");
                default:
                    return QString();
                }
            default:
                return QString();
            }
        }

        bool enabled() const { return m_enabled; }
        double lowMinHz() const { return m_lowMinHz; }
        double lowMaxHz() const { return m_lowMaxHz; }
        double midMinHz() const { return m_midMinHz; }
        double midMaxHz() const { return m_midMaxHz; }
        double highMinHz() const { return m_highMinHz; }
        double highMaxHz() const { return m_highMaxHz; }
        int frameSize() const { return m_frameSize; }
        int beatIntervalMs() const { return m_beatIntervalMs; }
        double bandAttackMs() const { return m_bandAttackMs; }
        double bandReleaseMs() const { return m_bandReleaseMs; }

        void setEnabled(bool enabled)
        {
            if (enabled == m_enabled) {
                return;
            }
            m_enabled = enabled;
            if (widget) {
                const QSignalBlocker blocker(widget->enableButton);
                widget->enableButton->setChecked(enabled);
            }
            Q_EMIT enabledChanged(enabled);
        }

        void setLowMinHz(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_lowMinHz + 1.0)) {
                return;
            }
            m_lowMinHz = v;
            syncWidgetValue(widget->lowMinHz, v);
            Q_EMIT lowMinHzChanged(v);
            pushConfigToWorker();
        }

        void setLowMaxHz(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_lowMaxHz + 1.0)) {
                return;
            }
            m_lowMaxHz = v;
            syncWidgetValue(widget->lowMaxHz, v);
            Q_EMIT lowMaxHzChanged(v);
            pushConfigToWorker();
        }

        void setMidMinHz(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_midMinHz + 1.0)) {
                return;
            }
            m_midMinHz = v;
            syncWidgetValue(widget->midMinHz, v);
            Q_EMIT midMinHzChanged(v);
            pushConfigToWorker();
        }

        void setMidMaxHz(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_midMaxHz + 1.0)) {
                return;
            }
            m_midMaxHz = v;
            syncWidgetValue(widget->midMaxHz, v);
            Q_EMIT midMaxHzChanged(v);
            pushConfigToWorker();
        }

        void setHighMinHz(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_highMinHz + 1.0)) {
                return;
            }
            m_highMinHz = v;
            syncWidgetValue(widget->highMinHz, v);
            Q_EMIT highMinHzChanged(v);
            pushConfigToWorker();
        }

        void setHighMaxHz(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_highMaxHz + 1.0)) {
                return;
            }
            m_highMaxHz = v;
            syncWidgetValue(widget->highMaxHz, v);
            Q_EMIT highMaxHzChanged(v);
            pushConfigToWorker();
        }

        void setFrameSize(int v)
        {
            if (m_frameSize == v) {
                return;
            }
            m_frameSize = v;
            {
                const QSignalBlocker blocker(widget->frameSize);
                widget->frameSize->setValue(v);
            }
            Q_EMIT frameSizeChanged(v);
            pushConfigToWorker();
        }

        void setBeatIntervalMs(int v)
        {
            if (m_beatIntervalMs == v) {
                return;
            }
            m_beatIntervalMs = v;
            {
                const QSignalBlocker blocker(widget->beatIntervalMs);
                widget->beatIntervalMs->setValue(v);
            }
            Q_EMIT beatIntervalMsChanged(v);
            pushConfigToWorker();
        }

        void setBandAttackMs(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_bandAttackMs + 1.0)) {
                return;
            }
            m_bandAttackMs = v;
            syncWidgetValue(widget->bandAttackMs, v);
            Q_EMIT bandAttackMsChanged(v);
            pushConfigToWorker();
        }

        void setBandReleaseMs(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_bandReleaseMs + 1.0)) {
                return;
            }
            m_bandReleaseMs = v;
            syncWidgetValue(widget->bandReleaseMs, v);
            Q_EMIT bandReleaseMsChanged(v);
            pushConfigToWorker();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port) {
            case LowPort:
                return m_lowOutput;
            case MidPort:
                return m_midOutput;
            case HighPort:
                return m_highOutput;
            case LevelPort:
                return m_levelOutput;
            case BeatPort:
                return m_beatOutput;
            default:
                return std::make_shared<VariableData>();
            }
        }

        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (port == 0) {
                auto audioData = std::dynamic_pointer_cast<AudioData>(nodeData);
                std::shared_ptr<AudioTimestampRingQueue> audioBuffer = nullptr;

                if (audioData && audioData->isConnectedToSharedBuffer()) {
                    audioBuffer = audioData->getSharedAudioBuffer();
                }

                QMetaObject::invokeMethod(_worker, "setInputBuffer",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, port),
                                          Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, audioBuffer));
            } else {
                auto data = std::dynamic_pointer_cast<VariableData>(nodeData);
                if (data) {
                    setEnabled(data->value().toBool());
                }
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    public slots:
        QJsonObject save() const override
        {
            QJsonObject values;
            values["enable"] = enabled();
            values["lowMinHz"] = lowMinHz();
            values["lowMaxHz"] = lowMaxHz();
            values["midMinHz"] = midMinHz();
            values["midMaxHz"] = midMaxHz();
            values["highMinHz"] = highMinHz();
            values["highMaxHz"] = highMaxHz();
            values["frameSize"] = frameSize();
            values["beatIntervalMs"] = beatIntervalMs();
            values["bandAttackMs"] = bandAttackMs();
            values["bandReleaseMs"] = bandReleaseMs();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(QJsonObject const &jsonObj) override
        {
            NodeDelegateModel::load(jsonObj);
            QJsonValue v = jsonObj["values"];
            if (!v.isUndefined() && v.isObject()) {
                setEnabled(v["enable"].toBool(false));
                setLowMinHz(v["lowMinHz"].toDouble(20.0));
                setLowMaxHz(v["lowMaxHz"].toDouble(250.0));
                setMidMinHz(v["midMinHz"].toDouble(250.0));
                setMidMaxHz(v["midMaxHz"].toDouble(4000.0));
                setHighMinHz(v["highMinHz"].toDouble(4000.0));
                setHighMaxHz(v["highMaxHz"].toDouble(20000.0));
                setFrameSize(v["frameSize"].toInt(2048));
                setBeatIntervalMs(v["beatIntervalMs"].toInt(120));
                setBandAttackMs(v["bandAttackMs"].toDouble(20.0));
                setBandReleaseMs(v["bandReleaseMs"].toDouble(120.0));
            }
        }

    signals:
        void enabledChanged(bool enabled);
        void lowMinHzChanged(double v);
        void lowMaxHzChanged(double v);
        void midMinHzChanged(double v);
        void midMaxHzChanged(double v);
        void highMinHzChanged(double v);
        void highMaxHzChanged(double v);
        void frameSizeChanged(int v);
        void beatIntervalMsChanged(int v);
        void bandAttackMsChanged(double v);
        void bandReleaseMsChanged(double v);

    protected:
        void afterModelReady() override
        {
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/lowMinHz"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/lowMaxHz"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/midMinHz"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/midMaxHz"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/highMinHz"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/highMaxHz"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/frameSize"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/beatIntervalMs"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/bandAttackMs"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/bandReleaseMs"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addr = ev.address;
            if (addr == makeFullOscAddress("/enable")) {
                setEnabled(ev.payload.toBool());
            } else if (addr == makeFullOscAddress("/lowMinHz")) {
                setLowMinHz(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/lowMaxHz")) {
                setLowMaxHz(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/midMinHz")) {
                setMidMinHz(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/midMaxHz")) {
                setMidMaxHz(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/highMinHz")) {
                setHighMinHz(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/highMaxHz")) {
                setHighMaxHz(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/frameSize")) {
                setFrameSize(ev.payload.toInt());
            } else if (addr == makeFullOscAddress("/beatIntervalMs")) {
                setBeatIntervalMs(ev.payload.toInt());
            } else if (addr == makeFullOscAddress("/bandAttackMs")) {
                setBandAttackMs(ev.payload.toDouble());
            } else if (addr == makeFullOscAddress("/bandReleaseMs")) {
                setBandReleaseMs(ev.payload.toDouble());
            }
        }

        void onProcessingStatusChanged(bool isProcessing)
        {
            Q_UNUSED(isProcessing)
        }

        void onGetResult(double low, double mid, double high, double level, bool beat)
        {
            if (!enabled()) {
                return;
            }

            m_lowOutput = std::make_shared<VariableData>(low);
            m_midOutput = std::make_shared<VariableData>(mid);
            m_highOutput = std::make_shared<VariableData>(high);
            m_levelOutput = std::make_shared<VariableData>(level);
            m_beatOutput = std::make_shared<VariableData>(beat);

            for (PortIndex port = 0; port < OutPortCount; ++port) {
                Q_EMIT dataUpdated(port);
            }
        }

    private:
        template<typename WidgetT>
        void registerBinding(const QString &address, const char *member, WidgetT *control)
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = member;
            b.control = control;
            AbstractDelegateModel::registerExternalBinding(address, this, b);
        }

        void registerBinding(const QString &address, const char *member)
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = member;
            AbstractDelegateModel::registerExternalBinding(address, this, b);
        }

        void syncWidgetValue(FloatDragValueWidget *control, double value)
        {
            const QSignalBlocker blocker(control);
            control->setValue(value);
        }

        void syncWidgetFromProperties()
        {
            widget->enableButton->setChecked(m_enabled);
            syncWidgetValue(widget->lowMinHz, m_lowMinHz);
            syncWidgetValue(widget->lowMaxHz, m_lowMaxHz);
            syncWidgetValue(widget->midMinHz, m_midMinHz);
            syncWidgetValue(widget->midMaxHz, m_midMaxHz);
            syncWidgetValue(widget->highMinHz, m_highMinHz);
            syncWidgetValue(widget->highMaxHz, m_highMaxHz);
            widget->frameSize->setValue(m_frameSize);
            widget->beatIntervalMs->setValue(m_beatIntervalMs);
            syncWidgetValue(widget->bandAttackMs, m_bandAttackMs);
            syncWidgetValue(widget->bandReleaseMs, m_bandReleaseMs);
        }

        void pushConfigToWorker()
        {
            if (!_worker) {
                return;
            }

            QMetaObject::invokeMethod(
                _worker,
                "setAnalysisParams",
                Qt::QueuedConnection,
                Q_ARG(float, static_cast<float>(m_lowMinHz)),
                Q_ARG(float, static_cast<float>(m_lowMaxHz)),
                Q_ARG(float, static_cast<float>(m_midMinHz)),
                Q_ARG(float, static_cast<float>(m_midMaxHz)),
                Q_ARG(float, static_cast<float>(m_highMinHz)),
                Q_ARG(float, static_cast<float>(m_highMaxHz)),
                Q_ARG(int, m_frameSize),
                Q_ARG(int, m_beatIntervalMs),
                Q_ARG(float, static_cast<float>(m_bandAttackMs)),
                Q_ARG(float, static_cast<float>(m_bandReleaseMs)));
        }

        AudioAnalysisWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        AudioAnalysisInterface *widget = new AudioAnalysisInterface();
        std::shared_ptr<VariableData> m_lowOutput;
        std::shared_ptr<VariableData> m_midOutput;
        std::shared_ptr<VariableData> m_highOutput;
        std::shared_ptr<VariableData> m_levelOutput;
        std::shared_ptr<VariableData> m_beatOutput;

        bool m_enabled = false;
        double m_lowMinHz = 20.0;
        double m_lowMaxHz = 250.0;
        double m_midMinHz = 250.0;
        double m_midMaxHz = 4000.0;
        double m_highMinHz = 4000.0;
        double m_highMaxHz = 20000.0;
        int m_frameSize = 2048;
        int m_beatIntervalMs = 120;
        double m_bandAttackMs = 20.0;
        double m_bandReleaseMs = 120.0;
    };
}
