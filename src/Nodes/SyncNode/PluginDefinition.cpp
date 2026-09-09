/**
 * @file PluginDefinition.cpp
 * @brief 注册 Sync Out / Sync In 两个节点模型。
 */

#include "PluginDefinition.hpp"

#include "SyncOutDataModel.hpp"
#include "SyncInDataModel.hpp"

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
    // 显示名需与 DataModel::Caption 一致，便于创建与序列化匹配
    reg->registerModel<Nodes::SyncOutDataModel>(QStringLiteral("Sync Out"), tag());
    reg->registerModel<Nodes::SyncInDataModel>(QStringLiteral("Sync In"), tag());
}
