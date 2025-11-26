#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>
#include "JSPluginManager.hpp"


// This needs to be the same as the name of your project file ${PROJECT_NAME}
#ifdef CustomScriptNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"  // 构建系统注入时会覆盖
#endif
/**
 * @brief 动态JS插件注册器
 * 根据JS文件中的元数据动态注册插件，使用工厂模式创建节点实例
 */
class DLL_EXPORT Plugin
    : public QObject
    , public QtNodes::PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QtNodes::PluginInterface)
    Q_PLUGIN_METADATA(IID "CustomScriptNode")

public:
    /**
     * @brief 构造函数，初始化插件管理器并扫描JS插件
     */
    Plugin();
    
    /**
     * @brief 析构函数，清理资源
     */
    ~Plugin();

    /**
     * @brief 获取插件系统名称
     * @return 插件系统名称
     */
    QString name() const override;
    
    /**
     * @brief 获取插件系统版本
     * @return 版本号
     */
    QString version() const override;
    
    /**
     * @brief 获取插件系统描述
     * @return 包含已加载插件数量的描述
     */
    QString describe() const override;
    
    /**
     * @brief 获取插件系统标签
     * @return 插件标签
     */
    QString tag() const override;
    
    /**
     * @brief 注册数据模型，为每个JS插件动态创建节点类型
     * @param reg 节点注册表
     */
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
    void registerJSPlugin(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg, 
                         const Nodes::JSPluginInfo& pluginInfo);
    
    /**
     * @brief 获取插件目录路径
     * @return 插件目录的绝对路径
     */
    QString getPluginDirectory() const;

private:
    static Plugin *_this_plugin;                    // 插件单例指针
    Nodes::JSPluginManager *m_pluginManager;       // JS插件管理器
    QList<Nodes::JSPluginInfo> m_jsPlugins;        // 已发现的JS插件列表
};
