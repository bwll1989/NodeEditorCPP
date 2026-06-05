#include "PluginDefinition.hpp"

#include "LFODataModel.hpp"
#include "MetronomeDataModel.hpp"

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

    reg->registerModel<Nodes::LFODataModel>(name(), tag());
    reg->registerModel<Nodes::MetronomeDataModel>(QStringLiteral("Metronome"), tag());
}
