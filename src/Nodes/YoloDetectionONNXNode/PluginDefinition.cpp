#include "PluginDefinition.hpp"
#include <onnxruntime_cxx_api.h>
#include "YoloDetectionONNXDataModel.hpp"

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

        reg->registerModel<Nodes::YoloDetectionONNXDataModel>(name(),tag());
}
