#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "InjectInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

#include <QSignalBlocker>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{
    class InjectDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        InjectDataModel()
        {
            InPortCount = 4;
            OutPortCount = 1;
            Caption = "Inject";
            CaptionVisible = true;
            WidgetEmbeddable = false;
            PortEditable = false;
            Resizable = true;

            m_outputData = std::make_shared<VariableData>();
            widget->setRowCount(static_cast<int>(InPortCount));

            connect(widget, &InjectInterface::rowAppended, this, &InjectDataModel::onRowAppended);
            connect(widget, &InjectInterface::rowRemoved, this, &InjectDataModel::onRowRemoved);
        }

        ~InjectDataModel() override = default;

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
            if (data == nullptr) {
                return;
            }

            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (!varData) {
                return;
            }

            if (!varData->asBool()) {
                return;
            }

            injectAt(portIndex);
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return QString("TRIGGER %1").arg(portIndex);
            case PortType::Out:
                return QString("OUTPUT %1").arg(portIndex);
            default:
                return {};
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["list"] = widget->exportValuesArray();
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QSignalBlocker widgetBlocker(widget);

            if (p.contains("list")) {
                widget->importValuesArray(p["list"].toArray());
            }
            widget->setRowCount(static_cast<int>(InPortCount));
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        QWidget *embeddedWidget() override { return widget; }

    private slots:
        void outDataSlot()
        {
            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(i);
            }
        }

        void onRowAppended()
        {
            const unsigned int oldCount = InPortCount;
            Q_EMIT portsAboutToBeInserted(PortType::In, oldCount, oldCount);
            InPortCount = oldCount + 1;
            Q_EMIT portsInserted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        void onRowRemoved(int index)
        {
            if (index < 0 || InPortCount <= 1) {
                return;
            }
            const auto portIndex = static_cast<PortIndex>(index);
            Q_EMIT portsAboutToBeDeleted(PortType::In, portIndex, portIndex);
            InPortCount -= 1;
            Q_EMIT portsDeleted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

    private:
        static QVariant parseListValue(const QString &text)
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

        void injectAt(PortIndex portIndex)
        {
            const QString valueText = widget->valueAt(static_cast<int>(portIndex));
            m_outputData = std::make_shared<VariableData>(parseListValue(valueText));
            m_outputData->insert(QStringLiteral("_pulse"), ++m_pulseCounter);
            outDataSlot();
        }

        InjectInterface *widget = new InjectInterface();
        std::shared_ptr<VariableData> m_outputData;
        quint64 m_pulseCounter = 0;
    };
}
