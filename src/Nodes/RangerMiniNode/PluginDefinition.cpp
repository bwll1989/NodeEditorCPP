#include "PluginDefinition.hpp"
#include "RangerMiniDataModel.hpp"
#include "NavControllerNode/NavControllerDataModel.hpp"

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
    reg->registerModel<Nodes::RangerMiniDataModel>(name(), QStringLiteral("Devices"));
    reg->registerModel<Nodes::NavControllerDataModel>(QStringLiteral("Nav Controller"), QStringLiteral("Devices"));
}
