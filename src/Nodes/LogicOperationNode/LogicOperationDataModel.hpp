#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>

#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#if defined(UNTITLED_LIBRARY)
#  define UNTITLED_EXPORT Q_DECL_EXPORT
#else
#  define UNTITLED_EXPORT Q_DECL_IMPORT
#endif
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes

{
    enum class LogicMethod : int {
        And = 0,
        Or = 1,
        NotEqual = 2,
        Max = 3,
        Min = 4,
        Less = 5,
        LessEqual = 6,
        Greater = 7,
        GreaterEqual = 8
    };
    class LogicOperationBaseDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        explicit LogicOperationBaseDataModel(LogicMethod method, const QString& caption)
            : m_logicMethod(method)
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption = caption;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;

            val=QVariant(false);
        }

        virtual ~LogicOperationBaseDataModel() override{}

    public:
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            case PortType::None:
                break;
            default:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return std::make_shared<VariableData>(val);
        }
        QWidget* embeddedWidget() override {
            return nullptr;
        }
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            if (auto textData = std::dynamic_pointer_cast<VariableData>(data)) {
                in_dictionary[portIndex]=textData->value();
                methodChanged();

            }
        }
        void methodChanged()
        {
            bool tempVal=false;

                switch (m_logicMethod) {
                case LogicMethod::And:
                    tempVal=in_dictionary[0].toString()==in_dictionary[1].toString();
                    break;
                case LogicMethod::Or:
                    tempVal=in_dictionary[0].toBool()||in_dictionary[1].toBool();
                    break;
                case LogicMethod::NotEqual:
                    tempVal=in_dictionary[0].toString()!=in_dictionary[1].toString();
                    break;
                case LogicMethod::Max:
                    tempVal=qMax(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                    break;
                case LogicMethod::Min:
                    tempVal=qMin(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                    break;
                case LogicMethod::Less:
                    tempVal=in_dictionary[0].toFloat()<in_dictionary[1].toFloat();
                    break;
                case LogicMethod::LessEqual:
                    tempVal=in_dictionary[0].toFloat()<=in_dictionary[1].toFloat();
                    break;
                case LogicMethod::Greater:
                    tempVal=in_dictionary[0].toFloat()>in_dictionary[1].toFloat();
                    break;
                case LogicMethod::GreaterEqual:
                    tempVal=in_dictionary[0].toFloat()>=in_dictionary[1].toFloat();
                    break;
                }
            // qDebug()<<"tempVal:"<<tempVal;
            val = tempVal;
            Q_EMIT dataUpdated(0);
        }

        // QWidget *embeddedWidget() override { return widget; }
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }

            return result;
        }
    private:

        std::unordered_map<unsigned int, QVariant> in_dictionary;
        QVariant val;
        LogicMethod m_logicMethod = LogicMethod::And;
    };

    class LogicAndDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicAndDataModel() : LogicOperationBaseDataModel(LogicMethod::And, "Logic And") {}
    };

    class LogicOrDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicOrDataModel() : LogicOperationBaseDataModel(LogicMethod::Or, "Logic Or") {}
    };

    class LogicNotEqualDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicNotEqualDataModel() : LogicOperationBaseDataModel(LogicMethod::NotEqual, "Logic NotEqual") {}
    };

    class LogicMaxDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicMaxDataModel() : LogicOperationBaseDataModel(LogicMethod::Max, "Logic Max") {}
    };

    class LogicMinDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicMinDataModel() : LogicOperationBaseDataModel(LogicMethod::Min, "Logic Min") {}
    };

    class LogicLessDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicLessDataModel() : LogicOperationBaseDataModel(LogicMethod::Less, "Logic Less") {}
    };

    class LogicLessEqualDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicLessEqualDataModel() : LogicOperationBaseDataModel(LogicMethod::LessEqual, "Logic LessEqual") {}
    };

    class LogicGreaterDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicGreaterDataModel() : LogicOperationBaseDataModel(LogicMethod::Greater, "Logic Greater") {}
    };

    class LogicGreaterEqualDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicGreaterEqualDataModel() : LogicOperationBaseDataModel(LogicMethod::GreaterEqual, "Logic GreaterEqual") {}
    };
}
