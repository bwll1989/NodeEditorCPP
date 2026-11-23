//
// Created by 吴斌 on 2024/1/18.
//

#include "MainWindowHeadLess.hpp"

#include "Common/ProcessGuard/SpawnNoInherit.hpp"  // bInheritHandles=FALSE 派生（幽灵 socket 修复）

#include "ProjectSnapshotBuilder.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "Widget/ExternalControl/ExternalControler.hpp"
#include "Widget/ExternalControl/HttpServer.hpp"
#include "Widget/CalendarWidget/ScheduledTaskManager.hpp"
#include "Widget/CalendarWidget/ScheduledTaskModel.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
// 【2026-11-12】旧时间轴退役——headless 不再创建 TimeLineModel
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/AppConfig/ProjectPersistence.h"

// ---- 时间轴回放运行时（2026-11-23 headless 集成；与 MainWindow 同族）----
#include "../TimeLineWidget/QtTimelineModelAdapter.hpp"     // Scene↔帧模型适配器
#include "Widget/TimeLineWidget/MsRuntimeBridge.hpp"        // 下发运行时桥
#include "Widget/TimeLineWidget/TimelinePlaybackController.hpp" // 播放总控

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QThread>

static bool startHttpServerWithRetry(Flow::NodeHttpServer* server, int port, int waitMs)
{
    if (!server) {
        return false;
    }
    const int step = 200;
    int elapsed = 0;
    while (elapsed <= waitMs) {
        if (server->start(port)) {
            return true;
        }
        QThread::msleep(static_cast<unsigned long>(step));
        elapsed += step;
    }
    return false;
}

MainWindowHeadLess::MainWindowHeadLess(QWidget *parent)
    : QMainWindow(parent)
{
}

MainWindowHeadLess::~MainWindowHeadLess()
{
    shutdown();
    // 时间轴回放运行时拆卸：先控制器（内部发送调度线程持有 adapter/桥的
    // 借用指针——先停线程再拆依赖），桥/适配器虽挂 this 父子仍显式删除
    // 保证析构序；Scene 纯结构最后。
    delete msPlayback_;
    msPlayback_ = nullptr;
    delete msTimelineAdapter_;
    msTimelineAdapter_ = nullptr;
    delete msRuntime_;
    msRuntime_ = nullptr;
    delete msScene_;
    msScene_ = nullptr;
    delete controller;
    controller = nullptr;
    delete log;
    log = nullptr;
}

void MainWindowHeadLess::shutdown()
{
    if (shutdownDone_) {
        return;
    }
    shutdownDone_ = true;

    if (trayIcon) {
        trayIcon->hide();
    }

    if (httpServer && httpServer->running()) {
        httpServer->stop();
        QCoreApplication::processEvents();
    }

    if (dataflowViewsManger) {
        dataflowViewsManger->clearDataflow();
    }

    if (scheduledTaskManager) {
        scheduledTaskManager->stop();
    }

    ProjectPersistence::markCleanShutdown();
}

void MainWindowHeadLess::reportLoadStatus(const QString& message)
{
    emit initStatus(message);
    if (m_splash) {
        m_splash->updateStatus(message);
    }
    QApplication::processEvents();
}

void MainWindowHeadLess::finishLoadSplash()
{
    if (m_splash) {
        m_splash->finish(this);
    }
}

void MainWindowHeadLess::init()
{
    log = new LogHandler(nullptr);
    emit initStatus(tr("Initialization log success"));

    dataflowViewsManger = new DataflowViewsManger(nullptr, this, DataflowPresentationMode::ModelOnly);
    emit initStatus(tr("Initialization dataflow views success"));

    // 【2026-11-12】TimeLineModel 创建退役（旧时间轴下线）

    scheduledTaskModel = new ScheduledTaskModel(this);
    scheduledTaskManager = new ScheduledTaskManager(scheduledTaskModel, this);
    scheduledTaskManager->setToleranceSeconds(1);
    scheduledTaskManager->start(1000);
    emit initStatus(tr("Initialization scheduled task success"));

    controller = new ExternalControler();
    emit initStatus(tr("Initialization external controler success"));

    httpServer = new Flow::NodeHttpServer(this);
    const int port = ConfigManager::instance().getHttpServerPort();
    if (!startHttpServerWithRetry(httpServer, port, 3000)) {
        emit initStatus(tr("HTTP Server 启动失败，端口可能被占用: %1").arg(port));
    }
    connect(httpServer, &Flow::NodeHttpServer::flowFileUploaded, this, &MainWindowHeadLess::loadFileFromPath);
    emit initStatus(tr("Initialization Http Server success"));

    // ---- 时间轴回放运行时（headless 回放半边）--------------------------------
    // GUI 版对应装配 = MainWindow：Scene + 下发桥 + 适配器 + 播放总控；
    // 刻意不建预览渲染器/音频 bus/时间轴视图/画布（Deps 各指针允许 null）——
    // 显示端（FlowDisplay）是唯一画面消费方。运行时启动失败仅降级
    // （时间轴可加载、无下发能力），与 GUI 同语义。
    msScene_ = new ms::Scene();
    msScene_->canvas_w = 1920.0;
    msScene_->canvas_h = 1080.0;
    msRuntime_ = new ms::MsRuntimeBridge(this);
    if (!msRuntime_->start()) {
        qWarning() << "ms-headless: 下发运行时启动失败（端口占用？）——"
                      "时间轴可加载，display 下发不可用";
    }
    msTimelineAdapter_ = new ms::QtTimelineModelAdapter(this);
    msTimelineAdapter_->attachWorkScene(msScene_);
    msTimelineAdapter_->setRuntimeBridge(msRuntime_);
    // 编辑自动追平节流沿用全局配置（headless 无编辑，仅保持同参）
    msTimelineAdapter_->setAutoPublishInterval(
        ConfigManager::instance().getSceneAutoPublishMs());
    msPlayback_ = new ms::TimelinePlaybackController(
        {msRuntime_, nullptr, nullptr, msTimelineAdapter_, nullptr}, this);
    // 循环模式按配置（默认开：无人值守播完回零续播；运行中可经
    // /timeline/master/loop OSC 命令翻转——与 GUI 同源）
    msPlayback_->setLoop(ConfigManager::instance().isHeadlessTimelineLoop());
    emit initStatus(tr("Initialization timeline playback runtime success"));

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icons/FlowMono.png"));
        trayMenu = new QMenu(this);
        trayExitAction = trayMenu->addAction(tr("  退出  "));
        connect(trayExitAction, &QAction::triggered, this, []() {
            qApp->quit();
        });
        trayIcon->setContextMenu(trayMenu);
        trayIcon->show();
    }

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        shutdown();
    });
}

void MainWindowHeadLess::loadPlugins()
{
    PluginsManagerWidget::loadAllFromDefaultFolder([this](const QString& status) {
        emit initStatus(status);
    });
    // 插件就绪后再建根图，保证 Container 子图 registry 完整
    if (dataflowViewsManger)
        dataflowViewsManger->resetDataflow(QStringLiteral("dataflow"));
}

bool MainWindowHeadLess::loadFileFromPath(const QString &path)
{
    if (!currentProjectPath.isEmpty()) {
        restartAndOpenFlow(path);
        return true;
    }

    const auto reportStatus = [this](const QString& msg) {
        reportLoadStatus(msg);
    };

    if (path.isEmpty()) {
        reportLoadStatus(tr("Path is empty"));
        finishLoadSplash();
        return false;
    }

    const ProjectFileReadResult readResult = readProjectFile(path, reportStatus);
    if (!readResult.success) {
        finishLoadSplash();
        return false;
    }

    ProjectLoadTargets targets;
    targets.dataflowViewsManger = dataflowViewsManger;
    targets.scheduledTaskModel = scheduledTaskModel;
    targets.httpServer = httpServer;
    targets.msScene = msScene_;     // mediaTimeline 段回灌落点（GUI 同款）
    targets.msRuntime = msRuntime_; // mediaDisplays 段显示端登记回灌落点

    ProjectLoadOptions options;
    options.restoreVisualLayout = false;
    options.loadWebLayout = true;

    const auto dataflowProgress = [this](const QString& sceneTitle,
                                         const QString& phase,
                                         int current,
                                         int total) {
        if (total > 0) {
            const int pct = qBound(0, (current * 100) / total, 100);
            reportLoadStatus(tr("DataFlow [%1] %2 %3% (%4/%5)")
                                 .arg(sceneTitle, phase)
                                 .arg(pct)
                                 .arg(current)
                                 .arg(total));
        } else {
            reportLoadStatus(tr("DataFlow [%1] %2 %3").arg(sceneTitle, phase).arg(current));
        }
    };

    QString loadError;
    if (!loadProjectModulesFromJson(readResult.root,
                                    targets,
                                    options,
                                    reportStatus,
                                    dataflowProgress,
                                    &loadError)) {
        finishLoadSplash();
        return false;
    }

    ConfigManager::instance().addRecentFile(readResult.absolutePath);
    currentProjectPath = readResult.absolutePath;

    // —— 时间轴段回灌 + 自动下发（headless，2026-11-23 用户定案不自动播放）——
    // 与 GUI 打开工程同两段：装载模型 → 打开即追平（display 注册应答/
    // 上线追平有场景可发）。播放不自动驱动——由外部命令起播
    //（/timeline/master/play 等 OSC 命令，经播放总控与会话链同源生效）。
    // 旧文件无 mediaTimeline 段或纯数据流工程（场景全空）整块跳过——
    // 行为与旧版 headless 完全一致。
    if (msScene_ && msTimelineAdapter_
        && (!msScene_->timeline.tracks.empty() || !msScene_->screens.empty()
            || !msScene_->assets.empty())) {
        msTimelineAdapter_->loadSceneToModel(*msScene_);
        if (msTimelineAdapter_->publishScene()) {
            qDebug() << "ms-headless: 时间轴场景已下发（待外部命令起播）";
        } else {
            qWarning() << "ms-headless: 时间轴场景未下发（运行时未启动）";
        }
    }

    reportLoadStatus(tr("Load flow file completed"));
    finishLoadSplash();

    if (trayIcon) {
        trayIcon->setToolTip(tr("正在运行 %1").arg(QFileInfo(readResult.absolutePath).fileName()));
    }
    qDebug() << tr("正在运行 %1").arg(QFileInfo(readResult.absolutePath).fileName());

    return true;
}

void MainWindowHeadLess::restartAndOpenFlow(const QString& path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (abs.isEmpty() || !QFileInfo::exists(abs)) {
        return;
    }

    if (httpServer && httpServer->running()) {
        httpServer->stop();
        QCoreApplication::processEvents();
        QThread::msleep(200);
    }

    const QString exe = QCoreApplication::applicationFilePath();
    if (exe.isEmpty() || !QFileInfo::exists(exe)) {
        return;
    }

    // 【幽灵 socket 修复 2026-11-13】同 MainWindow::restartAndOpenFlow——
    // QProcess::startDetached 会继承全部句柄（UDP/HTTP 监听 socket 被
    // "幽灵化"劫持端口流量），改走 bInheritHandles=FALSE 派生。
    const QString rawArgs = QStringLiteral("--restart-delay-ms=200 \"%1\"")
                                .arg(QDir::toNativeSeparators(abs));
    const bool started = ms::spawnNoInherit(exe, rawArgs);
    if (!started) {
        emit initStatus(tr("重启打开失败：无法启动新进程"));
        return;
    }

    isRestarting = true;
    qApp->quit();
}

bool MainWindowHeadLess::loadRecentFile()
{
    const QStringList recentFiles = ConfigManager::instance().getRecentFiles();
    if (recentFiles.isEmpty()) {
        return false;
    }

    const QString recentFile = recentFiles.first();
    if (recentFile.isEmpty()) {
        return false;
    }
    return loadFileFromPath(recentFile);
}

void MainWindowHeadLess::closeEvent(QCloseEvent* event)
{
    if (isRestarting) {
        event->accept();
        return;
    }

    shutdown();
    qDebug() << "The program(Headless) exits manually";
    event->accept();
}
