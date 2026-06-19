#include "PluginDefinition.hpp"
#include "ROIDataModel.hpp"

Plugin* Plugin::_this_plugin = nullptr;

/**
 * @brief 构造函数
 */
Plugin::Plugin()
{
    _this_plugin = this;
}

/**
 * @brief 析构函数
 */
Plugin::~Plugin() = default;

/**
 * @brief 注册 ROI 节点模型
 * @param reg 节点模型注册表
 */
void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg)
{
    assert(reg);
    reg->registerModel<Nodes::ROIDataModel>("ROI", tag());
}
