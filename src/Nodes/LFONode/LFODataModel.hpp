#pragma once

#include "NodeDataList.hpp"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QTimer>
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
            Triangle = 2
        };
        Q_ENUM(WaveType)

        Q_PROPERTY(WaveType method READ method WRITE setMethod NOTIFY methodChanged)
        Q_PROPERTY(double frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
        Q_PROPERTY(double amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)
        Q_PROPERTY(double phase READ phase WRITE setPhase NOTIFY phaseChanged)
        Q_PROPERTY(double sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
        Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

    public:
        LFODataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "LFO";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, &LFODataModel::generateWave);

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "method";
                AbstractDelegateModel::registerExternalBinding("/method", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "frequency";
                AbstractDelegateModel::registerExternalBinding("/frequency", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "amplitude";
                AbstractDelegateModel::registerExternalBinding("/amplitude", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "phase";
                AbstractDelegateModel::registerExternalBinding("/phase", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "sampleRate";
                AbstractDelegateModel::registerExternalBinding("/sampleRate", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "running";
                AbstractDelegateModel::registerExternalBinding("/running", this, b);
            }

            m_frequency = 5.0;
            m_amplitude = 10.0;
            m_phase = 0.0;
            m_sampleRate = 10.0;
            m_method = WaveType::Sine;
            m_running = false;
            m_time = 0.0;
            m_value = 0.0;
        }

        ~LFODataModel() override
        {
            if (m_timer && m_timer->isActive()) {
                m_timer->stop();
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            case PortType::None:
                break;
            default:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            auto result = std::make_shared<VariableData>(m_value);
            result->insert("method", static_cast<int>(m_method));
            return result;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex);
            if (!data) {
                return;
            }
            auto v = std::dynamic_pointer_cast<VariableData>(data);
            if (!v) {
                return;
            }
            if (v->value().toBool()) {
                setRunning(true);
            }
        }

        double generateSineWave(double amplitude, double frequency, double time, double phase) {
            return amplitude * qSin(2 * M_PI * frequency * time + phase);
        }

        double generateSquareWave(double amplitude, double frequency, double time, double phase) {
            return amplitude * (qSin(2 * M_PI * frequency * time + phase) >= 0 ? 1.0 : -1.0);
        }

        double generateTriangleWave(double amplitude, double frequency, double time, double phase) {
            if (qFuzzyIsNull(frequency)) {
                return 0.0;
            }
            const double period = 1.0 / frequency;
            const double t = std::fmod(time + phase / (2 * M_PI * frequency), period);
            const double v = (4.0 * amplitude / period) * (t - period / 2.0);
            return (v >= 0.0 ? 1.0 : -1.0) * qAbs(v);
        }
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }

            return result;
        }
    public slots:
        void generateWave() {
            if (qFuzzyIsNull(m_sampleRate) || m_sampleRate < 0.0001) {
                m_value = 0.0;
                Q_EMIT dataUpdated(0);
                return;
            }

            switch (m_method) {
                case WaveType::Sine:
                    m_value = generateSineWave(m_amplitude, m_frequency, m_time, m_phase);
                    break;
                case WaveType::Square:
                    m_value = generateSquareWave(m_amplitude, m_frequency, m_time, m_phase);
                    break;
                case WaveType::Triangle:
                    m_value = generateTriangleWave(m_amplitude, m_frequency, m_time, m_phase);
                    break;
            }

            m_time += 1.0 / m_sampleRate;
            Q_EMIT dataUpdated(0);
        }

        void setMethod(WaveType v)
        {
            if (m_method == v) {
                return;
            }
            m_method = v;
            m_time = 0.0;
            Q_EMIT methodChanged(static_cast<int>(m_method));
        }

        void setFrequency(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_frequency + 1.0)) {
                return;
            }
            m_frequency = v;
            m_time = 0.0;
            Q_EMIT frequencyChanged(m_frequency);
        }

        void setAmplitude(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_amplitude + 1.0)) {
                return;
            }
            m_amplitude = v;
            m_time = 0.0;
            Q_EMIT amplitudeChanged(m_amplitude);
        }

        void setPhase(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_phase + 1.0)) {
                return;
            }
            m_phase = v;
            m_time = 0.0;
            Q_EMIT phaseChanged(m_phase);
        }

        void setSampleRate(double v)
        {
            if (qFuzzyCompare(v + 1.0, m_sampleRate + 1.0)) {
                return;
            }
            m_sampleRate = v;
            if (m_running) {
                restartTimer();
            }
            Q_EMIT sampleRateChanged(m_sampleRate);
        }

        void setRunning(bool v)
        {
            if (m_running == v) {
                return;
            }
            m_running = v;
            if (m_running) {
                restartTimer();
            } else {
                if (m_timer) {
                    m_timer->stop();
                }
            }
            Q_EMIT runningChanged(m_running);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrMethod = makeFullOscAddress("/method");
            const QString addrFreq = makeFullOscAddress("/frequency");
            const QString addrAmp = makeFullOscAddress("/amplitude");
            const QString addrPhase = makeFullOscAddress("/phase");
            const QString addrSR = makeFullOscAddress("/sampleRate");
            const QString addrRun = makeFullOscAddress("/running");

            if (ev.address == addrMethod) {
                setMethod(static_cast<WaveType>(ev.payload.toInt()));
            } else if (ev.address == addrFreq) {
                setFrequency(ev.payload.toDouble());
            } else if (ev.address == addrAmp) {
                setAmplitude(ev.payload.toDouble());
            } else if (ev.address == addrPhase) {
                setPhase(ev.payload.toDouble());
            } else if (ev.address == addrSR) {
                setSampleRate(ev.payload.toDouble());
            } else if (ev.address == addrRun) {
                setRunning(ev.payload.toBool());
            }
        }

    signals:
        void methodChanged(int v);
        void frequencyChanged(double v);
        void amplitudeChanged(double v);
        void phaseChanged(double v);
        void sampleRateChanged(double v);
        void runningChanged(bool v);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/method"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/frequency"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/amplitude"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/phase"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/sampleRate"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/running"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        WaveType method() const { return m_method; }
        double frequency() const { return m_frequency; }
        double amplitude() const { return m_amplitude; }
        double phase() const { return m_phase; }
        double sampleRate() const { return m_sampleRate; }
        bool running() const { return m_running; }

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
            m_time = 0.0;
            m_timer->start(intervalMs);
        }

    private:
        QTimer* m_timer = nullptr;
        std::shared_ptr<VariableData> inData;
        double m_frequency = 5.0;
        double m_amplitude = 10.0;
        double m_phase = 0.0;
        double m_sampleRate = 10.0;
        double m_time = 0.0;
        double m_value = 0.0;
        bool m_running = false;
        WaveType m_method = WaveType::Sine;
    };
}