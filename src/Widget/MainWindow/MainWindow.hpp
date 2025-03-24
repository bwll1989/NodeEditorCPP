//
// Created by 吴斌 on 2024/1/18.
//

#ifndef NODEEDITORCPP_MAINWINDOW_HPP
#define NODEEDITORCPP_MAINWINDOW_HPP
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QMainWindow>
#include <QMenuBar>
#include "Widget/CurveWidget/CurveEditor.h"
#include "Widget/ConsoleWidget/LogWidget.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
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
// #include "Widget/ClipPropertyWidget/ClipPropertyWidget.hpp"
#include "Widget/NodeListWidget/NodeListWidget.hpp"
#include "Widget/StageWidget/stagewidget.hpp"
#include "Widget/TimeLineWidget/TimelineMVC/timelinemodel.hpp"
#include "Widget/ExternalControl/ExternalControler.hpp"
class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    //菜单栏
    MenuBarWidget *menuBar;
    //dock管理器
    ads::CDockManager* m_DockManager;
    //节点视图
    CustomGraphicsView *view;
    //节点场景
    CustomFlowGraphicsScene *scene;
    // 数据流模型
    CustomDataFlowGraphModel *dataFlowModel;
    //插件管理器
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
    // 时间线模型
    TimelineModel *timelineModel;
    // 片段属性
    // ClipPropertyWidget *clipProperty;
    // 舞台控件
    StageWidget *stageWidget;
signals:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
    /**
     * 显示关于对话框
     */
    static void showAboutDialog();
    /**
     * 初始化节点列表
     */
    void  initNodelist();
    /**
     * 保存可视状态
     */
    void saveVisualState();
    /**
     * 恢复可视状态
     */
    void restoreVisualState();
    /**
     * 锁定切换
     */
    void locked_switch();
    /**
     * 初始化
     */
    void init();
    /**
     * 保存文件到资源管理器
     */
    void saveFileToExplorer();
    /**
     * 从路径加载文件
     */
    void loadFileFromPath(QString *path);
    /**
     * 从资源管理器加载文件
     */
    void loadFileFromExplorer();
    /**
     * 保存文件到路径
     */
    void saveFileToPath();
   
protected:
    /**
     * 关闭事件
     */
    virtual void closeEvent(QCloseEvent* event) override;
    /**
     * 拖拽进入事件
     */
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * 拖拽释放事件
     */
    virtual void dropEvent(QDropEvent *event) override;
private:
    //日志表
    LogWidget *logTable;
    //日志处理器
    LogHandler *log;
    //当前项目路径
    QString currentProjectPath;
    //外部控制器
    ExternalControler *controller;
};


#endif //NODEEDITORCPP_MAINWINDOW_HPP
