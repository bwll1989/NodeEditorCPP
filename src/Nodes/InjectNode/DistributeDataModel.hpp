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
            OutPortCount = 4;
            Caption = "Distribute";
            CaptionVisible = true;
            WidgetEmbeddable = false;
            PortEditable = false;
            Resizable = true;

            m_jsEngine = new QJSEngine(this);
            m_emptyOutput = std::make_shared<VariableData>(QVariant(false));
            widget->setRowCount(static_cast<int>(OutPortCount));

            connect(widget, &DistributeInterface::rulesChanged, this, &DistributeDataModel::onRulesChanged);
            connect(widget, &DistributeInterface::rowAppended, this, &DistributeDataModel::onRowAppended);
            connect(widget, &DistributeInterface::rowRemoved, this, &DistributeDataModel::onRowRemoved);
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

            syncOutputPortCount(static_cast<unsigned int>(qMax(1, widget->rowCount())), false);
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        QWidget *embeddedWidget() override { return widget; }

    private slots:
        void onRulesChanged()
        {
            if (m_inputData) {
                distributeInput(m_inputData);
            }
        }

        void onRowAppended()
        {
            const unsigned int oldCount = OutPortCount;
            Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, oldCount);
            OutPortCount = oldCount + 1;
            Q_EMIT portsInserted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        void onRowRemoved(int index)
        {
            if (index < 0 || OutPortCount <= 1) {
                return;
            }
            // 与 Inject 一致：删除对应索引端口，其后端口与连线向上补位
            const auto portIndex = static_cast<PortIndex>(index);
            Q_EMIT portsAboutToBeDeleted(PortType::Out, portIndex, portIndex);
            OutPortCount -= 1;
            Q_EMIT portsDeleted();
            Q_EMIT embeddedWidgetSizeUpdated();

            // 端口索引变化后清掉旧缓存，按需用当前输入重算
            m_portOutputs.clear();
            if (m_inputData) {
                distributeInput(m_inputData);
            }
        }

    private:
        /** 仅用于 load：无连线迁移时直接对齐端口数 */
        void syncOutputPortCount(unsigned int newCount, bool notifyPorts)
        {
            const unsigned int oldCount = OutPortCount;
            if (newCount == oldCount || newCount < 1) {
                return;
            }

            if (notifyPorts) {
                if (newCount > oldCount) {
                    Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, newCount - 1);
                    OutPortCount = newCount;
                    Q_EMIT portsInserted();
                } else {
                    Q_EMIT portsAboutToBeDeleted(PortType::Out, newCount, oldCount - 1);
                    OutPortCount = newCount;
                    Q_EMIT portsDeleted();
                }
                Q_EMIT embeddedWidgetSizeUpdated();
            } else {
                OutPortCount = newCount;
            }
        }

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

        /** 按当前界面行号分发输入，一行对应一个输出端口。 */
        void distributeInput(const std::shared_ptr<VariableData> &input)
        {
            m_portOutputs.clear();

            for (int row = 0; row < widget->rowCount(); ++row) {
                if (!matchesCondition(*input, widget->conditionAt(row))) {
                    continue;
                }

                const auto outPort = static_cast<PortIndex>(row);
                auto output = std::make_shared<VariableData>(QVariant(true));
                output->insert(QStringLiteral("_pulse"), ++m_pulseCounter);
                output->insert(QStringLiteral("_distributedPort"), row);
                output->insert(QStringLiteral("_matchedRow"), row);

                m_portOutputs[outPort] = output;
                Q_EMIT dataUpdated(outPort);
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
