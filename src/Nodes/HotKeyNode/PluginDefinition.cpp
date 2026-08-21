#include "PluginDefinition.hpp"

#include "HotKeyDataModel.hpp"
#include "MouseInDataModel.hpp"

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

    reg->registerModel<Nodes::HotKeyDataModel>(name(), tag());
    // 兼容旧工程中保存的 "HotKey" 类型名
    reg->registerModel<Nodes::HotKeyDataModel>(QStringLiteral("HotKey"), tag());
    reg->registerModel<Nodes::MouseInDataModel>(QStringLiteral("Mouse In"), tag());
}
