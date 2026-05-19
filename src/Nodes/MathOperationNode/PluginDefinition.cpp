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
}
