//
// Created by 吴斌 on 2024/1/18.
//

#pragma once

#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QString>

class LogHandler;
class ExternalControler;
class TimeLineModel;
class ScheduledTaskModel;
class ScheduledTaskManager;
class DataflowViewsManger;
class CustomSplashScreen;
namespace NodeStudio { class NodeHttpServer; }

/**
 * @brief 无界面后台运行主窗口
 * 函数级注释：仅保留数据流执行、时间轴、计划任务、外部控制与 HTTP 服务等运行所需模块。
 */
class MainWindowHeadLess : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindowHeadLess(QWidget *parent = nullptr);
    ~MainWindowHeadLess() override;

Q_SIGNALS:
    void initStatus(const QString &message);

public Q_SLOTS:
    void init();
    bool loadFileFromPath(const QString &path);
    bool loadRecentFile();
    void restartAndOpenFlow(const QString& path);
    /** @brief 加载内建与第三方节点插件（无插件管理对话框） */
    void loadPlugins();
    void setSplashScreen(CustomSplashScreen* splash) { m_splash = splash; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    DataflowViewsManger* dataflowViewsManger = nullptr;
    TimeLineModel* timelineModel = nullptr;
    ScheduledTaskModel* scheduledTaskModel = nullptr;
    ScheduledTaskManager* scheduledTaskManager = nullptr;
    NodeStudio::NodeHttpServer* httpServer = nullptr;

    LogHandler* log = nullptr;
    ExternalControler* controller = nullptr;
    QString currentProjectPath;

    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayMenu = nullptr;
    QAction* trayExitAction = nullptr;
    CustomSplashScreen* m_splash = nullptr;

    void reportLoadStatus(const QString& message);
    void finishLoadSplash();
    void shutdown();

    bool isRestarting = false;
    bool shutdownDone_ = false;
};
