#include "PluginDefinition.hpp"

#include "DAWControllerDataModel.hpp"
#include "CurtainDataModel.hpp"
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

    reg->registerModel<Nodes::DAWControllerNode>("DAW Controller",tag());
    reg->registerModel<Nodes::CurtainDataModel>("Curtain Controller",tag());
}
