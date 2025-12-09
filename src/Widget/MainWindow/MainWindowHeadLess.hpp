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
#include "DockHub/DockHub.hpp"
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
// 新增：系统托盘支持
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindowHeadLess : public QMainWindow
{
Q_OBJECT
public:
    /**
     * @brief 构造函数(无头模式)
     * @param parent 父窗口
     * 函数级注释：无头模式下仍会构造必要的后台对象以保证功能不变。
     */
    explicit MainWindowHeadLess(QWidget *parent = nullptr);
    ~MainWindowHeadLess() override;
    //dock管理器
    ads::CDockManager* m_DockManager;
    //插件管理器
    PluginsManagerWidget *pluginsManagerDlg;
    // 节点库控件
    ads::CDockWidget *nodeDockLibraryWidget;
    // 节点库
    NodeLibraryWidget *nodeLibrary;
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
Q_SIGNALS:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
    void init();
    bool loadFileFromPath(const QString &path);
    bool loadRecentFile();
    /**
     * @brief 注入统一的启动画面对象
     * @param splash 由外部创建并管理生命周期的 CustomSplashScreen 指针
     * 函数级注释：使程序启动与文件加载共用同一个 SplashScreen；本类不负责关闭它。
     */
    void setSplashScreen(class CustomSplashScreen* splash) { m_splash = splash; }
protected:
    /**
     * @brief 关闭事件
     *
     * @param event 关闭事件指针
     */
    void closeEvent(QCloseEvent* event) override;
private:
    //日志表
    LogWidget *logTable;
    //日志处理器
    LogHandler *log;
    //当前项目路径
    QString currentProjectPath;
    //外部控制器
    ExternalControler *controller;

    // 新增：系统托盘相关成员
    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayMenu = nullptr;
    QAction* trayExitAction = nullptr;

    /**
     * 函数级注释：载入最近文件列表（从 cfg/RecentFiles.ini）
     */
    QStringList getRecentFiles() const;
    /**
     * @brief 判断是否已注入启动画面
     * @return true 表示拥有可用的 SplashScreen
     * 函数级注释：用于在加载流程中选择是否输出状态到启动画面。
     */
    bool hasSplashScreen() const { return m_splash != nullptr; }

protected:
    /**
     * @brief 外部统一管理的启动画面指针
     * 函数级注释：仅用于 updateStatus，不负责创建或销毁，也不在本类中 finish。
     */
    class CustomSplashScreen* m_splash = nullptr;
};

