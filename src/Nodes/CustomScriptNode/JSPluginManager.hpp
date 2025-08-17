#pragma once

#include <QObject>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QDebug>

namespace Nodes {

/**
 * @brief JS插件信息结构
 */
struct JSPluginInfo {
    QString name;           // 插件名称
    QString description;    // 插件描述
    QString version;        // 插件版本
    QString author;         // 插件作者
    QString category;       // 插件分类
    QString filePath;       // JS文件路径
    QString code;           // JS代码内容
    QJsonObject metadata;   // 元数据
    
    // 节点配置属性
    int inputs = 1;         // 输入端口数量
    int outputs = 1;        // 输出端口数量
    bool embeddable = true; // 是否可嵌入UI控件
    bool resizable = true;  // 是否可调整大小
    bool portEditable = false; // 端口是否可编辑
};

/**
 * @brief JS插件管理器
 * 负责扫描、加载和管理JS插件文件
 */
class JSPluginManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param pluginDir 插件目录路径
     */
    explicit JSPluginManager(const QString& pluginDir, QObject *parent = nullptr);
    
    /**
     * @brief 扫描插件目录，发现所有JS插件
     */
    void scanPlugins();
    
    /**
     * @brief 获取所有可用的插件列表
     * @return 插件信息列表
     */
    QList<JSPluginInfo> getAvailablePlugins() const;
    
    /**
     * @brief 根据名称获取插件信息
     * @param name 插件名称
     * @return 插件信息，如果未找到返回空的JSPluginInfo
     */
    JSPluginInfo getPlugin(const QString& name) const;
    
    /**
     * @brief 获取插件分类列表
     * @return 分类列表
     */
    QStringList getCategories() const;
    
    /**
     * @brief 根据分类获取插件列表
     * @param category 分类名称
     * @return 该分类下的插件列表
     */
    QList<JSPluginInfo> getPluginsByCategory(const QString& category) const;

private:
    /**
     * @brief 解析JS文件头部的元数据注释
     * @param filePath JS文件路径
     * @return 解析出的插件信息
     */
    JSPluginInfo parsePluginFile(const QString& filePath);
    
    /**
     * @brief 从JS注释中提取元数据
     * @param jsCode JS代码内容
     * @return 元数据JSON对象
     */
    QJsonObject extractMetadata(const QString& jsCode);
    
    /**
     * @brief 从元数据中解析节点配置属性
     * @param metadata 元数据JSON对象
     * @param plugin 插件信息引用，用于设置属性
     */
    void parseNodeAttributes(const QJsonObject& metadata, JSPluginInfo& plugin);

private:
    QString m_pluginDir;                    // 插件目录
    QList<JSPluginInfo> m_plugins;          // 插件列表
};

}