/**
 * @file PluginDefinition.cpp
 * @brief Q-SYS QRC 插件注册实现
 */
#include "PluginDefinition.hpp"
#include "QSysQrcDataModel.hpp"

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
    reg->registerModel<Nodes::QSysQrcDataModel>(name(), tag());
}
