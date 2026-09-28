#include "PluginDefinition.hpp"
#include "AJTNodeDataModel.hpp"
#include "AJTRelayDataModel.hpp"
#include "AJTGatewayDataModel.hpp"

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
    reg->registerModel<Nodes::AJTNodeDataModel>(QStringLiteral("AJT Node"), tag());
    reg->registerModel<Nodes::AJTNodeDataModel>(QStringLiteral("AJT Dimming Node"), tag());
    reg->registerModel<Nodes::AJTRelayDataModel>(QStringLiteral("AJT Relay Node"), tag());
    reg->registerModel<Nodes::AJTGatewayDataModel>(QStringLiteral("AJT Gateway"), tag());
}
