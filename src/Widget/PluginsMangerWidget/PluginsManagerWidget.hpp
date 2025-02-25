#pragma once

#include <QDialog>
#include <QDir>
#include <QStandardItemModel>
#include <QtNodes/PluginsManager>

using QtNodes::PluginsManager;

class PluginsManagerWidget : public QDialog
{
    Q_OBJECT

public:
    /**
     * 构造函数
     */
    PluginsManagerWidget(QWidget *parent = nullptr);

    /**
     * 析构函数
     */
    virtual ~PluginsManagerWidget();

public:
    /**
     * 打开插件文件夹
     */
    void openPluginsFolder();
    /**
     * 初始化布局
     */
    void initLayout();
    /**
     * 获取插件文件夹路径
     */
    QString pluginsFolderPath() const;
    /**
     * 加载插件从文件夹
     */
    void loadPluginsFromFolder();
signals:
    /**
     * 加载插件状态
     * @param const QString &stauts 状态
     */
    void loadPluginStatus(const QString &stauts);
private:
    //插件文件夹
    QDir _pluginsFolder;
    //模型
    QStandardItemModel *_model = nullptr;
};
