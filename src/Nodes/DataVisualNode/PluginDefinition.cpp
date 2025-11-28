#include "PluginDefinition.hpp"

#include "ScatterSeriesDataModel.hpp"
#include "ScatterSingleDataModel.hpp"

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

    reg->registerModel<Nodes::ScatterSeriesDataModel>("ScatterSeries",tag());
    reg->registerModel<Nodes::ScatterSingleDataModel>("ScatterSingle",tag());


}
