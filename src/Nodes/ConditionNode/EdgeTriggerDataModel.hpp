#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QSignalBlocker>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "EdgeTriggerInterface.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace QtNodes;
using namespace NodeDataTypes;

namespace Nodes
{
    class EdgeTriggerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        enum class EdgeMode : int {
            Rising = 0,
            Falling = 1
        };
        Q_ENUM(EdgeMode)

        enum OutputPort : PortIndex {
            PulsePort = 0,
            DataPort = 1
        };

        Q_PROPERTY(EdgeMode edgeMode READ edgeMode WRITE setEdgeMode NOTIFY edgeModeChanged)

    public:
        EdgeTriggerDataModel()
        {
            InPortCount = 1;
            OutPortCount = 2;
            Caption = QStringLiteral("Edge Trigger");
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;

            m_pulseOutput = std::make_shared<VariableData>(QVariant(false));
            m_edgeMode = EdgeMode::Rising;

            connect(widget->expression, &QLineEdit::editingFinished, this, &EdgeTriggerDataModel::onParametersChanged);
            connect(widget->edgeMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int index) { setEdgeMode(static_cast<EdgeMode>(index)); });

            // {
            //     NodeDelegateModel::ExternalBinding b;
            //     b.member = "edgeMode";
            //     b.control = widget->edgeMode;
            //     AbstractDelegateModel::registerExternalBinding("/edgeMode", this, b);
            // }

            m_jsEngine = new QJSEngine(this);
        }

        ~EdgeTriggerDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QStringLiteral("INPUT");
            case PortType::Out:
                switch (portIndex) {
                case PulsePort:
                    return QStringLiteral("PULSE");
                case DataPort:
                    return QStringLiteral("DATA");
                default:
                    return QString();
                }
            default:
                return QString();
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex) {
            case PulsePort:
                return m_pulseOutput ? m_pulseOutput : std::make_shared<VariableData>(QVariant(false));
            case DataPort:
                return m_dataOutput ? m_dataOutput : std::make_shared<VariableData>();
            default:
                return std::make_shared<VariableData>();
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            evaluateAndEmit();
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values["expression"] = widget->expression->text();
            values["edgeMode"] = static_cast<int>(m_edgeMode);

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                widget->expression->setText(v["expression"].toString());
                setEdgeMode(static_cast<EdgeMode>(v["edgeMode"].toInt(0)));
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        EdgeMode edgeMode() const { return m_edgeMode; }

    public slots:
        void setEdgeMode(EdgeMode mode)
        {
            if (m_edgeMode == mode) {
                return;
            }
            m_edgeMode = mode;
            {
                const QSignalBlocker blocker(widget->edgeMode);
                widget->edgeMode->setCurrentIndex(static_cast<int>(mode));
            }
            m_hasPrevious = false;
            Q_EMIT edgeModeChanged(mode);
            onParametersChanged();
        }

    signals:
        void edgeModeChanged(EdgeMode mode);

    private slots:
        void onParametersChanged()
        {
            if (m_inData) {
                evaluateAndEmit();
            }
        }

    private:
        bool evaluateCurrentBool()
        {
            if (!m_inData) {
                return false;
            }

            const QString expression = widget->expression->text().trimmed();
            if (expression.isEmpty()) {
                return m_inData->value().toBool();
            }

            QJSValue jsInput = m_jsEngine->toScriptValue(m_inData->getMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);

            const QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                qDebug() << "EdgeTrigger JS表达式错误:" << result.toString();
                return false;
            }

            return result.toBool();
        }

        bool detectEdge(bool current) const
        {
            if (!m_hasPrevious) {
                return false;
            }

            switch (m_edgeMode) {
            case EdgeMode::Rising:
                return !m_previousBool && current;
            case EdgeMode::Falling:
                return m_previousBool && !current;
            }
            return false;
        }

        void evaluateAndEmit()
        {
            const bool current = evaluateCurrentBool();

            if (detectEdge(current)) {
                m_pulseOutput = std::make_shared<VariableData>(QVariant(true));
                Q_EMIT dataUpdated(PulsePort);

                QVariantMap outputMap = m_inData->getMap();
                outputMap.insert(QStringLiteral("default"), true);
                m_dataOutput = std::make_shared<VariableData>(outputMap);
                Q_EMIT dataUpdated(DataPort);
            }

            m_previousBool = current;
            m_hasPrevious = true;
        }

    private:
        EdgeTriggerInterface *widget = new EdgeTriggerInterface();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_pulseOutput;
        std::shared_ptr<VariableData> m_dataOutput;
        QJSEngine *m_jsEngine = nullptr;

        bool m_previousBool = false;
        bool m_hasPrevious = false;
        EdgeMode m_edgeMode = EdgeMode::Rising;
    };
}
