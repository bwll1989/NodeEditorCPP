#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>
#include "Vst3PluginManager.hpp"
// This needs to be the same as the name of your project file ${PROJECT_NAME}
#ifdef VST3PluginNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif


#define PLUGIN_NAME "VST3 Plugin"
#define PLUGIN_VERSION "20240410"
#define PLUGIN_DESCRIBE "VST3插件"
#define PLUGIN_TAG "Audio"
class DLL_EXPORT Plugin
    : public QObject
    , public QtNodes::PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QtNodes::PluginInterface)
    Q_PLUGIN_METADATA(IID PLUGIN_NAME)

public:
    Plugin();
    ~Plugin();

    QString name() const override { return PLUGIN_NAME; }
    QString version() const override {return PLUGIN_VERSION;}
    QString describe() const override {return PLUGIN_DESCRIBE;}
    QString tag() const override {return PLUGIN_TAG;}
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg) override;
private:
    /**
     * @brief 初始化插件管理器，扫描插件目录
     */
    void initializePluginManager();

    /**
     * @brief 为单个JS插件注册工厂函数
     * @param reg 节点注册表
     * @param pluginInfo JS插件信息
     */
    void registerVst3Plugin(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg,
                         const QString& pluginPath);

    /**
     * @brief 获取插件目录路径
     * @return 插件目录的绝对路径
     */
    QString getPluginDirectory() const;

private:
    static Plugin *_this_plugin;
    Nodes::Vst3PluginManager *m_pluginManager;       // Vst3插件管理器
    QList<QString> m_Vst3Plugins;        // 已发现的Vst3插件列表
};
