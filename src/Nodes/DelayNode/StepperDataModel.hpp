#pragma once

#include <algorithm>
#include <memory>

#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "StatusContainer/GlobalEventBus.hpp"
#include "StepperInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::InvalidPortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Stepper：输出口可编辑，前 N-1 个按索引逐步触发，最后一个为 STEP（最近触发的输出端口 index，复位为 -1）。
     * TRIGGER 执行下一步；RESET（端口或界面按钮）回到 -1。节点尺寸固定。
     * 外部控制：/step（当前端口 index）、/reset（复位脉冲）。
     *
     * 动作口不缓存粘性 true：仅本次触发的端口携带脉冲，避免粘贴/重建连线时多口同时触发。
     */
    class StepperDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int step READ step WRITE setStep NOTIFY stepChanged)
        Q_PROPERTY(bool reset READ reset WRITE setReset NOTIFY resetChanged)

    public:
        StepperDataModel()
        {
            InPortCount = 2;
            OutPortCount = 3; // OUTPUT0, OUTPUT1, STEP
            CaptionVisible = true;
            Caption = QStringLiteral("Stepper");
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;

            m_stepData = std::make_shared<VariableData>(-1);

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "step";
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/step"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "reset";
                b.control = widget->resetButton();
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/reset"), this, b);
            }

            connect(widget, &StepperInterface::resetClicked, this, [this]() { setReset(true); });
            refreshUi();
        }

        ~StepperDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0:
                    return QStringLiteral("TRIGGER");
                case 1:
                    return QStringLiteral("RESET");
                default:
                    return QStringLiteral("IN %1").arg(portIndex);
                }
            case PortType::Out:
                if (isStepPort(portIndex)) {
                    return QStringLiteral("STEP");
                }
                return QStringLiteral("OUTPUT %1").arg(portIndex);
            default:
                return {};
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (isStepPort(port)) {
                return m_stepData;
            }
            // 仅“刚触发”的那一口返回脉冲；其余口返回 false，避免粘贴连线时全员触发
            if (port == m_lastFiredPort && m_pulse) {
                return m_pulse;
            }
            return std::make_shared<VariableData>(false);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) {
                return;
            }
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }

            switch (portIndex) {
            case 0: // TRIGGER：仅布尔 true 时步进
                if (!var->asBool()) {
                    return;
                }
                advanceStep();
                break;
            case 1: // RESET：仅布尔 true 时复位
                if (!var->asBool()) {
                    return;
                }
                setReset(true);
                break;
            default:
                break;
            }
        }

        QJsonObject save() const override
        {
            return NodeDelegateModel::save();
        }

        void load(const QJsonObject &p) override
        {
            NodeDelegateModel::load(p);
            clearPulse();
            setStep(-1);
        }

        QWidget *embeddedWidget() override { return widget; }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index)
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            default:
                return ConnectionPolicy::One;
            }
        }

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress(QStringLiteral("/step")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/reset")), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private slots:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("step")) {
                setStep(ev.payload.toInt());
            } else if (localPath == QLatin1String("reset")) {
                if (ev.payload.toBool()) {
                    setReset(true);
                }
            }
        }

    private:
        int stepCount() const
        {
            return std::max(0, static_cast<int>(OutPortCount) - 1);
        }

        PortIndex stepPortIndex() const
        {
            return OutPortCount > 0 ? static_cast<PortIndex>(OutPortCount - 1) : 0;
        }

        bool isStepPort(PortIndex portIndex) const
        {
            return OutPortCount > 0 && portIndex == stepPortIndex();
        }

        int normalizeStep(int value) const
        {
            if (value < 0) {
                return -1;
            }
            const int n = stepCount();
            if (n <= 0) {
                return -1;
            }
            return value % n;
        }

        int step() const { return m_cursor; }

        void setStep(int value)
        {
            const int normalized = normalizeStep(value);
            if (m_cursor == normalized) {
                refreshUi();
                return;
            }
            m_cursor = normalized;
            m_stepData = std::make_shared<VariableData>(m_cursor);
            refreshUi();
            Q_EMIT stepChanged(m_cursor);
            if (OutPortCount > 0) {
                Q_EMIT dataUpdated(stepPortIndex());
            }
        }

        bool reset() const { return m_reset; }

        void setReset(bool value)
        {
            if (!value || m_reset) {
                return;
            }
            m_reset = true;
            Q_EMIT resetChanged(true);
            clearPulse();
            setStep(-1);
            m_reset = false;
            Q_EMIT resetChanged(false);
        }

        void refreshUi()
        {
            if (widget) {
                widget->setStepDisplay(m_cursor);
            }
        }

        void clearPulse()
        {
            m_lastFiredPort = InvalidPortIndex;
            m_pulse.reset();
        }

        void advanceStep()
        {
            const int n = stepCount();
            if (n <= 0) {
                clearPulse();
                setStep(-1);
                return;
            }

            const int actionPort = (m_cursor < 0) ? 0 : ((m_cursor + 1) % n);
            m_lastFiredPort = static_cast<PortIndex>(actionPort);
            m_pulse = std::make_shared<VariableData>(true);
            m_pulse->insert(QStringLiteral("_pulse"), ++m_pulseCounter);

            setStep(actionPort);
            Q_EMIT dataUpdated(m_lastFiredPort);
        }

        StepperInterface *widget = new StepperInterface();
        std::shared_ptr<VariableData> m_stepData;
        std::shared_ptr<VariableData> m_pulse;
        PortIndex m_lastFiredPort = InvalidPortIndex;
        quint64 m_pulseCounter = 0;
        int m_cursor = -1;
        bool m_reset = false;

    signals:
        void stepChanged(int value);
        void resetChanged(bool value);
    };
}
