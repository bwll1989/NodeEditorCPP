//
// Created by 吴斌 on 2024/1/18.
//

#ifndef NODEEDITORCPP_MAINWINDOW_H
#define NODEEDITORCPP_MAINWINDOW_H
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QMainWindow>
#include <QMenuBar>
#include "Widget/CurveWidget/CurveEditor.h"
#include "Widget/ConsoleWidget/LogWidget.h"
#include "Widget/ConsoleWidget/LogHandler.h"
#include "DockManager.h"
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "Widget/NodeWidget/CustomFlowGraphicsScene.h"
#include "Widget/NodeWidget/CustomGraphicsView.h"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/MenuBarWidget/MenuBarWidget.h"
// #include "QtAwesome/QtAwesome.h"
#include "Widget/PortEditWidget/PortEditWidget.h"
#include "Widget/NodeLibraryWidget/draggabletreewidget.hpp"
#include "Widget/NodeLibraryWidget/NodeLibraryWidget.h"
#include "Widget/TimeLineWidget/timelinewidget.hpp"
#include "Widget/TimeLineWidget/TimelineWidget.hpp"
#include "Widget/ClipPropertyWidget/ClipPropertyWidget.hpp"
#include "Widget/NodeListWidget/NodeListWidget.hpp"
class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    MenuBarWidget *menuBar;
    ads::CDockManager* m_DockManager;
    CustomGraphicsView *view;
    CustomFlowGraphicsScene *scene;
    // 数据流模型
    CustomDataFlowGraphModel *dataFlowModel;
    PluginsManagerWidget *pluginsManagerDlg;
    // 端口编辑控件
    PortEditWidget *portEdit;
    // 节点库控件
    ads::CDockWidget *nodeDockLibraryWidget;
    bool isLocked= false;
    // 节点库
    NodeLibraryWidget *nodeLibrary;
    // 节点列表
    NodeListWidget *nodeListWidget;
    // 时间线
    TimelineWidget *timeline;
    // 片段属性
    ClipPropertyWidget *clipProperty;
signals:
    void initStatus(const QString &message);
public Q_SLOTS:
    static void showAboutDialog();
    void  initNodelist();
    void saveVisualState();
    void restoreVisualState();
    void locked_switch();
    void init();
    void loadFileFromPath(QString *path);
    void loadFileFromExplorer();
    void savFileToPath(const QString *path);
    void saveFileToExplorer();
protected:
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
private:
    // fa::QtAwesome *awesome;
    LogWidget *logTable;
    LogHandler *log;
};


#endif //NODEEDITORCPP_MAINWINDOW_H
