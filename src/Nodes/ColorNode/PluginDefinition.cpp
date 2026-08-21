#include "PluginDefinition.hpp"

#include "ColorDataModel.hpp"
#include "HsvColorDataModel.hpp"

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

    reg->registerModel<Nodes::ColorDataModel>("RGBA", tag());
    reg->registerModel<Nodes::HsvColorDataModel>("HSV", tag());
}
