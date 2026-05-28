#pragma once

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>

#include <QtCore/qglobal.h>

#include "Common/BaseClass/AbstractDelegateModel.h"
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
    enum class MathMethod : int {
        Add = 0,
        Sub = 1,
        Mul = 2,
        Div = 3,
        Mod = 4,
        Pow = 5,
    };
    class MathOperationBaseDataModel : public AbstractDelegateModel
    {
        Q_OBJECT



    public:
        explicit MathOperationBaseDataModel(MathMethod method, const QString& caption)
            : m_mathMethod(method)
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption = caption;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;

            val=QVariant(0.0);
        }

        virtual ~MathOperationBaseDataModel() override{}

    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                return "INPUT "+QString::number(portIndex);
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return std::make_shared<VariableData>(val);
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
        QWidget* embeddedWidget() override {
            return nullptr;
        }
        void methodChanged()
        {
            switch (m_mathMethod) {
            case MathMethod::Add:
                val=in_dictionary[0].toDouble()+in_dictionary[1].toDouble();
                break;
            case MathMethod::Sub:
                val=in_dictionary[0].toDouble()-in_dictionary[1].toDouble();
                break;
            case MathMethod::Mul:
                val=in_dictionary[0].toDouble()*in_dictionary[1].toDouble();
                break;
            case MathMethod::Div:
                if (in_dictionary[1].toDouble()==0){
                    val=0;
                    break;
                }
                val=in_dictionary[0].toDouble()/in_dictionary[1].toDouble();
                break;
            case MathMethod::Mod:
                val=std::fmod(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                break;
            case MathMethod::Pow:
                val=std::pow(in_dictionary[0].toDouble(),in_dictionary[1].toDouble());
                break;
            }

            Q_EMIT dataUpdated(0);
        }

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
        MathMethod m_mathMethod = MathMethod::Add;
    };

    class MathAddDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathAddDataModel() : MathOperationBaseDataModel(MathMethod::Add, "Math Add") {}
    };

    class MathSubDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathSubDataModel() : MathOperationBaseDataModel(MathMethod::Sub, "Math Sub") {}
    };

    class MathMulDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathMulDataModel() : MathOperationBaseDataModel(MathMethod::Mul, "Math Mul") {}
    };

    class MathDivDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathDivDataModel() : MathOperationBaseDataModel(MathMethod::Div, "Math Div") {}
    };

    class MathModDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathModDataModel() : MathOperationBaseDataModel(MathMethod::Mod, "Math Mod") {}
    };

    class MathPowDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathPowDataModel() : MathOperationBaseDataModel(MathMethod::Pow, "Math Pow") {}
    };
}
