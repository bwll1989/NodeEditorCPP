#include "PluginDefinition.hpp"

#include "CountDataModel.hpp"
#include "RangeMapDataModel.hpp"
#include "FeedbackDataModel.hpp"
#include "LogOutDataModel.hpp"

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

    reg->registerModel<Nodes::CountDataModel>(name(), tag());
    reg->registerModel<Nodes::RangeMapDataModel>(QStringLiteral("Range Map"), tag());
    reg->registerModel<Nodes::FeedbackDataModel>(QStringLiteral("Feedback"), tag());
    reg->registerModel<Nodes::LogOutDataModel>(QStringLiteral("Log Out"), tag());
}
