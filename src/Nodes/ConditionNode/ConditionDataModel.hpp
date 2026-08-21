#pragma once

#include <QtCore/QObject>
#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "ConditionInterface.hpp"

#include <QtWidgets/QLineEdit>
#include <QtCore/qglobal.h>
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
    class ConditionDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        enum OutputPort : PortIndex {
            ConditionPort = 0,
            DataPort = 1
        };

    public:
        ConditionDataModel()
        {
            InPortCount = 1;
            OutPortCount = 2;
            Caption = QStringLiteral("Condition");
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;

            m_boolOutput = std::make_shared<VariableData>(QVariant(false));

            connect(widget->Editor, &QLineEdit::editingFinished, this, &ConditionDataModel::onExpressionEdited);
            m_jsEngine = new QJSEngine(this);
        }

        ~ConditionDataModel() override = default;

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
                case ConditionPort:
                    return QStringLiteral("CONDITION");
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
            case ConditionPort:
                return m_boolOutput ? m_boolOutput : std::make_shared<VariableData>(QVariant(false));
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
            values["expression"] = widget->Editor->text();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                widget->Editor->setText(v["expression"].toString());
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    private slots:
        void onExpressionEdited()
        {
            evaluateAndEmit();
        }

    private:
        void evaluateAndEmit()
        {
            if (!m_inData) {
                m_boolOutput = std::make_shared<VariableData>(QVariant(false));
                Q_EMIT dataUpdated(ConditionPort);
                return;
            }

            const QString expression = widget->Editor->text();

            QJSValue jsInput = m_jsEngine->toScriptValue(m_inData->asMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);

            const QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                qDebug() << "Condition JS表达式错误" << result.toString();
                m_boolOutput = std::make_shared<VariableData>(QVariant(false));
                Q_EMIT dataUpdated(ConditionPort);
                return;
            }

            const bool expressionResult = result.toBool();
            m_boolOutput = std::make_shared<VariableData>(expressionResult);
            Q_EMIT dataUpdated(ConditionPort);

            if (expressionResult) {
                QVariantMap outputMap = m_inData->asMap();
                outputMap.insert(QStringLiteral("default"), true);
                m_dataOutput = std::make_shared<VariableData>(outputMap);
                Q_EMIT dataUpdated(DataPort);
            }
        }

    private:
        ConditionInterface *widget = new ConditionInterface();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_boolOutput;
        std::shared_ptr<VariableData> m_dataOutput;
        QJSEngine *m_jsEngine = nullptr;
    };
}
