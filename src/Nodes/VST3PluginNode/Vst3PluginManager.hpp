#pragma once

#include <QObject>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QDebug>

namespace Nodes {



class Vst3PluginManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param pluginDir 插件目录路径
     */
    explicit Vst3PluginManager(const QString& pluginDir, QObject *parent = nullptr);
    
    /**
     * @brief 扫描插件目录，发现所有Vst3插件
     */
    void scanPlugins();
    
    /**
     * @brief 获取所有可用的插件列表
     * @return 插件信息列表
     */
    QList<QString> getAvailablePlugins() const;

private:
    QString m_pluginDir;                    // 插件目录
    QList<QString> m_plugins;          // 插件列表
};

}