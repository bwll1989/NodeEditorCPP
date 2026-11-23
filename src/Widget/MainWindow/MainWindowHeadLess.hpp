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
class ScheduledTaskModel;
class ScheduledTaskManager;
class DataflowViewsManger;
class CustomSplashScreen;
namespace Flow { class NodeHttpServer; }
namespace ms {
struct Scene;                   // 时间轴权威场景（model/SceneModel.hpp；指针成员）
class MsRuntimeBridge;          // 下发运行时桥（EditorRuntime PIMPL 壳）
class QtTimelineModelAdapter;   // Scene↔帧模型适配器（兼下发编排）
class TimelinePlaybackController; // 播放总控（headless 无视图装配）
}

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
    // 【2026-11-12】TimeLineModel 成员退役（旧时间轴下线）
    ScheduledTaskModel* scheduledTaskModel = nullptr;
    ScheduledTaskManager* scheduledTaskManager = nullptr;
    Flow::NodeHttpServer* httpServer = nullptr;

    LogHandler* log = nullptr;
    ExternalControler* controller = nullptr;
    QString currentProjectPath;

    // ---- 时间轴回放运行时（2026-11-23 headless 集成）------------------------
    // 无界面回放半边（GUI 版对应物 = MainWindow 的 ms* 成员族）：
    // Scene（mediaTimeline 段回灌落点）+ 下发桥（mediaDisplays 段登记 +
    // 场景/素材/锚点命令）+ 适配器（装载/下发编排）+ 播放总控（加载即
    // 下发"打开即追平"；播放不自动——由外部 OSC 命令经控制器同源起播）。
    // 刻意不建：预览渲染器、音频 bus、时间轴视图、画布——Deps 各指针
    // 允许 null，显示端是唯一消费方。
    ms::Scene* msScene_ = nullptr;
    ms::MsRuntimeBridge* msRuntime_ = nullptr;
    ms::QtTimelineModelAdapter* msTimelineAdapter_ = nullptr;
    ms::TimelinePlaybackController* msPlayback_ = nullptr;

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
