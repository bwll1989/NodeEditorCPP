#include "PluginDefinition.hpp"

#include "BlurImageOperateModel.hpp"
#include "ChromaKeyImageOperateModel.hpp"
#include "CrossImageOperateModel.hpp"
#include "CropImageOperateModel.hpp"
#include "DifferenceImageOperateModel.hpp"
#include "FlipImageOperateModel.hpp"
#include "UnderImageOperateModel.hpp"
#include "ImageSwitchDataModel.hpp"
#include "ImageThresholdDataModel.hpp"
#include "ImageScaleDataModel.hpp"
Plugin* Plugin::_this_plugin = nullptr;

Plugin::Plugin()
{
    _this_plugin = this;
}

Plugin::~Plugin() = default;

void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg)
{
    assert(reg);

    reg->registerModel<Nodes::FlipImageOperateModel>("Image Flip", tag());
    reg->registerModel<Nodes::CrossImageOperateModel>("Image Cross", tag());
    reg->registerModel<Nodes::DifferenceImageOperateModel>("Image Difference", tag());
    reg->registerModel<Nodes::BlurImageOperateModel>("Image Blur", tag());
    reg->registerModel<Nodes::CropImageOperateModel>("Image Crop", tag());
    reg->registerModel<Nodes::ChromaKeyImageOperateModel>("Image Chroma Key", tag());
    reg->registerModel<Nodes::UnderImageOperateModel>("Image Under", tag());
    reg->registerModel<Nodes::ImageSwitchDataModel>("Image Switch", tag());
    reg->registerModel<Nodes::ImageThresholdDataModel>("Image Threshold", tag());
    reg->registerModel<Nodes::ImageScaleDataModel>("Image Scale", tag());
}
