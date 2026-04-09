//
// Created by 吴斌 on 2024/1/18.
//

#pragma once

#include <QMainWindow>
#include <QMetaObject>
#include "Widget/ConsoleWidget/LogWidget.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "DockHub/DockHub.hpp"
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
#include "../ExternalControl/HttpServer.hpp"
class PropertyWidget;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    /**
     * @brief 构造函数（支持无头模式）
     * @param parent 父窗口
     * @param headlessMode 是否启用无头模式（不显示任何界面）
     * 函数级注释：无头模式下仍会构造必要的后台对象以保证功能不变。
     */
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    //菜单栏
    MenuBarWidget *menuBar=nullptr;
    //dock管理器
    ads::CDockManager* m_DockManager=nullptr;
    //插件管理器
    PluginsManagerWidget *pluginsManagerDlg=nullptr;
    // 节点库控件
    ads::CDockWidget *nodeDockLibraryWidget=nullptr;
    // 属性面板 Dock
    ads::CDockWidget *propertyDockWidget=nullptr;
    PropertyWidget *propertyWidget=nullptr;
    QMetaObject::Connection propertySelectionConn;
    // 锁定状态
    bool isLocked= false;
    // 节点库
    NodeLibraryWidget *nodeLibrary=nullptr;
    // 节点列表
    NodeListWidget *nodeListWidget=nullptr;
    // 时间线
    TimelineWidget *timeline     = nullptr;
    // 时间线模型
    TimeLineModel *timelineModel=nullptr;
    // 舞台控件
    StageWidget *stageWidget=nullptr;
    // osc 日历控件
    ScheduledTaskWidget *scheduledTaskWidget=nullptr;
    // 媒体库
    MediaLibraryWidget *mediaLibraryWidget=nullptr;
    //节点视图
    DataflowViewsManger *dataflowViewsManger=nullptr;
    // http 服务器
    NodeStudio::NodeHttpServer *httpServer=nullptr;
Q_SIGNALS:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
    /**
     * @brief 初始化主窗口与各功能模块
     * 函数级注释：创建 DockManager 与各 DockWidget，初始化菜单栏、日志、节点编辑器、时间线、舞台、媒体库、外部控制与 HTTP 服务等。
     */
    void init();
    /**
     * 初始化节点列表
     */
    void initNodelist();
    /**
     * 更新默认可视化布局状态
     */
    void updateVisualState();
    /**
     * 恢复默认可视布局
     */
    void resetVisualState();
    /**
     * 锁定切换
     */
    void locked_switch();
    /**
     * 初始化正常模式，显示所有界面
     */
    /**
     * 保存文件到资源管理器
     */
    void saveFileToExplorer();
    /**
     * @brief 从路径加载文件
     *
     * @param path 文件路径
     */
    /**
     * @brief 从路径加载 .flow 项目文件
     * @param path 文件路径
     * 函数级注释：启动阶段（尚未加载任何项目）在当前进程中完成加载；
     *            若已加载过项目，则不在当前进程卸载并重建节点，而是触发重启打开新项目，避免第三方节点质量问题导致的内存持续增长。
     */
    void loadFileFromPath(const QString &path);
    /**
     * 从资源管理器加载文件
     */
    /**
     * @brief 通过文件对话框选择并打开 .flow 文件
     * 函数级注释：为避免运行时切换项目带来的内存风险，该入口会走“重启打开”。
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

    /**
     * @brief 从系统托盘恢复窗口
     * 函数级注释：托盘图标双击，显示主窗口并隐藏托盘图标。
     */
    void switchVisibleFromTray();


    /**
     * @brief 打开最近文件（菜单项触发）
     * @param path 最近文件路径
     * 函数级注释：统一走“重启打开”，保证与拖拽/文件对话框行为一致。
     */
    void openRecentFile(const QString& path);

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
    /**
     * 添加菜单项
     * @param parent
     * @param actions
     * @return
     */
    QMenu* makeOptionsMenu(QWidget* parent, const QList<QAction*>& actions);

    void switchTheme(bool isDark);

    /**
     * @brief 重启进程并在新进程中打开指定 .flow 文件
     * @param path 目标项目文件路径
     * 函数级注释：启动新进程后退出当前进程；重启时跳过托盘最小化确认弹窗；
     *            并在重启前尽量释放 HTTP 端口（若启用）以保证网页可访问。
     */
    void restartAndOpenFlow(const QString& path);

private:
    //日志表
    LogWidget *logTable;
    //日志处理器
    LogHandler *log;
    // 当前项目路径：非空表示当前进程已加载过项目（用于判断是否需要走重启打开）
    QString currentProjectPath;
    //外部控制器
    ExternalControler *controller;

    // 新增：系统托盘相关成员
    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayMenu = nullptr;
    QAction* trayExitAction = nullptr;
    bool isDarkTheme = false;
    // 标记当前是否处于“自重启退出”流程；用于绕过 closeEvent 中的“是否最小化到托盘”弹窗
    bool isRestarting = false;

  

};

