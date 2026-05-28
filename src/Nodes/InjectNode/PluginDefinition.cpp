#include "PluginDefinition.hpp"

#include "InjectDataModel.hpp"
#include "DistributeDataModel.hpp"

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

    reg->registerModel<Nodes::InjectDataModel>(name(), tag());
    reg->registerModel<Nodes::DistributeDataModel>(QStringLiteral("Distribute"), tag());
}
