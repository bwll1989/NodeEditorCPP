#include "PluginDefinition.hpp"

#include "InternalControlModel.hpp"
#include "OscOutGroupModel.hpp"
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

    reg->registerModel<Nodes::InternalControlModel>(name(),tag());
    reg->registerModel<Nodes::OscOutGroupModel>("OscOutGroup","Communications");
}
