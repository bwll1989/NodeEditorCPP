#include "PluginDefinition.hpp"

#include "Curve1D/Curve1DDataModel.hpp"
#include "Curve2D/Curve2DDataModel.hpp"
#include "Curve3D/Curve3DDataModel.hpp"

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

    // 一个插件（CurvesNode.node）提供三个子节点，菜单分类与合并前保持一致
    reg->registerModel<Nodes::Curve1DDataModel>(QStringLiteral("Curve1D"), tag());
    reg->registerModel<Nodes::Curve2DDataModel>(QStringLiteral("Curve2D"), tag());
    reg->registerModel<Nodes::Curve3DDataModel>(QStringLiteral("Curve3D"), tag());
}
