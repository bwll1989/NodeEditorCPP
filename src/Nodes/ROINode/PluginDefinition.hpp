#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

#ifdef ROINode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "ROI"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif
#define PLUGIN_DESCRIBE "交互式 ROI 图像裁剪"
#define PLUGIN_TAG "Image"

class DLL_EXPORT Plugin
    : public QObject
    , public QtNodes::PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QtNodes::PluginInterface)
    Q_PLUGIN_METADATA(IID PLUGIN_NAME)

public:
    /**
     * @brief 构造函数
     */
    Plugin();

    /**
     * @brief 析构函数
     */
    ~Plugin();

    /**
     * @brief 返回插件名称
     * @return 插件名称
     */
    QString name() const override { return PLUGIN_NAME; }

    /**
     * @brief 返回插件版本
     * @return 插件版本
     */
    QString version() const override { return PLUGIN_VERSION; }

    /**
     * @brief 返回插件说明
     * @return 插件说明
     */
    QString describe() const override { return PLUGIN_DESCRIBE; }

    /**
     * @brief 返回插件分类标签
     * @return 分类标签
     */
    QString tag() const override { return PLUGIN_TAG; }

    /**
     * @brief 向注册表注册数据模型
     * @param reg 节点模型注册表
     */
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg) override;

private:
    static Plugin* _this_plugin;
};
