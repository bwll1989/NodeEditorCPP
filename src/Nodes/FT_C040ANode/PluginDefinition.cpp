#include "PluginDefinition.hpp"
#include "FTC040ADataModel.hpp"

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
    reg->registerModel<Nodes::FTC040ADataModel>(name(), tag());
}
