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
    PluginsManagerWidget(QWidget *parent = nullptr);

    virtual ~PluginsManagerWidget();

public:
    void openPluginsFolder();
    void initLayout();
    QString pluginsFolderPath() const;

    void loadPluginsFromFolder();
signals:
    void loadPluginStatus(const QString &stauts);
private:


private:
   
    QDir _pluginsFolder;

    QStandardItemModel *_model = nullptr;
};
