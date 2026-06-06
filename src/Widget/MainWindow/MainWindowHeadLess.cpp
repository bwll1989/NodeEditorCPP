//
// Created by 吴斌 on 2024/1/18.
//

#include "MainWindowHeadLess.hpp"

#include "ProjectSnapshotBuilder.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "Widget/ExternalControl/ExternalControler.hpp"
#include "Widget/ExternalControl/HttpServer.hpp"
#include "Widget/CalendarWidget/ScheduledTaskManager.hpp"
#include "Widget/CalendarWidget/ScheduledTaskModel.hpp"
#include "Widget/PluginsMangerWidget/PluginsManagerWidget.hpp"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
#include "Widget/TimeLineWidget/TimeLineModel.h"
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/AppConfig/ProjectPersistence.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QThread>

static bool startHttpServerWithRetry(NodeStudio::NodeHttpServer* server, int port, int waitMs)
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
        dataflowViewsManger->clearAllScenes();
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

    timelineModel = new TimeLineModel(this);
    emit initStatus(tr("Initialization timeline success"));

    scheduledTaskModel = new ScheduledTaskModel(this);
    scheduledTaskManager = new ScheduledTaskManager(scheduledTaskModel, this);
    scheduledTaskManager->setToleranceSeconds(1);
    scheduledTaskManager->start(1000);
    emit initStatus(tr("Initialization scheduled task success"));

    controller = new ExternalControler();
    emit initStatus(tr("Initialization external controler success"));

    httpServer = new NodeStudio::NodeHttpServer(this);
    const int port = ConfigManager::instance().getHttpServerPort();
    if (!startHttpServerWithRetry(httpServer, port, 3000)) {
        emit initStatus(tr("HTTP Server 启动失败，端口可能被占用: %1").arg(port));
    }
    connect(httpServer, &NodeStudio::NodeHttpServer::flowFileUploaded, this, &MainWindowHeadLess::loadFileFromPath);
    emit initStatus(tr("Initialization Http Server success"));

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icons/NodeStudioRed.png"));
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
    targets.timelineModel = timelineModel;
    targets.scheduledTaskModel = scheduledTaskModel;
    targets.httpServer = httpServer;

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

    QStringList args;
    args << QStringLiteral("--restart-delay-ms=200");
    args << abs;

    const bool started = QProcess::startDetached(exe, args);
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
