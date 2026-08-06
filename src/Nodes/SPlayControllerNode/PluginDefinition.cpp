/**
 * @file PluginDefinition.cpp
 * @brief SPlay Controller 插件注册实现
 */
#include "PluginDefinition.hpp"
#include "SPlayControllerDataModel.hpp"

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
    reg->registerModel<Nodes::SPlayControllerDataModel>(name(), tag());
}
