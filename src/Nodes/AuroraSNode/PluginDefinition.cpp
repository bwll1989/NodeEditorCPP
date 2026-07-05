#include "PluginDefinition.hpp"
#include "AuroraSDataModel.hpp"

Plugin* Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin() = default;

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg)
{
    reg->registerModel<Nodes::AuroraSDataModel>(name(), tag());
}
