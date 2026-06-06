#include "PluginDefinition.hpp"

#include "MathOperationDataModel.hpp"

Plugin *Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin()
{
    // TODO: Unregister all models here
}

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg)
{
    assert(reg);

    reg->registerModel<Nodes::MathAddDataModel>("Math Add", tag());
    reg->registerModel<Nodes::MathSubDataModel>("Math Sub", tag());
    reg->registerModel<Nodes::MathMulDataModel>("Math Mul", tag());
    reg->registerModel<Nodes::MathDivDataModel>("Math Div", tag());
    reg->registerModel<Nodes::MathModDataModel>("Math Mod", tag());
    reg->registerModel<Nodes::MathPowDataModel>("Math Pow", tag());
    reg->registerModel<Nodes::MathMaxDataModel>("Math Max", tag());
    reg->registerModel<Nodes::MathMinDataModel>("Math Min", tag());
    reg->registerModel<Nodes::MathAbsDataModel>("Math Abs", tag());
    reg->registerModel<Nodes::MathFloorDataModel>("Math Floor", tag());
    reg->registerModel<Nodes::MathCeilDataModel>("Math Ceil", tag());
    reg->registerModel<Nodes::MathRoundDataModel>("Math Round", tag());
    reg->registerModel<Nodes::MathSqrtDataModel>("Math Sqrt", tag());
    reg->registerModel<Nodes::MathLogDataModel>("Math Log", tag());
    reg->registerModel<Nodes::MathAvgDataModel>("Math Avg", tag());
    reg->registerModel<Nodes::MathHypotDataModel>("Math Hypot", tag());
}
