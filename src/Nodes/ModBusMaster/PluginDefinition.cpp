#include "PluginDefinition.hpp"
#include "ModBusMasterCoilControllerDataModel.hpp"
#include "ModBusMasterDiscreteInputDataModel.hpp"

Plugin *Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin()
{
}

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg)
{
    assert(reg);
    reg->registerModel<Nodes::ModBusMasterCoilControllerDataModel>(name(), tag());
    reg->registerModel<Nodes::ModBusMasterDiscreteInputDataModel>("ModBus Discrete Input", tag());
}
