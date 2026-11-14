#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

// This needs to be the same as the name of your project file ${PROJECT_NAME}
#ifdef TimeLineAudioBusNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "TimeLine Audio Bus"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"  // 构建系统注入时会覆盖
#endif
#define PLUGIN_DESCRIBE "时间线音频总线：timeline 混音经命名环形队列进入节点图（0 号口 SYNC 发布播放头，其余口为音频 bus）"
#define PLUGIN_TAG "Audio"
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

    QString name() const override { return PLUGIN_NAME; };
    QString version() const override {return PLUGIN_VERSION;};
    QString describe() const override {return PLUGIN_DESCRIBE;};
    QString tag() const override {return PLUGIN_TAG;};
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg) override;

private:
    static Plugin *_this_plugin;
};
