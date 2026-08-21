#pragma once

#include <QtCore/QObject>

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "DistributeInterface.hpp"
#include "ConditionMatch.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

#include <QHash>
#include <QJSEngine>
#include <QSignalBlocker>
#include <unordered_map>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
    class DistributeDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        DistributeDataModel()
        {
            InPortCount = 1;
            OutPortCount = 5;
            Caption = "Distribute";
            CaptionVisible = true;
            WidgetEmbeddable = false;
            PortEditable = true;
            Resizable = true;

            m_jsEngine = new QJSEngine(this);
            m_emptyOutput = std::make_shared<VariableData>(QVariant(false));
            connect(widget, &DistributeInterface::rulesChanged, this, &DistributeDataModel::onRulesChanged);
            // syncOutputPortCount();
        }

        ~DistributeDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            const auto it = m_portOutputs.find(portIndex);
            if (it != m_portOutputs.end() && it->second) {
                return it->second;
            }
            return m_emptyOutput;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)

            if (data == nullptr) {
                return;
            }

            m_inputData = std::dynamic_pointer_cast<VariableData>(data);
            if (!m_inputData) {
                return;
            }

            distributeInput(m_inputData);
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QStringLiteral("INPUT");
            case PortType::Out:
                return QString("OUTPUT %1").arg(portIndex);
            default:
                return {};
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["rules"] = widget->exportRulesArray();
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QSignalBlocker widgetBlocker(widget);

            if (p.contains("rules")) {
                widget->importRulesArray(p["rules"].toArray());
            }

            // syncOutputPortCount();
        }

        QWidget *embeddedWidget() override { return widget; }

    private slots:
        void onRulesChanged()
        {
            // syncOutputPortCount();
            if (m_inputData) {
                distributeInput(m_inputData);
            }
        }

    private:
        // void syncOutputPortCount()
        // {
        //     const int maxPort = widget->maxConfiguredOutputPort();
        //     const unsigned int required = static_cast<unsigned int>(maxPort + 1);
        //     if (required > OutPortCount) {
        //         OutPortCount = required;
        //     }
        // }

        void setupJsInput(const VariableData &input)
        {
            QJSValue jsInput = m_jsEngine->toScriptValue(input.asMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);
        }

        bool matchesCondition(const VariableData &input, const QString &conditionText)
        {
            const QString condition = conditionText.trimmed();
            if (condition.isEmpty()) {
                return false;
            }

            if (condition == QStringLiteral("*")) {
                return true;
            }

            setupJsInput(input);

            const QJSValue result = m_jsEngine->evaluate(condition);
            if (result.isError()) {
                return ConditionMatch::matches(input.value(), condition);
            }

            return result.toBool();
        }

        void distributeInput(const std::shared_ptr<VariableData> &input)
        {
            QList<int> matchedRows;
            for (int row = 0; row < widget->rowCount(); ++row) {
                if (matchesCondition(*input, widget->conditionAt(row))) {
                    matchedRows.append(row);
                }
            }

            if (matchedRows.isEmpty()) {
                return;
            }

            for (const int row : matchedRows) {
                bool ok = false;
                const int outPort = widget->outputPortAt(row).toInt(&ok);
                if (!ok || outPort < 0 || outPort >= static_cast<int>(OutPortCount)) {
                    continue;
                }

                auto output = std::make_shared<VariableData>(QVariant(true));
                output->insert(QStringLiteral("_pulse"), ++m_pulseCounter);
                output->insert(QStringLiteral("_distributedPort"), outPort);
                output->insert(QStringLiteral("_matchedRow"), row);

                m_portOutputs[static_cast<PortIndex>(outPort)] = output;
                Q_EMIT dataUpdated(static_cast<PortIndex>(outPort));
            }
        }

        DistributeInterface *widget = new DistributeInterface();
        std::shared_ptr<VariableData> m_emptyOutput;
        std::shared_ptr<VariableData> m_inputData;
        std::unordered_map<PortIndex, std::shared_ptr<VariableData>> m_portOutputs;
        QJSEngine *m_jsEngine = nullptr;
        quint64 m_pulseCounter = 0;
    };
}
