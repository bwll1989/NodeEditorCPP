#include "PluginDefinition.hpp"

#include "AudioDeviceOutDataModel.hpp"

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
    //加载插件时就实例化对象一次，通过牺牲启动速度，换取模块打开速度
    reg->registerModelInstance<Nodes::AudioDeviceOutDataModel>(tag());
    // reg->registerModel<Nodes::AudioDeviceOutDataModel>(name(),tag());
}
