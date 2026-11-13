#ifndef PLUGINLOADER_CPP
#define PLUGINLOADER_CPP


#include "BasePluginLoader.h"

    BasePluginLoader::BasePluginLoader(QObject *parent) : QObject(parent) {}


    void BasePluginLoader::loadPlugins() {
        // 【round6 S3-1】基类默认实现改为 no-op：原死路径扫描
        // applicationDirPath()/plugins/TimeLine（本工程部署形态恒不存在，
        // 每次启动 qDebug 空转一行）。真实装载链 = WorkScenePluginLoader
        // 覆写（多候选目录磁盘扫描，见 QtTimelineModelAdapter.cpp）；基类
        // 实例仅作为 setPluginLoader 换装前的占位默认（BaseTimeLineModel
        // 构造函数），无插件可载是常态而非异常，静默即可。
    }

    QStringList BasePluginLoader::getAvailableTypes() const {
        return m_pluginsByType.keys();
    }

    ClipPlugInterface* BasePluginLoader::getPluginByType(const QString& type) const {
        return m_pluginsByType.value(type, nullptr);
    }

    AbstractClipModel* BasePluginLoader::createModelForType(const QString& type, qint64 startUs) const {
        ClipPlugInterface* plugin = getPluginByType(type);
        if (plugin) {
            return plugin->createModel(startUs);
        }
        return nullptr;
    }

    void BasePluginLoader::registerPlugin(ClipPlugInterface* plugin) {
        if (!plugin) return;
        const QString type = plugin->clipType();
        m_pluginsByType[type] = plugin;
    }

#endif // PLUGINLOADER_HPP
