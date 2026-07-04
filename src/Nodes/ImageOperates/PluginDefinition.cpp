#include "PluginDefinition.hpp"

/**
 * @file PluginDefinition.cpp
 * @brief ImageOperates 插件注册 — 节点列表与 Doc.md §1 一致
 */

#include "AddImageOperateModel.hpp"
#include "BlurImageOperateModel.hpp"
#include "ChromaKeyImageOperateModel.hpp"
#include "CrossImageOperateModel.hpp"
#include "CropImageOperateModel.hpp"
#include "DifferenceImageOperateModel.hpp"
#include "SubtractImageOperateModel.hpp"
#include "FlipImageOperateModel.hpp"
#include "ChannelImageOperateModel.hpp"
#include "LevelImageOperateModel.hpp"
#include "MonochromeImageOperateModel.hpp"
#include "OverImageOperateModel.hpp"
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

    // GPU 算子（tick 驱动）；Image Switch 为透传路由
    reg->registerModel<Nodes::FlipImageOperateModel>("Image Flip", tag());
    reg->registerModel<Nodes::CrossImageOperateModel>("Image Cross", tag());
    reg->registerModel<Nodes::AddImageOperateModel>("Image Add", tag());
    reg->registerModel<Nodes::SubtractImageOperateModel>("Image Subtract", tag());
    reg->registerModel<Nodes::DifferenceImageOperateModel>("Image Difference", tag());
    reg->registerModel<Nodes::BlurImageOperateModel>("Image Blur", tag());
    reg->registerModel<Nodes::CropImageOperateModel>("Image Crop", tag());
    reg->registerModel<Nodes::ChromaKeyImageOperateModel>("Image Chroma Key", tag());
    reg->registerModel<Nodes::UnderImageOperateModel>("Image Under", tag());
    reg->registerModel<Nodes::OverImageOperateModel>("Image Over", tag());
    reg->registerModel<Nodes::MonochromeImageOperateModel>("Image Monochrome", tag());
    reg->registerModel<Nodes::LevelImageOperateModel>("Image Level", tag());
    reg->registerModel<Nodes::ChannelImageOperateModel>("Image Channel", tag());
    reg->registerModel<Nodes::ImageSwitchDataModel>("Image Switch", tag());
    reg->registerModel<Nodes::ImageThresholdDataModel>("Image Threshold", tag());
    reg->registerModel<Nodes::ImageScaleDataModel>("Image Scale", tag());
}
