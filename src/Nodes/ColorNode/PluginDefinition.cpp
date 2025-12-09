#include "PluginDefinition.hpp"

#include "ColorDataModel.hpp"
#include "ToRGBADataModel.hpp"
#include "ToHSVDataModel.hpp"
#include "ToRGBAFloatDataModel.hpp"
#include "ColorFDataModel.hpp"
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

    reg->registerModel<Nodes::ColorDataModel>("Color",tag());
    reg->registerModel<Nodes::ColorFDataModel>("Color Float",tag());
    reg->registerModel<Nodes::ToRGBADataModel>("To RGBA",tag());
    reg->registerModel<Nodes::ToHSVDataModel>("To HSV",tag());
    reg->registerModel<Nodes::ToRGBAFloatDataModel>("To Float RGBA",tag());
}
