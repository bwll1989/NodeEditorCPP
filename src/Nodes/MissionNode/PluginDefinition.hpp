#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

#ifdef MissionNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "Nav Missions"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif
#define PLUGIN_DESCRIBE "导航任务序列：导航 / 等待 / 动作"
#define PLUGIN_TAG "Controls"

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
    QString version() const override { return PLUGIN_VERSION; }
    QString describe() const override { return PLUGIN_DESCRIBE; }
    QString tag() const override { return PLUGIN_TAG; }
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg) override;

private:
    static Plugin *_this_plugin;
};
