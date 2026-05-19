#include "PluginDefinition.hpp"

#include "LogicOperationDataModel.hpp"

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

    reg->registerModel<Nodes::LogicAndDataModel>("Logic And", tag());
    reg->registerModel<Nodes::LogicOrDataModel>("Logic Or", tag());
    reg->registerModel<Nodes::LogicNotEqualDataModel>("Logic NotEqual", tag());
    reg->registerModel<Nodes::LogicMaxDataModel>("Logic Max", tag());
    reg->registerModel<Nodes::LogicMinDataModel>("Logic Min", tag());
    reg->registerModel<Nodes::LogicLessDataModel>("Logic Less", tag());
    reg->registerModel<Nodes::LogicLessEqualDataModel>("Logic LessEqual", tag());
    reg->registerModel<Nodes::LogicGreaterDataModel>("Logic Greater", tag());
    reg->registerModel<Nodes::LogicGreaterEqualDataModel>("Logic GreaterEqual", tag());
}
