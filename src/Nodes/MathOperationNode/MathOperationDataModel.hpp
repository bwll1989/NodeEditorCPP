#pragma once

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
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
    enum class MathMethod : int {
        Add = 0,
        Sub = 1,
        Mul = 2,
        Div = 3,
        Mod = 4,
        Pow = 5,
        Max = 6,
        Min = 7,
        Abs = 8,
        Floor = 9,
        Ceil = 10,
        Round = 11,
        Sqrt = 12,
        Log = 13,
        Avg = 14,
        Hypot = 15,
    };

    enum class MathInputMode {
        Unary,
        Binary,
        Variadic,
    };

    struct MathPortConfig {
        MathInputMode inputMode = MathInputMode::Binary;
        unsigned int defaultInPortCount = 2;
        bool portEditable = false;
    };

    inline MathPortConfig portConfigFor(MathMethod method)
    {
        switch (method) {
        case MathMethod::Abs:
        case MathMethod::Floor:
        case MathMethod::Ceil:
        case MathMethod::Round:
        case MathMethod::Sqrt:
        case MathMethod::Log:
            return {MathInputMode::Unary, 1, false};
        case MathMethod::Add:
        case MathMethod::Mul:
        case MathMethod::Max:
        case MathMethod::Min:
        case MathMethod::Avg:
            return {MathInputMode::Variadic, 2, true};
        case MathMethod::Sub:
        case MathMethod::Div:
        case MathMethod::Mod:
        case MathMethod::Pow:
        case MathMethod::Hypot:
        default:
            return {MathInputMode::Binary, 2, false};
        }
    }

    class MathOperationBaseDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        explicit MathOperationBaseDataModel(MathMethod method, const QString& caption)
            : m_mathMethod(method)
            , m_portConfig(portConfigFor(method))
        {
            InPortCount = m_portConfig.defaultInPortCount;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = caption;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = m_portConfig.portEditable;

            val = QVariant(0.0);
        }

        virtual ~MathOperationBaseDataModel() override {}

    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                if (m_portConfig.inputMode == MathInputMode::Unary) {
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

        QWidget* embeddedWidget() override
        {
            return nullptr;
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
        std::vector<double> collectInputValues() const
        {
            std::vector<double> values;
            values.reserve(InPortCount);
            for (unsigned int i = 0; i < InPortCount; ++i) {
                const auto it = in_dictionary.find(i);
                if (it != in_dictionary.end()) {
                    values.push_back(it->second.toDouble());
                }
            }
            return values;
        }

        std::optional<double> inputValue(PortIndex portIndex) const
        {
            const auto it = in_dictionary.find(portIndex);
            if (it == in_dictionary.end()) {
                return std::nullopt;
            }
            return it->second.toDouble();
        }

        void methodChanged()
        {
            switch (m_mathMethod) {
            case MathMethod::Add: {
                double sum = 0.0;
                for (const double value : collectInputValues()) {
                    sum += value;
                }
                val = sum;
                break;
            }
            case MathMethod::Sub: {
                const auto a = inputValue(0).value_or(0.0);
                const auto b = inputValue(1).value_or(0.0);
                val = a - b;
                break;
            }
            case MathMethod::Mul: {
                const auto values = collectInputValues();
                if (values.empty()) {
                    val = 0.0;
                    break;
                }
                double product = 1.0;
                for (const double value : values) {
                    product *= value;
                }
                val = product;
                break;
            }
            case MathMethod::Div: {
                const auto a = inputValue(0).value_or(0.0);
                const auto b = inputValue(1).value_or(0.0);
                val = (b == 0.0) ? 0.0 : a / b;
                break;
            }
            case MathMethod::Mod: {
                const auto a = inputValue(0).value_or(0.0);
                const auto b = inputValue(1).value_or(0.0);
                val = std::fmod(a, b);
                break;
            }
            case MathMethod::Pow: {
                const auto a = inputValue(0).value_or(0.0);
                const auto b = inputValue(1).value_or(0.0);
                val = std::pow(a, b);
                break;
            }
            case MathMethod::Max: {
                const auto values = collectInputValues();
                val = values.empty() ? 0.0 : *std::max_element(values.begin(), values.end());
                break;
            }
            case MathMethod::Min: {
                const auto values = collectInputValues();
                val = values.empty() ? 0.0 : *std::min_element(values.begin(), values.end());
                break;
            }
            case MathMethod::Abs: {
                const auto a = inputValue(0).value_or(0.0);
                val = std::abs(a);
                break;
            }
            case MathMethod::Floor: {
                const auto a = inputValue(0).value_or(0.0);
                val = std::floor(a);
                break;
            }
            case MathMethod::Ceil: {
                const auto a = inputValue(0).value_or(0.0);
                val = std::ceil(a);
                break;
            }
            case MathMethod::Round: {
                const auto a = inputValue(0).value_or(0.0);
                val = std::round(a);
                break;
            }
            case MathMethod::Sqrt: {
                const auto a = inputValue(0).value_or(0.0);
                val = (a < 0.0) ? 0.0 : std::sqrt(a);
                break;
            }
            case MathMethod::Log: {
                const auto a = inputValue(0).value_or(0.0);
                val = (a <= 0.0) ? 0.0 : std::log(a);
                break;
            }
            case MathMethod::Avg: {
                const auto values = collectInputValues();
                if (values.empty()) {
                    val = 0.0;
                    break;
                }
                double sum = 0.0;
                for (const double value : values) {
                    sum += value;
                }
                val = sum / static_cast<double>(values.size());
                break;
            }
            case MathMethod::Hypot: {
                const auto a = inputValue(0).value_or(0.0);
                const auto b = inputValue(1).value_or(0.0);
                val = std::hypot(a, b);
                break;
            }
            }

            Q_EMIT dataUpdated(0);
        }

    private:
        std::unordered_map<unsigned int, QVariant> in_dictionary;
        QVariant val;
        MathMethod m_mathMethod = MathMethod::Add;
        MathPortConfig m_portConfig;
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

    class MathMaxDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathMaxDataModel() : MathOperationBaseDataModel(MathMethod::Max, "Math Max") {}
    };

    class MathMinDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathMinDataModel() : MathOperationBaseDataModel(MathMethod::Min, "Math Min") {}
    };

    class MathAbsDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathAbsDataModel() : MathOperationBaseDataModel(MathMethod::Abs, "Math Abs") {}
    };

    class MathFloorDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathFloorDataModel() : MathOperationBaseDataModel(MathMethod::Floor, "Math Floor") {}
    };

    class MathCeilDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathCeilDataModel() : MathOperationBaseDataModel(MathMethod::Ceil, "Math Ceil") {}
    };

    class MathRoundDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathRoundDataModel() : MathOperationBaseDataModel(MathMethod::Round, "Math Round") {}
    };

    class MathSqrtDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathSqrtDataModel() : MathOperationBaseDataModel(MathMethod::Sqrt, "Math Sqrt") {}
    };

    class MathLogDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathLogDataModel() : MathOperationBaseDataModel(MathMethod::Log, "Math Log") {}
    };

    class MathAvgDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathAvgDataModel() : MathOperationBaseDataModel(MathMethod::Avg, "Math Avg") {}
    };

    class MathHypotDataModel final : public MathOperationBaseDataModel
    {
    public:
        MathHypotDataModel() : MathOperationBaseDataModel(MathMethod::Hypot, "Math Hypot") {}
    };
}
