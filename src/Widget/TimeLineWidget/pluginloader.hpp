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

    void loadPlugins() ;

    QList<QObject*> getLoadedPlugins() const;

    QStringList getAvailableTypes() const;

    ClipPlugInterface* getPluginByType(const QString& type) const;

    AbstractClipModel* createModelForType(const QString& type, int start) const ;

    AbstractClipDelegate* createDelegateForType(const QString& type) const;

signals:
    void pluginLoaded(QObject* plugin);
    void pluginsLoaded();

private:
    QList<QObject*> m_loadedPlugins;
    QMap<QString, ClipPlugInterface*> m_pluginsByType;
};

#endif // PLUGINLOADER_HPP