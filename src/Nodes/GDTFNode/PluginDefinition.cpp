#include "PluginDefinition.hpp"

#include "GDTFDataModel.hpp"

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

    reg->registerModel<Nodes::GDTFDataModel>(name(), tag());
}
