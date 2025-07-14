#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>


// This needs to be the same as the name of your project file ${PROJECT_NAME}
#ifdef TCPServerNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "TCP Server"
#define PLUGIN_VERSION "20250707"
#define PLUGIN_DESCRIBE "TCP服务器"
#define PLUGIN_TAG "Network"
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
