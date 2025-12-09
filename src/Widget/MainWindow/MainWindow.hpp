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
    MenuBarWidget *menuBar;
    //dock管理器
    ads::CDockManager* m_DockManager;
    //插件管理器
    PluginsManagerWidget *pluginsManagerDlg;
    // 节点库控件
    ads::CDockWidget *nodeDockLibraryWidget;
    // // 节点设置控件
    // ads::CDockWidget *propertyDockWidget;
    // 锁定状态
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
Q_SIGNALS:
    //初始化状态信号
    void initStatus(const QString &message);
public Q_SLOTS:
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
    void loadFileFromPath(const QString &path);
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

    /**
     * @brief 从系统托盘恢复窗口
     * 函数级注释：托盘图标双击，显示主窗口并隐藏托盘图标。
     */
    void switchVisibleFromTray();

    /**
     * @brief 打开最近文件（菜单项触发）
     * 函数级注释：接收文件绝对路径，调用现有加载逻辑
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
     * 函数级注释：保存最近文件列表（至 cfg/RecentFiles.ini）
     */
    void saveRecentFiles(const QStringList& files) const;

    /**
     * 函数级注释：将新路径加入最近文件列表（去重、限制数量并更新菜单）
     */
    void addToRecentFiles(const QString& path);


};

