#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "ValueLookupInterface.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

#include <QSignalBlocker>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
    class ValueLookupDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        ValueLookupDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            Caption = "Lookup";
            CaptionVisible = true;
            WidgetEmbeddable = false;
            PortEditable = false;
            Resizable = true;

            m_jsEngine = new QJSEngine(this);

            connect(widget, &ValueLookupInterface::listChanged, this, [this]() {
                if (m_inputData) {
                    evaluateInput();
                }
            });
        }

        ~ValueLookupDataModel() override = default;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if (!m_outputData) {
                return std::make_shared<VariableData>();
            }
            return m_outputData;
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

            evaluateInput();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QString("INPUT %1").arg(portIndex);
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
        }

        QWidget *embeddedWidget() override { return widget; }

    private:
        void setupJsInput(const VariableData &input)
        {
            QJSValue jsInput = m_jsEngine->toScriptValue(input.asMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);
        }

        static QVariant parseOutputValue(const QString &text)
        {
            if (text.isEmpty()) {
                return {};
            }

            bool ok = false;
            const int intValue = text.toInt(&ok);
            if (ok) {
                return intValue;
            }

            const double doubleValue = text.toDouble(&ok);
            if (ok) {
                return doubleValue;
            }

            if (text.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
                return true;
            }
            if (text.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
                return false;
            }

            return text;
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
                qDebug() << "ValueLookup Condition JS表达式错误" << result.toString();
                return false;
            }

            return result.toBool();
        }

        QVariant evaluateValueExpression(const VariableData &input, const QString &valueText)
        {
            const QString expression = valueText.trimmed();
            if (expression.isEmpty()) {
                return {};
            }

            setupJsInput(input);

            const QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                return parseOutputValue(expression);
            }

            return result.toVariant();
        }

        static std::shared_ptr<VariableData> buildOutputFromValue(const QVariant &value)
        {
            if (!value.isValid()) {
                return nullptr;
            }

            if (value.metaType().id() == QMetaType::QVariantMap) {
                return std::make_shared<VariableData>(value.toMap());
            }

            return std::make_shared<VariableData>(value);
        }

        void evaluateInput()
        {
            if (!m_inputData) {
                return;
            }

            int matchedRow = -1;
            for (int row = 0; row < widget->rowCount(); ++row) {
                if (matchesCondition(*m_inputData, widget->conditionAt(row))) {
                    matchedRow = row;
                    break;
                }
            }

            if (matchedRow < 0) {
                m_outputData.reset();
                return;
            }

            const QString valueText = widget->outputValueAt(matchedRow);
            m_outputData = buildOutputFromValue(evaluateValueExpression(*m_inputData, valueText));
            if (!m_outputData) {
                return;
            }

            Q_EMIT dataUpdated(0);
        }

        ValueLookupInterface *widget = new ValueLookupInterface();
        std::shared_ptr<VariableData> m_outputData;
        std::shared_ptr<VariableData> m_inputData;
        QJSEngine *m_jsEngine = nullptr;
    };
}
