#pragma once

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
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
    enum class LogicMethod : int {
        And = 0,
        Or = 1,
        NotEqual = 2,
        Less = 3,
        LessEqual = 4,
        Greater = 5,
        GreaterEqual = 6,
        Equal = 7,
        Xor = 8,
        Nand = 9,
        Nor = 10,
        Not = 11,
        IsEmpty = 12,
        EqualNum = 13,
    };

    enum class LogicInputMode {
        Unary,
        Binary,
    };

    struct LogicPortConfig {
        LogicInputMode inputMode = LogicInputMode::Binary;
        unsigned int defaultInPortCount = 2;
    };

    inline LogicPortConfig portConfigFor(LogicMethod method)
    {
        switch (method) {
        case LogicMethod::Not:
        case LogicMethod::IsEmpty:
            return {LogicInputMode::Unary, 1};
        default:
            return {LogicInputMode::Binary, 2};
        }
    }

    class LogicOperationBaseDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        explicit LogicOperationBaseDataModel(LogicMethod method, const QString& caption)
            : m_logicMethod(method)
            , m_portConfig(portConfigFor(method))
        {
            InPortCount = m_portConfig.defaultInPortCount;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = caption;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            val = QVariant(false);
        }

        virtual ~LogicOperationBaseDataModel() override {}

    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                if (m_portConfig.inputMode == LogicInputMode::Unary) {
                    return QStringLiteral("INPUT");
                }
                return QStringLiteral("INPUT ") + QString::number(portIndex);
            case PortType::Out:
                return QStringLiteral("OUTPUT ") + QString::number(portIndex);
            default:
                return {};
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

        QWidget* embeddedWidget() override
        {
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                in_dictionary.erase(portIndex);
                methodChanged();
                return;
            }

            if (auto textData = std::dynamic_pointer_cast<VariableData>(data)) {
                in_dictionary[portIndex] = textData->value();
                methodChanged();
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index);
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            case PortType::None:
            default:
                break;
            }
            return ConnectionPolicy::One;
        }

    private:
        std::optional<QVariant> inputValue(PortIndex portIndex) const
        {
            const auto it = in_dictionary.find(portIndex);
            if (it == in_dictionary.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        static bool toBool(const QVariant& value)
        {
            return value.toBool();
        }

        static bool isEmptyValue(const QVariant& value)
        {
            if (!value.isValid() || value.isNull()) {
                return true;
            }
            if (value.typeId() == QMetaType::QString) {
                return value.toString().isEmpty();
            }
            if (value.canConvert<double>()) {
                return false;
            }
            return !value.isValid();
        }

        void methodChanged()
        {
            bool tempVal = false;

            const auto a = inputValue(0).value_or(QVariant());
            const auto b = inputValue(1).value_or(QVariant());

            switch (m_logicMethod) {
            case LogicMethod::And:
                tempVal = toBool(a) && toBool(b);
                break;
            case LogicMethod::Or:
                tempVal = toBool(a) || toBool(b);
                break;
            case LogicMethod::Xor:
                tempVal = toBool(a) ^ toBool(b);
                break;
            case LogicMethod::Nand:
                tempVal = !(toBool(a) && toBool(b));
                break;
            case LogicMethod::Nor:
                tempVal = !(toBool(a) || toBool(b));
                break;
            case LogicMethod::Not:
                tempVal = !toBool(a);
                break;
            case LogicMethod::Equal:
                tempVal = a.toString() == b.toString();
                break;
            case LogicMethod::NotEqual:
                tempVal = a.toString() != b.toString();
                break;
            case LogicMethod::EqualNum:
                tempVal = qFuzzyCompare(a.toDouble(), b.toDouble())
                          || (a.toDouble() == 0.0 && b.toDouble() == 0.0);
                break;
            case LogicMethod::Less:
                tempVal = a.toFloat() < b.toFloat();
                break;
            case LogicMethod::LessEqual:
                tempVal = a.toFloat() <= b.toFloat();
                break;
            case LogicMethod::Greater:
                tempVal = a.toFloat() > b.toFloat();
                break;
            case LogicMethod::GreaterEqual:
                tempVal = a.toFloat() >= b.toFloat();
                break;
            case LogicMethod::IsEmpty:
                tempVal = isEmptyValue(a);
                break;
            }

            val = tempVal;
            Q_EMIT dataUpdated(0);
        }

    private:
        std::unordered_map<unsigned int, QVariant> in_dictionary;
        QVariant val;
        LogicMethod m_logicMethod = LogicMethod::And;
        LogicPortConfig m_portConfig;
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

    class LogicXorDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicXorDataModel() : LogicOperationBaseDataModel(LogicMethod::Xor, "Logic Xor") {}
    };

    class LogicNandDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicNandDataModel() : LogicOperationBaseDataModel(LogicMethod::Nand, "Logic Nand") {}
    };

    class LogicNorDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicNorDataModel() : LogicOperationBaseDataModel(LogicMethod::Nor, "Logic Nor") {}
    };

    class LogicNotDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicNotDataModel() : LogicOperationBaseDataModel(LogicMethod::Not, "Logic Not") {}
    };

    class LogicEqualDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicEqualDataModel() : LogicOperationBaseDataModel(LogicMethod::Equal, "Logic Equal") {}
    };

    class LogicNotEqualDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicNotEqualDataModel() : LogicOperationBaseDataModel(LogicMethod::NotEqual, "Logic NotEqual") {}
    };

    class LogicEqualNumDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicEqualNumDataModel() : LogicOperationBaseDataModel(LogicMethod::EqualNum, "Logic EqualNum") {}
    };

    class LogicIsEmptyDataModel final : public LogicOperationBaseDataModel
    {
    public:
        LogicIsEmptyDataModel() : LogicOperationBaseDataModel(LogicMethod::IsEmpty, "Logic IsEmpty") {}
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
