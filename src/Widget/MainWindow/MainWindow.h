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
#include "QtAwesome/QtAwesome.h"
#include "Widget/PortEditWidget/PortEditWidget.h"
#include "Widget/NodeListWidget/draggabletreewidget.hpp"
#include "Widget/NodeListWidget/NodeListWidget.h"
#include "Timeline/timelinewidget.hpp"
#include "Widget/TimeLineWidget/TimeLineWidget.h"

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    MenuBarWidget *menuBar;
    ads::CDockManager* m_DockManager;
    CustomGraphicsView *view;
    CustomFlowGraphicsScene *scene;
    //    CustomDataFlowModel model= CustomDataFlowModel(registerDataModels());
    CustomDataFlowGraphModel *dataFlowModel;
    PluginsManagerWidget *pluginsManagerDlg;
    // PropertyWidget *property;
    PortEditWidget *portEdit;
    ads::CDockWidget *nodeListWidget;
    bool isLocked= false;
    NodeListWidget *nodeList;
    TimeLineWidget *timeline;
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
    fa::QtAwesome *awesome;
    LogWidget *logTable;
    LogHandler *log;
};


#endif //NODEEDITORCPP_MAINWINDOW_H
