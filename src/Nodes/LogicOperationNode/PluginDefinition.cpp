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
    reg->registerModel<Nodes::LogicXorDataModel>("Logic Xor", tag());
    reg->registerModel<Nodes::LogicNandDataModel>("Logic Nand", tag());
    reg->registerModel<Nodes::LogicNorDataModel>("Logic Nor", tag());
    reg->registerModel<Nodes::LogicNotDataModel>("Logic Not", tag());
    reg->registerModel<Nodes::LogicEqualDataModel>("Logic Equal", tag());
    reg->registerModel<Nodes::LogicNotEqualDataModel>("Logic NotEqual", tag());
    reg->registerModel<Nodes::LogicEqualNumDataModel>("Logic EqualNum", tag());
    reg->registerModel<Nodes::LogicIsEmptyDataModel>("Logic IsEmpty", tag());
    reg->registerModel<Nodes::LogicLessDataModel>("Logic Less", tag());
    reg->registerModel<Nodes::LogicLessEqualDataModel>("Logic LessEqual", tag());
    reg->registerModel<Nodes::LogicGreaterDataModel>("Logic Greater", tag());
    reg->registerModel<Nodes::LogicGreaterEqualDataModel>("Logic GreaterEqual", tag());
}
