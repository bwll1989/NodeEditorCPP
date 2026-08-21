#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace QtNodes;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Feedback 节点：保留上一次输入，分别输出当前值与上一帧值�?
     */
    class FeedbackDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        enum OutputPort : PortIndex {
            CurrentPort = 0,
            PreviousPort = 1
        };

    public:
        FeedbackDataModel()
        {
            InPortCount = 1;
            OutPortCount = 2;
            Caption = QStringLiteral("Feedback");
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
        }

        ~FeedbackDataModel() override = default;

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
                return QStringLiteral("INPUT");
            case PortType::Out:
                switch (portIndex) {
                case CurrentPort:
                    return QStringLiteral("CURRENT");
                case PreviousPort:
                    return QStringLiteral("PREVIOUS");
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
            case CurrentPort:
                return m_currentData ? m_currentData : std::make_shared<VariableData>();
            case PreviousPort:
                return m_previousData ? m_previousData : std::make_shared<VariableData>();
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

            auto incoming = std::dynamic_pointer_cast<VariableData>(data);
            if (!incoming) {
                return;
            }

            m_previousData = m_currentData;
            m_currentData = std::make_shared<VariableData>(incoming->asMap());

            Q_EMIT dataUpdated(CurrentPort);
            Q_EMIT dataUpdated(PreviousPort);
        }

        QWidget *embeddedWidget() override
        {
            return nullptr;
        }

    private:
        std::shared_ptr<VariableData> m_currentData;
        std::shared_ptr<VariableData> m_previousData;
    };
}
