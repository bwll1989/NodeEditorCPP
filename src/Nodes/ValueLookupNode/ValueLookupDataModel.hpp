#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "ValueLookupInterface.hpp"
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
            Caption = "Value Lookup";
            CaptionVisible = true;
            WidgetEmbeddable = false;
            PortEditable = false;
            Resizable = true;

            m_outputData = std::make_shared<VariableData>();
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
            return m_outputData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)

            if (data == nullptr) {
                return;
            }

            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (!varData) {
                return;
            }

            evaluateInput(varData->value());
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

    private slots:
        void outDataSlot()
        {
            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(i);
            }
        }

    private:
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

        void evaluateInput(const QVariant &input)
        {
            const int row = widget->findMatchingRow(input);
            if (row < 0) {
                m_outputData = std::make_shared<VariableData>(QString());
                outDataSlot();
                return;
            }

            const QString valueText = widget->outputValueAt(row);
            m_outputData = std::make_shared<VariableData>(parseOutputValue(valueText));
            m_outputData->insert(QStringLiteral("_matchedRow"), row);
            m_outputData->insert(QStringLiteral("_pulse"), ++m_pulseCounter);
            outDataSlot();
        }

        ValueLookupInterface *widget = new ValueLookupInterface();
        std::shared_ptr<VariableData> m_outputData;
        quint64 m_pulseCounter = 0;
    };
}
