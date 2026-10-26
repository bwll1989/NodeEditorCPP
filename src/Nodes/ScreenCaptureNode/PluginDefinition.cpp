#include "PluginDefinition.hpp"

#include "ScreenCaptureModel.hpp"

Plugin* Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin() = default;

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg)
{
    assert(reg);
    reg->registerModel<Nodes::ScreenCaptureModel>(name(), tag());
}
