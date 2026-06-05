/**
 * @file PluginDefinition.cpp
 * @brief VLC Remote 插件注册实现
 */
#include "PluginDefinition.hpp"
#include "VlcRemoteDataModel.hpp"

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
    // 注册到 "Devices" 分类，显示名称为 "VLC Remote"
    reg->registerModel<Nodes::VlcRemoteDataModel>(name(), tag());
}
