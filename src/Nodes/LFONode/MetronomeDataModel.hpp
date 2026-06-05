#pragma once

#include "NodeDataList.hpp"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "MetronomeInterface.hpp"

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QTimer>
#include <QSignalBlocker>
#include <QPushButton>
#include <QLineEdit>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

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
    class MetronomeDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
        Q_PROPERTY(QString signalValue READ signalValue WRITE setSignalValue NOTIFY signalValueChanged)
        Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)

        enum InputPort : PortIndex {
            IntervalPort = 0,
            ValuePort = 1,
            EnablePort = 2
        };

    public:
        MetronomeDataModel()
            : m_widget(new MetronomeInterface())
        {
            InPortCount = 3;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Metronome";
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;

            m_timer = new QTimer(this);
            connect(m_timer, &QTimer::timeout, this, &MetronomeDataModel::onTick);

            m_widget->interval->setValue(m_interval);
            m_widget->signalValue->setText(m_signalValue);

            connect(m_widget->interval, &IntDragValueWidget::valueChanged,
                    this, &MetronomeDataModel::setInterval);
            connect(m_widget->signalValue, &QLineEdit::textChanged,
                    this, &MetronomeDataModel::setSignalValue);
            connect(m_widget->start, &QPushButton::toggled,
                    this, &MetronomeDataModel::setRunning);

            connect(this, &MetronomeDataModel::intervalChanged, this, [this](int v) {
                QSignalBlocker blocker(m_widget->interval);
                m_widget->interval->setValue(v);
            });
            connect(this, &MetronomeDataModel::signalValueChanged, this, [this](const QString& v) {
                QSignalBlocker blocker(m_widget->signalValue);
                m_widget->signalValue->setText(v);
            });
            connect(this, &MetronomeDataModel::runningChanged, this, [this](bool v) {
                QSignalBlocker blocker(m_widget->start);
                m_widget->start->setChecked(v);
            });

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "interval";
                b.control = m_widget->interval;
                AbstractDelegateModel::registerExternalBinding("/interval", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "signalValue";
                b.control = m_widget->signalValue;
                AbstractDelegateModel::registerExternalBinding("/signalValue", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "running";
                b.control = m_widget->start;
                AbstractDelegateModel::registerExternalBinding("/running", this, b);
            }
        }

        ~MetronomeDataModel() override
        {
            if (m_timer) {
                m_timer->stop();
            }

            GlobalEventBus::instance()->unsubscribe(this);

            if (m_widget) {
                m_widget->blockSignals(true);
                if (m_widget->interval) {
                    m_widget->interval->blockSignals(true);
                }
                if (m_widget->signalValue) {
                    m_widget->signalValue->blockSignals(true);
                }
                if (m_widget->start) {
                    m_widget->start->blockSignals(true);
                }
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return std::make_shared<VariableData>(m_value);
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
            case IntervalPort:
                setInterval(v->value().toInt());
                break;
            case ValuePort:
                setSignalValue(v->value().toString());
                break;
            case EnablePort:
                setRunning(v->value().toBool());
                break;
            default:
                break;
            }
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                switch (portIndex) {
                case IntervalPort:
                    return QStringLiteral("INTERVAL");
                case ValuePort:
                    return QStringLiteral("VALUE");
                case EnablePort:
                    return QStringLiteral("Enable");
                default:
                    break;
                }
            } else if (portType == PortType::Out) {
                return QStringLiteral("OUTPUT");
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
                return ConnectionPolicy::One;
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
            values["interval"] = m_interval;
            values["signalValue"] = m_signalValue;
            values["running"] = m_running;
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject& p) override
        {
            NodeDelegateModel::load(p);

            const QJsonValue v = p["values"];
            if (!v.isObject()) {
                return;
            }

            const QJsonObject values = v.toObject();
            if (values.contains("interval")) {
                setInterval(values["interval"].toInt());
            }
            if (values.contains("signalValue")) {
                setSignalValue(values["signalValue"].toString());
            }
            if (values.contains("running")) {
                setRunning(values["running"].toBool());
            }
        }

    public slots:
        void onTick()
        {
            m_value = m_signalValue;
            Q_EMIT dataUpdated(0);
        }

        void setInterval(int v)
        {
            if (m_interval == v) {
                return;
            }
            m_interval = qMax(1, v);
            if (m_running) {
                restartTimer();
            }
            Q_EMIT intervalChanged(m_interval);
        }

        void setSignalValue(const QString& v)
        {
            if (m_signalValue == v) {
                return;
            }
            m_signalValue = v;
            Q_EMIT signalValueChanged(m_signalValue);
        }

        void setRunning(bool v)
        {
            if (m_running == v) {
                return;
            }
            m_running = v;
            if (m_running) {
                restartTimer();
            } else if (m_timer) {
                m_timer->stop();
            }
            Q_EMIT runningChanged(m_running);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrInterval = makeFullOscAddress("/interval");
            const QString addrSignal = makeFullOscAddress("/signalValue");
            const QString addrRun = makeFullOscAddress("/running");

            if (ev.address == addrInterval) {
                setInterval(ev.payload.toInt());
            } else if (ev.address == addrSignal) {
                setSignalValue(ev.payload.toString());
            } else if (ev.address == addrRun) {
                setRunning(ev.payload.toBool());
            }
        }

    signals:
        void intervalChanged(int v);
        void signalValueChanged(const QString& v);
        void runningChanged(bool v);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/interval"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/signalValue"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/running"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        int interval() const { return m_interval; }
        QString signalValue() const { return m_signalValue; }
        bool running() const { return m_running; }

        void restartTimer()
        {
            if (!m_timer) {
                return;
            }
            m_timer->stop();
            const int intervalMs = qMax(1, m_interval);
            onTick();
            m_timer->start(intervalMs);
        }

    private:
        MetronomeInterface* m_widget = nullptr;
        QTimer* m_timer = nullptr;
        int m_interval = 1000;
        QString m_signalValue = QStringLiteral("tick");
        QString m_value;
        bool m_running = false;
    };
}
