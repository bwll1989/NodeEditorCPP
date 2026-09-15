/**
 * @file PluginDefinition.cpp
 * @brief 注册 Nodes::ViosoDataModel，分类标签为 Image
 */

#include "PluginDefinition.hpp"
#include "ViosoDataModel.hpp"

Plugin* Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin() = default;

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg)
{
    assert(reg);
    reg->registerModel<Nodes::ViosoDataModel>("Vioso", tag());
}
