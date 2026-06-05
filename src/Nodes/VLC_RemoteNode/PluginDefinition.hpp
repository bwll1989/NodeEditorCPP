/**
 * @file PluginDefinition.hpp
 * @brief VLC Remote 节点插件注册入口
 *
 * 实现 QtNodes::PluginInterface，将 VlcRemoteDataModel 注册到节点编辑器。
 * 编译产物为 VLC_RemoteNode.node 动态库。
 */
#pragma once

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

#ifdef VLC_RemoteNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "VLC Remote"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif
#define PLUGIN_DESCRIBE "VLC 播放器 HTTP 远程控制"
#define PLUGIN_TAG "Devices"

/**
 * @class Plugin
 * @brief Flow 节点插件包装类
 *
 * 通过 Q_PLUGIN_METADATA 导出，由节点编辑器在启动时自动加载。
 */
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

    /** @brief 向注册表添加 VlcRemoteDataModel 节点类型 */
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg) override;

private:
    static Plugin *_this_plugin;
};
