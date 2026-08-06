#include "PluginDefinition.hpp"
#include "ModBusSlaveCoilDataModel.hpp"

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
    reg->registerModel<Nodes::ModBusSlaveCoilDataModel>(name(), tag());
}
