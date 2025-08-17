#include "JSPluginManager.hpp"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QJsonParseError>

namespace Nodes {

/**
 * @brief 构造函数，初始化插件管理器
 * @param pluginDir 插件目录路径
 * @param parent 父对象
 */
JSPluginManager::JSPluginManager(const QString& pluginDir, QObject *parent)
    : QObject(parent), m_pluginDir(pluginDir)
{
    // 确保插件目录存在
    QDir dir(m_pluginDir);
    if (!dir.exists()) {
        dir.mkpath(m_pluginDir);
    }
}

/**
 * @brief 扫描插件目录，发现所有JS插件
 */
void JSPluginManager::scanPlugins()
{
    m_plugins.clear();
    
    QDir pluginDir(m_pluginDir);
    QStringList filters;
    filters << "*.js";
    
    // 扫描JS文件
    QFileInfoList fileList = pluginDir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo& fileInfo : fileList) {
        JSPluginInfo plugin = parsePluginFile(fileInfo.absoluteFilePath());
        if (!plugin.name.isEmpty()) {
            m_plugins.append(plugin);
        }
    }
    

}

/**
 * @brief 获取所有可用的插件列表
 * @return 插件信息列表
 */
QList<JSPluginInfo> JSPluginManager::getAvailablePlugins() const
{
    return m_plugins;
}

/**
 * @brief 根据名称获取插件信息
 * @param name 插件名称
 * @return 插件信息，如果未找到返回空的JSPluginInfo
 */
JSPluginInfo JSPluginManager::getPlugin(const QString& name) const
{
    for (const JSPluginInfo& plugin : m_plugins) {
        if (plugin.name == name) {
            return plugin;
        }
    }
    return JSPluginInfo(); // 返回空的插件信息
}

/**
 * @brief 获取插件分类列表
 * @return 分类列表
 */
QStringList JSPluginManager::getCategories() const
{
    QStringList categories;
    for (const JSPluginInfo& plugin : m_plugins) {
        if (!plugin.category.isEmpty() && !categories.contains(plugin.category)) {
            categories.append(plugin.category);
        }
    }
    categories.sort();
    return categories;
}

/**
 * @brief 根据分类获取插件列表
 * @param category 分类名称
 * @return 该分类下的插件列表
 */
QList<JSPluginInfo> JSPluginManager::getPluginsByCategory(const QString& category) const
{
    QList<JSPluginInfo> result;
    for (const JSPluginInfo& plugin : m_plugins) {
        if (plugin.category == category) {
            result.append(plugin);
        }
    }
    return result;
}

/**
 * @brief 解析JS文件头部的元数据注释
 * @param filePath JS文件路径
 * @return 解析出的插件信息
 */
JSPluginInfo JSPluginManager::parsePluginFile(const QString& filePath)
{
    JSPluginInfo plugin;
    plugin.filePath = filePath;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开插件文件:" << filePath;
        return plugin;
    }
    
    QTextStream in(&file);
    plugin.code = in.readAll();
    file.close();
    
    // 提取元数据
    plugin.metadata = extractMetadata(plugin.code);
    
    // 从元数据中获取基本信息
    plugin.name = plugin.metadata.value("name").toString();
    plugin.description = plugin.metadata.value("description").toString();
    plugin.version = plugin.metadata.value("version").toString();
    plugin.author = plugin.metadata.value("author").toString();
    plugin.category = plugin.metadata.value("category").toString();
    
    // 解析节点配置属性
    parseNodeAttributes(plugin.metadata, plugin);
    
    // 如果没有名称，使用文件名
    if (plugin.name.isEmpty()) {
        QFileInfo fileInfo(filePath);
        plugin.name = fileInfo.baseName();
    }
    
    // 设置默认分类
    if (plugin.category.isEmpty()) {
        plugin.category = "通用";
    }
    
    return plugin;
}

/**
 * @brief 从JS代码中提取插件元数据
 * @param jsCode JS代码字符串
 * @return 元数据JSON对象
 */
QJsonObject JSPluginManager::extractMetadata(const QString& jsCode)
{
    QJsonObject metadata;
    
    // 查找JS文件头部的元数据注释块
    // 支持两种格式:
    // 1. /*! @plugin { "name": "插件名", ... } */
    // 2. /*! { "name": "插件名", ... } */
    QRegularExpression metaRegex(R"(/\*!\s*(?:@plugin\s+)?\s*({[^}]*(?:{[^}]*}[^}]*)*})\s*\*/)");
    QRegularExpressionMatch match = metaRegex.match(jsCode);
    
    if (match.hasMatch()) {
        QString jsonStr = match.captured(1);
        // 清理JSON字符串，移除多余的空白字符和换行符
        jsonStr = jsonStr.replace(QRegularExpression("\\s*\\n\\s*"), " ").simplified();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
        
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            metadata = doc.object();
        } else {
            qWarning() << "插件元数据JSON解析错误:" << error.errorString();
            qWarning() << "JSON内容:" << jsonStr;
        }
    } else {
        qDebug() << "未找到元数据注释块";
    }
    
    return metadata;
}

/**
 * @brief 从元数据中解析节点配置属性
 * @param metadata 元数据JSON对象
 * @param plugin 插件信息引用，用于设置属性
 */
void JSPluginManager::parseNodeAttributes(const QJsonObject& metadata, JSPluginInfo& plugin)
{
    // 解析输入端口数量
    if (metadata.contains("inputs")) {
        plugin.inputs = metadata.value("inputs").toInt(1);
    }
    
    // 解析输出端口数量
    if (metadata.contains("outputs")) {
        plugin.outputs = metadata.value("outputs").toInt(1);
    }
    
    // 解析是否可嵌入UI控件
    if (metadata.contains("embeddable")) {
        plugin.embeddable = metadata.value("embeddable").toBool(true);
    }
    
    // 解析是否可调整大小
    if (metadata.contains("resizable")) {
        plugin.resizable = metadata.value("resizable").toBool(true);
    }
    
    // 解析端口是否可编辑
    if (metadata.contains("portEditable")) {
        plugin.portEditable = metadata.value("portEditable").toBool(false);
    }
}

}