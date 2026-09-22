#include "PluginDefinition.hpp"

#include "AudioMatrixDataModel.hpp"
#include "AudioRouterDataModel.hpp"

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

    reg->registerModel<Nodes::AudioMatrixDataModel>(name(), tag());
    reg->registerModel<Nodes::AudioRouterDataModel>(QStringLiteral("Audio Router"), tag());
}
