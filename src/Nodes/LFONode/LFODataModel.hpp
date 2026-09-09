#pragma once

#include "NodeDataList.hpp"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "LFOInterface.hpp"

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QTimer>
#include <QSignalBlocker>
#include <QtMath>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;

using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    class LFODataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        enum class WaveType : int {
            Sine = 0,
            Square = 1,
            Triangle = 2,
            Gaussian = 3,
            Ramp = 4,
            Pulse = 5
        };
        Q_ENUM(WaveType)

        Q_PROPERTY(WaveType method READ method WRITE setMethod NOTIFY methodChanged)
        Q_PROPERTY(double period READ period WRITE setPeriod NOTIFY periodChanged)
        Q_PROPERTY(double amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)
        Q_PROPERTY(double offset READ offset WRITE setOffset NOTIFY offsetChanged)
        Q_PROPERTY(double bias READ bias WRITE setBias NOTIFY biasChanged)
        Q_PROPERTY(double phase READ phase WRITE setPhase NOTIFY phaseChanged)
        Q_PROPERTY(double sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
        Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged)
        Q_PROPERTY(double loopCount READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)
        Q_PROPERTY(bool play READ play WRITE setPlay NOTIFY playChanged)

        enum InputPort : PortIndex {
            PlayPort = 0,
            StopPort = 1,
            ResetPort = 2
        };

        enum OutputPort : PortIndex {
            ValuePort = 0,
            TimePort = 1,
            VectorPort = 2
        };

    public:
        LFODataModel()
            : m_widget(new LFOInterface())
        {
            InPortCount = 3;
            OutPortCount = 3;
            CaptionVisible = true;
            Caption = QStringLiteral("LFO");
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;

            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, &LFODataModel::generateWave);

            syncWidgetFromModel();

            connect(m_widget->method, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int index) {
                        setMethod(static_cast<WaveType>(m_widget->method->itemData(index).toInt()));
                    });
            connect(m_widget->period, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setPeriod);
            connect(m_widget->amplitude, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setAmplitude);
            connect(m_widget->offset, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setOffset);
            connect(m_widget->bias, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setBias);
            connect(m_widget->phase, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setPhase);
            connect(m_widget->sampleRate, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setSampleRate);
            connect(m_widget->loop, &QCheckBox::toggled,
                    this, &LFODataModel::setLoop);
            connect(m_widget->loopCount, &FloatDragValueWidget::valueChanged,
                    this, &LFODataModel::setLoopCount);
            connect(m_widget->play, &QPushButton::toggled,
                    this, &LFODataModel::setPlay);
            connect(m_widget->reset, &QPushButton::clicked, this, &LFODataModel::resetTime);

            connect(this, &LFODataModel::methodChanged, this, [this](int v) {
                QSignalBlocker blocker(m_widget->method);
                const int idx = m_widget->method->findData(v);
                if (idx >= 0) {
                    m_widget->method->setCurrentIndex(idx);
                }
                updateBiasEnabled();
            });
            connect(this, &LFODataModel::periodChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->period);
                m_widget->period->setValue(v);
            });
            connect(this, &LFODataModel::amplitudeChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->amplitude);
                m_widget->amplitude->setValue(v);
            });
            connect(this, &LFODataModel::offsetChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->offset);
                m_widget->offset->setValue(v);
            });
            connect(this, &LFODataModel::biasChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->bias);
                m_widget->bias->setValue(v);
            });
            connect(this, &LFODataModel::phaseChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->phase);
                m_widget->phase->setValue(v);
            });
            connect(this, &LFODataModel::sampleRateChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->sampleRate);
                m_widget->sampleRate->setValue(v);
            });
            connect(this, &LFODataModel::loopChanged, this, [this](bool v) {
                QSignalBlocker blocker(m_widget->loop);
                m_widget->loop->setChecked(v);
                m_widget->loopCount->setEnabled(v);
            });
            connect(this, &LFODataModel::loopCountChanged, this, [this](double v) {
                QSignalBlocker blocker(m_widget->loopCount);
                m_widget->loopCount->setValue(v);
            });
            connect(this, &LFODataModel::playChanged, this, [this](bool v) {
                {
                    QSignalBlocker blocker(m_widget->play);
                    m_widget->play->setChecked(v);
                }
                refreshPreviewPlayhead();
            });

            refreshPreviewPlayhead();
            Q_EMIT embeddedWidgetSizeUpdated();
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("method");
                b.control = m_widget->method;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/method"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("period");
                b.control = m_widget->period;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/period"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("amplitude");
                b.control = m_widget->amplitude;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/amplitude"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("offset");
                b.control = m_widget->offset;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/offset"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("bias");
                b.control = m_widget->bias;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/bias"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("phase");
                b.control = m_widget->phase;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/phase"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("sampleRate");
                b.control = m_widget->sampleRate;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/sampleRate"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("loop");
                b.control = m_widget->loop;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/loop"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("loopCount");
                b.control = m_widget->loopCount;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/loopCount"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("play");
                b.control = m_widget->play;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/play"), this, b);
            }
        }

        ~LFODataModel() override
        {
            if (m_timer && m_timer->isActive()) {
                m_timer->stop();
            }

            GlobalEventBus::instance()->unsubscribe(this);

            if (m_widget) {
                m_widget->blockSignals(true);
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portType)
            Q_UNUSED(portIndex)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port) {
            case ValuePort:
                return std::make_shared<VariableData>(m_value);
            case TimePort:
                return std::make_shared<VariableData>(m_time);
            case VectorPort:
                return std::make_shared<VariableData>(QVariantList{m_time, m_value});
            default:
                break;
            }
            return std::make_shared<VariableData>();
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) {
                return;
            }
            auto v = std::dynamic_pointer_cast<VariableData>(data);
            if (!v) {
                return;
            }

            switch (portIndex) {
            case PlayPort:
                if (v->asBool()) {
                    setPlay(true);
                }
                break;
            case StopPort:
                if (v->asBool()) {
                    setPlay(false);
                }
                break;
            case ResetPort:
                if (v->asBool()) {
                    resetTime();
                }
                break;
            default:
                break;
            }
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                switch (portIndex) {
                case PlayPort:
                    return QStringLiteral("PLAY");
                case StopPort:
                    return QStringLiteral("STOP");
                case ResetPort:
                    return QStringLiteral("RESET");
                default:
                    break;
                }
            } else if (portType == PortType::Out) {
                switch (portIndex) {
                case ValuePort:
                    return QStringLiteral("VALUE");
                case TimePort:
                    return QStringLiteral("TIME");
                case VectorPort:
                    return QStringLiteral("VECTOR");
                default:
                    break;
                }
            }
            return {};
        }

        QWidget* embeddedWidget() override
        {
            return m_widget;
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index);
            if (portType == PortType::In) {
                return ConnectionPolicy::Many;
            }
            if (portType == PortType::Out) {
                return ConnectionPolicy::Many;
            }
            return ConnectionPolicy::One;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values[QStringLiteral("method")] = static_cast<int>(m_method);
            values[QStringLiteral("period")] = m_period;
            values[QStringLiteral("amplitude")] = m_amplitude;
            values[QStringLiteral("offset")] = m_offset;
            values[QStringLiteral("bias")] = m_bias;
            values[QStringLiteral("phase")] = m_phase;
            values[QStringLiteral("sampleRate")] = m_sampleRate;
            values[QStringLiteral("loop")] = m_loop;
            values[QStringLiteral("loopCount")] = m_loopCount;
            values[QStringLiteral("play")] = m_play;
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject& p) override
        {
            NodeDelegateModel::load(p);

            const QJsonValue v = p[QStringLiteral("values")];
            if (!v.isObject()) {
                return;
            }

            const QJsonObject values = v.toObject();
            if (values.contains(QStringLiteral("method"))) {
                setMethod(static_cast<WaveType>(values[QStringLiteral("method")].toInt()));
            }
            if (values.contains(QStringLiteral("period"))) {
                setPeriod(values[QStringLiteral("period")].toDouble());
            }
            if (values.contains(QStringLiteral("amplitude"))) {
                setAmplitude(values[QStringLiteral("amplitude")].toDouble());
            }
            if (values.contains(QStringLiteral("offset"))) {
                setOffset(values[QStringLiteral("offset")].toDouble());
            }
            if (values.contains(QStringLiteral("bias"))) {
                setBias(values[QStringLiteral("bias")].toDouble());
            }
            if (values.contains(QStringLiteral("phase"))) {
                setPhase(values[QStringLiteral("phase")].toDouble());
            }
            if (values.contains(QStringLiteral("sampleRate"))) {
                setSampleRate(values[QStringLiteral("sampleRate")].toDouble());
            }
            if (values.contains(QStringLiteral("loop"))) {
                setLoop(values[QStringLiteral("loop")].toBool());
            }
            if (values.contains(QStringLiteral("loopCount"))) {
                setLoopCount(values[QStringLiteral("loopCount")].toDouble());
            }
            if (values.contains(QStringLiteral("play"))) {
                setPlay(values[QStringLiteral("play")].toBool());
            }
        }

    public slots:
        void generateWave()
        {
            if (qFuzzyIsNull(m_sampleRate) || m_sampleRate < 0.0001) {
                m_value = 0.0;
                emitOutputs();
                return;
            }

            m_value = sampleWave();
            emitOutputs();
            refreshPreviewPlayhead();
            m_time += 1.0 / m_sampleRate;

            if (m_loop && m_period > 0.0 && m_time >= m_period * m_loopCount) {
                m_time = m_period * m_loopCount;
                m_value = sampleWave();
                emitOutputs();
                refreshPreviewPlayhead();
                setPlay(false);
            }
        }

        void resetTime()
        {
            m_time = 0.0;
            m_value = sampleWave();
            emitOutputs();
            refreshPreviewPlayhead();
        }

        void setMethod(WaveType v)
        {
            if (m_method == v) {
                return;
            }
            m_method = v;
            Q_EMIT methodChanged(static_cast<int>(m_method));
            refreshPreviewWave();
        }

        void setPeriod(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_period + 1.0)) {
                return;
            }
            m_period = v;
            Q_EMIT periodChanged(m_period);
            refreshPreviewWave();
        }

        void setAmplitude(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_amplitude + 1.0)) {
                return;
            }
            m_amplitude = v;
            Q_EMIT amplitudeChanged(m_amplitude);
            refreshPreviewWave();
        }

        void setOffset(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_offset + 1.0)) {
                return;
            }
            m_offset = v;
            Q_EMIT offsetChanged(m_offset);
            refreshPreviewWave();
        }

        void setBias(double v)
        {
            v = qBound(-1.0, v, 1.0);
            if (qFuzzyCompare(v + 1.0, m_bias + 1.0)) {
                return;
            }
            m_bias = v;
            Q_EMIT biasChanged(m_bias);
            refreshPreviewWave();
        }

        void setPhase(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_phase + 1.0)) {
                return;
            }
            m_phase = v;
            Q_EMIT phaseChanged(m_phase);
            refreshPreviewWave();
        }

        void setSampleRate(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_sampleRate + 1.0)) {
                return;
            }
            m_sampleRate = v;
            if (m_play) {
                restartTimer();
            }
            Q_EMIT sampleRateChanged(m_sampleRate);
            refreshPreviewWave();
        }

        void setLoop(bool v)
        {
            if (m_loop == v) {
                return;
            }
            m_loop = v;
            Q_EMIT loopChanged(m_loop);
        }

        void setLoopCount(double v)
        {
            v = qMax(0.01, v);
            if (qFuzzyCompare(v + 1.0, m_loopCount + 1.0)) {
                return;
            }
            m_loopCount = v;
            Q_EMIT loopCountChanged(m_loopCount);
        }

        void setPlay(bool v)
        {
            if (m_play == v) {
                return;
            }
            m_play = v;
            if (m_play) {
                // 周期用尽后再次 Play：自动从 0 重新开始，无需手动 Reset
                if (m_loop && m_period > 0.0 && m_time >= m_period * m_loopCount) {
                    m_time = 0.0;
                    m_value = sampleWave();
                    emitOutputs();
                    refreshPreviewPlayhead();
                }
                restartTimer();
            } else if (m_timer) {
                m_timer->stop();
            }
            Q_EMIT playChanged(m_play);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrMethod = makeFullOscAddress(QStringLiteral("/method"));
            const QString addrPeriod = makeFullOscAddress(QStringLiteral("/period"));
            const QString addrAmp = makeFullOscAddress(QStringLiteral("/amplitude"));
            const QString addrOffset = makeFullOscAddress(QStringLiteral("/offset"));
            const QString addrBias = makeFullOscAddress(QStringLiteral("/bias"));
            const QString addrPhase = makeFullOscAddress(QStringLiteral("/phase"));
            const QString addrSR = makeFullOscAddress(QStringLiteral("/sampleRate"));
            const QString addrLoop = makeFullOscAddress(QStringLiteral("/loop"));
            const QString addrLoopCount = makeFullOscAddress(QStringLiteral("/loopCount"));
            const QString addrPlay = makeFullOscAddress(QStringLiteral("/play"));
            const QString addrReset = makeFullOscAddress(QStringLiteral("/reset"));

            if (ev.address == addrMethod) {
                setMethod(static_cast<WaveType>(ev.payload.toInt()));
            } else if (ev.address == addrPeriod) {
                setPeriod(ev.payload.toDouble());
            } else if (ev.address == addrAmp) {
                setAmplitude(ev.payload.toDouble());
            } else if (ev.address == addrOffset) {
                setOffset(ev.payload.toDouble());
            } else if (ev.address == addrBias) {
                setBias(ev.payload.toDouble());
            } else if (ev.address == addrPhase) {
                setPhase(ev.payload.toDouble());
            } else if (ev.address == addrSR) {
                setSampleRate(ev.payload.toDouble());
            } else if (ev.address == addrLoop) {
                setLoop(ev.payload.toBool());
            } else if (ev.address == addrLoopCount) {
                setLoopCount(ev.payload.toDouble());
            } else if (ev.address == addrPlay) {
                setPlay(ev.payload.toBool());
            } else if (ev.address == addrReset) {
                if (ev.payload.toBool()) {
                    resetTime();
                }
            }
        }

    signals:
        void methodChanged(int v);
        void periodChanged(double v);
        void amplitudeChanged(double v);
        void offsetChanged(double v);
        void biasChanged(double v);
        void phaseChanged(double v);
        void sampleRateChanged(double v);
        void loopChanged(bool v);
        void loopCountChanged(double v);
        void playChanged(bool v);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/method")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/period")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/amplitude")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/offset")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/bias")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/phase")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/sampleRate")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/loop")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/loopCount")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/play")), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/reset")), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        WaveType method() const { return m_method; }
        double period() const { return m_period; }
        double amplitude() const { return m_amplitude; }
        double offset() const { return m_offset; }
        double bias() const { return m_bias; }
        double phase() const { return m_phase; }
        double sampleRate() const { return m_sampleRate; }
        bool loop() const { return m_loop; }
        double loopCount() const { return m_loopCount; }
        bool play() const { return m_play; }

        void syncWidgetFromModel()
        {
            QSignalBlocker b1(m_widget->method);
            QSignalBlocker b2(m_widget->period);
            QSignalBlocker b3(m_widget->amplitude);
            QSignalBlocker bOffset(m_widget->offset);
            QSignalBlocker bBias(m_widget->bias);
            QSignalBlocker b4(m_widget->phase);
            QSignalBlocker b5(m_widget->sampleRate);
            QSignalBlocker b6(m_widget->loop);
            QSignalBlocker b7(m_widget->loopCount);
            QSignalBlocker bPlay(m_widget->play);

            const int idx = m_widget->method->findData(static_cast<int>(m_method));
            if (idx >= 0) {
                m_widget->method->setCurrentIndex(idx);
            }
            m_widget->period->setValue(m_period);
            m_widget->amplitude->setValue(m_amplitude);
            m_widget->offset->setValue(m_offset);
            m_widget->bias->setValue(m_bias);
            m_widget->phase->setValue(m_phase);
            m_widget->sampleRate->setValue(m_sampleRate);
            m_widget->loop->setChecked(m_loop);
            m_widget->loopCount->setValue(m_loopCount);
            m_widget->loopCount->setEnabled(m_loop);
            m_widget->play->setChecked(m_play);
            updateBiasEnabled();
            refreshPreviewWave();
        }

        void updateBiasEnabled()
        {
            if (!m_widget || !m_widget->bias) {
                return;
            }
            const bool needBias = (m_method == WaveType::Triangle
                                   || m_method == WaveType::Square
                                   || m_method == WaveType::Gaussian);
            m_widget->bias->setEnabled(needBias);
        }

        void emitOutputs()
        {
            Q_EMIT dataUpdated(ValuePort);
            Q_EMIT dataUpdated(TimePort);
            Q_EMIT dataUpdated(VectorPort);
        }

        void refreshPreviewWave()
        {
            if (m_widget && m_widget->preview) {
                m_widget->preview->setWave(static_cast<int>(m_method),
                                           m_period,
                                           m_amplitude,
                                           m_offset,
                                           m_phase,
                                           m_bias,
                                           m_sampleRate);
            }
        }

        void refreshPreviewPlayhead()
        {
            if (m_widget && m_widget->preview) {
                m_widget->preview->setPlayhead(m_time, m_play || m_time > 0.0);
            }
        }

        double pulseWidth() const
        {
            if (m_period <= 0.0 || m_sampleRate <= 0.0) {
                return 0.02;
            }
            return qBound(1e-4, 1.0 / (m_period * m_sampleRate), 0.25);
        }

        double sampleWave() const
        {
            return lfoSample(static_cast<LFOWaveType>(m_method),
                             m_period,
                             m_time,
                             m_phase,
                             m_amplitude,
                             m_offset,
                             m_bias,
                             pulseWidth());
        }

        void restartTimer()
        {
            if (!m_timer) {
                return;
            }
            m_timer->stop();
            if (qFuzzyIsNull(m_sampleRate) || m_sampleRate < 0.0001) {
                return;
            }
            const int intervalMs = qMax(1, static_cast<int>(1000.0 / m_sampleRate));
            m_timer->start(intervalMs);
        }

    private:
        LFOInterface* m_widget = nullptr;
        QTimer* m_timer = nullptr;
        double m_period = 1.0;
        double m_amplitude = 10.0;
        double m_offset = 0.0;
        double m_bias = 0.0;
        double m_phase = 0.0;
        double m_sampleRate = 10.0;
        double m_time = 0.0;
        double m_value = 0.0;
        bool m_loop = false;
        double m_loopCount = 1.0;
        bool m_play = false;
        WaveType m_method = WaveType::Sine;
    };
}
