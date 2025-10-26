//
// Created by 吴斌 on 2024/1/18.
//

#pragma once
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QMainWindow>
#include <QMenuBar>
#include "Widget/ConsoleWidget/LogWidget.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "DockManager.h"
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "Widget/NodeWidget/CustomFlowGraphicsScene.h"
#include "Widget/NodeWidget/CustomGraphicsView.h"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/MenuBarWidget/MenuBarWidget.h"
#include "Widget/NodeLibraryWidget/NodeLibraryWidget.h"
#include "Widget/TimeLineWidget/TimeLineWidget.hpp"
#include "Widget/NodeListWidget/NodeListWidget.hpp"
#include "Widget/StageWidget/StageWidget.hpp"
#include "Widget/TimeLineWidget/TimeLineModel.h"
#include "Widget/ExternalControl/ExternalControler.hpp"
#include "Widget/CalendarWidget/ScheduledTaskWidget.hpp"
#include "Widget/MediaLibraryWidget/MediaLibraryWidget.h"
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
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
    //插件管理器
    PluginsManagerWidget *pluginsManagerDlg;
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
    TimeLineModel *timelineModel;
    // 舞台控件
    StageWidget *stageWidget;
    // osc 日历控件
    ScheduledTaskWidget *scheduledTaskWidget;
    // 媒体库
    MediaLibraryWidget *mediaLibraryWidget;
    //节点视图
    DataflowViewsManger *dataflowViewsManger;
signals:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
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
     * @brief 从路径加载文件
     *
     * @param path 文件路径
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
    /**
     * 创建数据流程视图
     */
    void createDataflowWidget();
    /**
     * 更新菜单栏视图action
     */
    void updateViewMenu(QMenu* menu);
protected:
    /**
     * @brief 关闭事件
     *
     * @param event 关闭事件指针
     */
    void closeEvent(QCloseEvent* event) override;
    /**
     * @brief 拖拽进入事件
     *
     * @param event 拖拽进入事件指针
     */
    void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * @brief 拖拽释放事件
     *
     * @param event 拖拽释放事件指针
     */
    void dropEvent(QDropEvent *event) override;
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

