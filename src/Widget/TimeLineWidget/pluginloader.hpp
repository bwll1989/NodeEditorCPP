#ifndef PLUGINLOADER_HPP
#define PLUGINLOADER_HPP

#include <QObject>
#include <QDir>
#include <QPluginLoader>
#include <QDebug>
#include <QApplication>
#include "clippluginterface.hpp"

class PluginLoader : public QObject
{
    Q_OBJECT
public:
    explicit PluginLoader(QObject *parent = nullptr) ;

    /*
        加载插件
    */
    void loadPlugins() ;

    QList<QObject*> getLoadedPlugins() const;
    /*
        获取所有插件类型
    */
    QStringList getAvailableTypes() const;
    /*
        获取指定类型的插件
    */
    ClipPlugInterface* getPluginByType(const QString& type) const;
    /*
        创建指定类型的模型
    */      
    AbstractClipModel* createModelForType(const QString& type, int start) const ;
    /*
        创建指定类型的代理
    */
    AbstractClipDelegate* createDelegateForType(const QString& type) const;

signals:
    /*
        插件加载完成信号
    */
    void pluginLoaded(QObject* plugin);
    /*
        所有插件加载完成信号
    */
    void pluginsLoaded();

private:
    // 加载的插件列表
    QList<QObject*> m_loadedPlugins;
    // 插件类型列表
    QMap<QString, ClipPlugInterface*> m_pluginsByType;
};

#endif // PLUGINLOADER_HPP