#pragma once

/**
 * @file PluginDefinition.hpp
 * @brief FaustNode 插件入口（Qt PluginInterface）
 * 注册 Nodes::FaustDataModel，节点库显示名 "Faust DSP"，标签 Audio。
 */

#include <QObject>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

#ifdef FaustNode_EXPORTS
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

#define PLUGIN_NAME "FaustNode"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "dev"
#endif
#define PLUGIN_DESCRIBE "Faust DSP：加载用户 .dsp 文件做音频处理"
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
    ~Plugin() override;

    QString name() const override { return PLUGIN_NAME; }
    QString version() const override { return PLUGIN_VERSION; }
    QString describe() const override { return PLUGIN_DESCRIBE; }
    QString tag() const override { return PLUGIN_TAG; }
    bool NeedAuthorized() const override { return false; }
    void registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry>& reg) override;

private:
    static Plugin* _this_plugin;
};
