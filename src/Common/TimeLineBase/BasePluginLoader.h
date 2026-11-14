#ifndef BASEPLUGINLOADER_H
#define BASEPLUGINLOADER_H

#include <QObject>
#include <QDir>
#include <QPluginLoader>
#include <QDebug>
#include <QApplication>
#include "AbstractClipInterface.h"
#include "TimelineExports.hpp"

// 【2026-11-13 迁移】原在 BaseClass（BASECLASS_EXPORT）——职责重定位：
// 本类是「宿主侧片段类型注册表/装载器基类」（BaseTimeLineModel 构造占位
// 默认 + 宿主 WorkScenePluginLoader 磁盘装载链的基类），不是片段作者实现
// 时引用的基类（片段作者只用 ClipPlugInterface/AbstractClipModel/
// SceneClipModel）——故随唯一库内消费者 BaseTimeLineModel 迁入 TimeLineBase。
class NODE_TIMELINE_PUBLIC BasePluginLoader : public QObject
{
    Q_OBJECT
public:
    explicit BasePluginLoader(QObject *parent = nullptr) ;

    /**
     * 加载插件
     */
    virtual void loadPlugins() ;
    /**
     * 获取所有插件类型
     * @return QStringList 插件类型列表
     */
    virtual QStringList getAvailableTypes() const;
    /**
     * 获取指定类型的插件
     * @param const QString& type 类型
    */
    virtual ClipPlugInterface* getPluginByType(const QString& type) const;
    /**
     * 创建指定类型的模型
     * @param const QString& type 类型
     * @param int start 开始
    */
    virtual AbstractClipModel* createModelForType(const QString& type, qint64 startUs) const;
    /**
     * 注册进程内插件（不经磁盘扫描——宿主把插件实例直接挂进类型表，
     * 与磁盘 DLL 插件共用同一张 m_pluginsByType 表；磁盘装载链
     * WorkScenePluginLoader 也经此入表）。
     * @param ClipPlugInterface* plugin 插件指针
    */
    void registerPlugin(ClipPlugInterface* plugin);

private:
    // 插件类型表（type → 插件实例）
    QMap<QString, ClipPlugInterface*> m_pluginsByType;
};

#endif // PLUGINLOADER_H
