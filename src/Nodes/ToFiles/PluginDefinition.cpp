#include "PluginDefinition.hpp"

#include "ToAudioFileDataModel.hpp"
#include "ToImageFileDataModel.hpp"
#include "ToTextFileDataModel.hpp"
#include "ToVideoFileDataModel.hpp"

Plugin* Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin() = default;

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg)
{
    assert(reg);

    reg->registerModel<Nodes::ToImageFileDataModel>(QStringLiteral("To Image File"), "Image");
    reg->registerModel<Nodes::ToVideoFileDataModel>(QStringLiteral("To Video File"), "Image");
    reg->registerModel<Nodes::ToAudioFileDataModel>(QStringLiteral("To Audio File"), "Audio");
    reg->registerModel<Nodes::ToTextFileDataModel>(QStringLiteral("To Text File"), tag());
}
