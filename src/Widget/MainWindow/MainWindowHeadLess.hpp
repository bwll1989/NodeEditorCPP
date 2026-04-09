//
// Created by 吴斌 on 2024/1/18.
//

#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include "Widget/ConsoleWidget/LogWidget.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "DockHub/DockHub.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/NodeLibraryWidget/NodeLibraryWidget.h"
#include "Widget/TimeLineWidget/TimeLineWidget.hpp"
#include "Widget/StageWidget/StageWidget.hpp"
#include "Widget/TimeLineWidget/TimeLineModel.h"
#include "Widget/ExternalControl/ExternalControler.hpp"
#include "Widget/CalendarWidget/ScheduledTaskWidget.hpp"
#include "Widget/MediaLibraryWidget/MediaLibraryWidget.h"
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
// 新增：系统托盘支持
#include <QSystemTrayIcon>
#include <QMenu>
#include "../ExternalControl/HttpServer.hpp"
class MainWindowHeadLess : public QMainWindow
{
Q_OBJECT
public:
    /**
     * @brief 构造函数（无头模式）
     * @param parent 父窗口
     * 函数级注释：无头模式下不展示主界面，但仍会构建数据流/时间线/外部控制/HTTP 服务等后台对象。
     *            该模式主要用于常驻后台运行、提供网页控制与自动化能力。
     */
    explicit MainWindowHeadLess(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     * 函数级注释：释放无头模式下创建的后台对象，确保退出时资源正确回收。
     */
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
 	// http 服务器
    NodeStudio::NodeHttpServer *httpServer=nullptr;
Q_SIGNALS:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
    /**
     * @brief 初始化无头主窗口依赖的后台模块
     * 函数级注释：创建 DockManager / 日志 / DataflowViewsManger / 时间线与舞台 / 计划任务 / 外部控制器 / HTTP 服务器 / 托盘等。
     *            无头模式不展示 Dock 的 UI，但仍复用其数据与逻辑。
     */
    void init();

    /**
     * @brief 从指定路径加载 .flow 项目文件
     * @param path .flow 文件路径
     * @return true 加载成功；false 加载失败
     * 函数级注释：若当前进程已加载过项目（currentProjectPath 非空），则不在本进程切换项目，
     *            而是触发 restartAndOpenFlow 以“重启打开新项目”，避免第三方节点卸载不彻底导致的内存增长。
     */
    bool loadFileFromPath(const QString &path);

    /**
     * @brief 加载最近一次打开的项目文件
     * @return true 加载成功；false 加载失败
     * 函数级注释：用于启动阶段未指定文件参数时的兜底加载逻辑。
     */
    bool loadRecentFile();

    /**
     * @brief 重启进程并在新进程中打开指定 flow 文件
     * @param path flow 文件路径
     * 函数级注释：先尽量停止 HTTP 服务释放端口，再以命令行参数启动新进程打开目标文件，然后退出当前进程。
     */
    void restartAndOpenFlow(const QString& path);

    /**
     * @brief 注入统一的启动画面对象
     * @param splash 由外部创建并管理生命周期的 CustomSplashScreen 指针
     * 函数级注释：用于将无头模式的初始化/加载状态输出到外部 Splash；本类不负责创建/销毁/finish。
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
    // 日志表：用于在无头模式下承载日志显示与调试输出
    LogWidget *logTable;
    // 日志处理器：将运行信息写入日志表
    LogHandler *log;
    // 当前项目路径：非空表示本进程已经加载过一个项目（后续再打开新项目将走“重启打开”）
    QString currentProjectPath;
    // 外部控制器：承载 OSC / 外部控制等逻辑
    ExternalControler *controller;

    // 新增：系统托盘相关成员
    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayMenu = nullptr;
    QAction* trayExitAction = nullptr;

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

