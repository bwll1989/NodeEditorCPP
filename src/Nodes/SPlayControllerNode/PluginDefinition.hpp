/**
 * @file PluginDefinition.hpp
 * @brief SPlay Controller 节点插件注册入口
 */
#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

#ifdef SPlayControllerNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "SPlay Controller"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif
#define PLUGIN_DESCRIBE "ENTTEC S-Play 多 playlist 并发控制（按 Index 端口播/停）"
#define PLUGIN_TAG "Devices"

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
