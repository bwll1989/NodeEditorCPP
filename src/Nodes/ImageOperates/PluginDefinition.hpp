#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

#ifdef ImageOperates_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "ImageOperates"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif
#define PLUGIN_DESCRIBE "图像操作节点集合"
#define PLUGIN_TAG "Image"

/**
 * @brief ImageOperates 插件入口
 */
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
    ~Plugin() override;

    QString name() const override { return PLUGIN_NAME; }
    QString version() const override { return PLUGIN_VERSION; }
    QString describe() const override { return PLUGIN_DESCRIBE; }
    QString tag() const override { return PLUGIN_TAG; }

    /**
     * @brief 向注册表注册本插件中的所有节点
     * @param reg 节点注册表
     */
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg) override;

private:
    static Plugin* _this_plugin;
};
