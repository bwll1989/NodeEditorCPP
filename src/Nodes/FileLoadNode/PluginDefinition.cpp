#include "PluginDefinition.hpp"

#include "JsonLoaderModel.hpp"
#include "CsvLoaderModel.hpp"
#include "IniLoaderModel.hpp"
#include "ImageLoaderModel.hpp"
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

    reg->registerModel<Nodes::JsonLoaderModel>("JSON File",tag());
    // reg->registerModel<Nodes::CsvLoaderModel>("Csv File",tag());
    reg->registerModel<Nodes::IniLoaderModel>("INI File",tag());
    reg->registerModel<Nodes::ImageLoaderModel>("Image File","Image");
}
