#include "PluginDefinition.hpp"

#include "DAWControllerDataModel.hpp"
#include "CurtainDataModel.hpp"
#include "FTControlBoxDataModel.hpp"
#include "PeripProtoServerDataModel.hpp"
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

    reg->registerModel<Nodes::DAWControllerNode>("FT-DAWController",tag());
    reg->registerModel<Nodes::CurtainDataModel>("FT-CurtainController",tag());
    reg->registerModel<Nodes::FTControlBoxDataModel>("FT-ControlBox", tag());
    reg->registerModel<Nodes::PeripProtoServerDataModel>("FT-LocationProto", tag());
}
