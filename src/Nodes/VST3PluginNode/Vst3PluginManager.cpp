#include "Vst3PluginManager.hpp"
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
Vst3PluginManager::Vst3PluginManager(const QString& pluginDir, QObject *parent)
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
void Vst3PluginManager::scanPlugins()
{
    m_plugins.clear();
    
    QDir pluginDir(m_pluginDir);
    QStringList filters;
    filters << "*.vst3";
    // 扫描JS文件
    QFileInfoList fileList = pluginDir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo& fileInfo : fileList) {
        m_plugins.append(fileInfo.absoluteFilePath());
    }
}

/**
 * @brief 获取所有可用的插件列表
 * @return 插件信息列表
 */
QList<QString> Vst3PluginManager::getAvailablePlugins() const
{
    return m_plugins;
}

}