//
// Created by 吴斌 on 2024/1/18.
//

#include <QMessageBox>
#include "MainWindow.hpp"
#include "Common/AppConfig/ConstantDefines.h"
#include "Nodes/NodeEditorStyle.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "QFile"
#include"Widget/ConsoleWidget/LogWidget.hpp"
#include <QDropEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMimeData>

#include <QWKWidgets/widgetwindowagent.h>
#include <widgetframe/windowbar.h>
#include <widgetframe/windowbutton.h>
// #include "Widget/PropertyWidget/PropertyWidget.hpp"
#include <QSettings>
#include <exception>
#include "../../Common/AppConfig/AutosaveManager.h"
#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/AppConfig/ProjectPersistence.h"
#include "ProjectSnapshotBuilder.hpp"
#include <QMetaObject>
#include <QTimer>
using namespace ads;

/**
 * @brief 启动 HTTP 服务器并在端口占用的短窗口期内重试
 * @param server HTTP 服务器对象
 * @param port 监听端口
 * @param waitMs 最大等待时间（毫秒）
 * @return true 启动成功；false 启动失败
 * 函数级注释：用于解决“重启切换项目后端口尚未完全释放”导致网页不可访问的问题。
 */
static bool startHttpServerWithRetry(Flow::NodeHttpServer* server, int port, int waitMs)
{
    if (!server) return false;
    const int step = 200;
    int elapsed = 0;
    while (elapsed <= waitMs) {
        if (server->start(port)) return true;
        QThread::msleep(static_cast<unsigned long>(step));
        elapsed += step;
    }
    return false;
}

/** @brief 获取应用图标（优先 QApplication，回退到 Flow 资源） */
static QIcon applicationIcon()
{
    if (!QApplication::windowIcon().isNull()) {
        return QApplication::windowIcon();
    }
    return QIcon(QStringLiteral(":/icons/icons/Flow.png"));
}

/**
 * @brief 手动模拟控件的鼠标离开事件，清除 hover 高亮状态
 *
 * 背景：点击标题栏「最大化/还原」按钮时，会先触发 QPushButton::clicked，再改变窗口
 *       状态（showMaximized / showNormal）。Qt 在此场景下有时不会向按钮发送
 *       QEvent::Leave / QHoverEvent::HoverLeave，导致按钮仍显示 :hover 样式，
 *       直到用户再次移动鼠标。该问题在无边框自定义标题栏中较常见。
 *
 * 做法：在窗口状态切换后的下一事件循环（singleShot(0)）检查光标是否已不在控件
 *       区域内；若是，则向控件投递 Leave 与 HoverLeave 事件，强制结束 hover 态。
 *
 * @param widget 需要清除 hover 状态的控件（当前用于最大化按钮 maxButton）
 */
static void emulateLeaveEvent(QWidget *widget)
{
    if (!widget) {
        return;
    }
    // 延迟到下一事件循环：等窗口几何/状态更新完成后再判断光标位置
    QTimer::singleShot(0, widget, [widget]() {
        const QScreen *screen = widget->screen();
        const QPoint globalPos = QCursor::pos(screen);
        // 光标已离开控件区域时，才需要补发离开事件
        if (!QRect(widget->mapToGlobal(QPoint{0, 0}), widget->size()).contains(globalPos)) {
            QCoreApplication::postEvent(widget, new QEvent(QEvent::Leave));
            if (widget->testAttribute(Qt::WA_Hover)) {
                const QPoint scenePos = widget->window()->mapFromGlobal(globalPos);
                static constexpr const auto oldPos = QPoint{};
                const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
                const auto event =
                    new QHoverEvent(QEvent::HoverLeave, scenePos, globalPos, oldPos, modifiers);
                QCoreApplication::postEvent(widget, event);
            }
        }
    });
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    // QWindowKit：尽早 setup，以便接管无边框窗口的尺寸与原生行为
    windowAgent = new QWK::WidgetWindowAgent(this);
    windowAgent->setup(this);
    setWindowTitle(AppConstants::PRODUCT_NAME);
    // 同步应用/窗口图标，供标题栏左上角与任务栏使用
    const QIcon appIcon = applicationIcon();
    QApplication::setWindowIcon(appIcon);
    setWindowIcon(appIcon);

    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    //    聚焦高亮
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, true);
    //    每个区域都有独立的关闭按钮
    ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, false);
    //
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);

    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);

    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton, true);
}

MainWindow::~MainWindow()
{
    if (httpServer) {
        if (httpServer->running()) {
            httpServer->stop();
        }
        delete httpServer;
        httpServer = nullptr;
    }
    delete controller;
    delete timeline;
}

void MainWindow::init()
{
    m_DockManager = new ads::CDockManager(this);
    // 注册到 DockHub，供外部模块统一挂载
    DockHub::instance().setDockManager(m_DockManager);

    m_DockManager->setDockWidgetToolBarStyle(Qt::ToolButtonStyle::ToolButtonIconOnly,CDockWidget::eState::StateDocked);
    this->setCentralWidget(m_DockManager);
    emit initStatus("Initialization ADS success");
     // 菜单条控件
    menuBar=new MenuBarWidget(this);
    connect(menuBar, &MenuBarWidget::recentFileTriggered,
             this, &MainWindow::openRecentFile);
     // 初始化时刷新最近文件菜单
    menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
    setupFramelessWindow();
    emit initStatus("Initialization MenuBar success");
    // 首先实例化终端显示控件，保证日志输出正常
    auto *logDockViewer= m_DockManager->createDockWidget("终端显示");
    logTable=new LogWidget();
    log=new LogHandler(logTable);
    logDockViewer->setWidget(logTable);
    logDockViewer->setIcon(QIcon(":/icons/icons/bug.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, logDockViewer);
    //添加菜单项
    QMenu* OptionsMenu = makeOptionsMenu(logTable, logTable->getActions());
    logDockViewer->setTitleBarActions({OptionsMenu->menuAction()});
    emit initStatus("Initialization console widget");
   
    // 按配置应用主题（重启后生效）
    applyTheme(ConfigManager::instance().isDefaultDarkTheme());
    // 节点编辑：由 DataflowViewsManger 创建单画布 Dock
    emit initStatus("load node editor style success");
    dataflowViewsManger=new DataflowViewsManger(m_DockManager,this);
    // 插件需先注册再创建根图（否则 Container/内置节点不在 registry）
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager success");
    dataflowViewsManger->resetDataflow(QStringLiteral("dataflow"));
    emit initStatus("Init node editor success");
    // 节点库控件
    // nodeDockLibraryWidget = m_DockManager->createDockWidget("节点库");
    // nodeDockLibraryWidget->setIcon(QIcon(":/icons/icons/library.png"));
    // m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeDockLibraryWidget);
    // emit initStatus("Initialization nodeLibrary success");
    // 时间轴控件
    // 创建时间线模型
    timelineModel = new TimeLineModel();
    // 创建时间线部件
    timeline = new TimelineWidget(timelineModel);
    // 创建时间轴控件
    auto *timelineDockWidget = m_DockManager->createDockWidget("时间轴");
    timelineDockWidget->setObjectName("timeline");
    timelineDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    // timeline=new timelinewidget(timelineModel);
    timelineDockWidget->setWidget(timeline);
    // 添加到左侧区域
     m_DockManager->addDockWidget(ads::LeftDockWidgetArea,  timelineDockWidget);
     // 添加到菜单栏
     emit initStatus("Initialization Timeline editor success");
    auto *calendarDockWidget = m_DockManager->createDockWidget("计划任务");
    calendarDockWidget->setObjectName("scheduled");
    calendarDockWidget->setIcon(QIcon(":/icons/icons/scheduled.png"));
    scheduledTaskWidget=new ScheduledTaskWidget();
    calendarDockWidget->setWidget(scheduledTaskWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, calendarDockWidget);
    // QMenu* Options = makeOptionsMenu(scheduledTaskWidget, scheduledTaskWidget->getActions());
    calendarDockWidget->setTitleBarActions({makeOptionsMenu(scheduledTaskWidget, scheduledTaskWidget->getActions())->menuAction()});
    emit initStatus("Initialization Scheduled Task success");
    // 媒体库控件
    auto *mediaLibraryDockWidget = m_DockManager->createDockWidget("媒体库");
    mediaLibraryDockWidget->setObjectName("mediaLibrary");
    mediaLibraryDockWidget->setIcon(QIcon(":/icons/icons/media_mange.png"));
    mediaLibraryWidget = new MediaLibraryWidget();
    mediaLibraryDockWidget->setWidget(mediaLibraryWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, mediaLibraryDockWidget);
    // menuBar->views->addAction(mediaLibraryDockWidget->toggleViewAction());
    mediaLibraryDockWidget->setTitleBarActions({makeOptionsMenu(mediaLibraryWidget, mediaLibraryWidget->getActions())->menuAction()});
    emit initStatus("Initialization Media Library Widget success");
    // propertyDockWidget = m_DockManager->createDockWidget("节点属性");
    // propertyDockWidget->setObjectName("property");
    // propertyDockWidget->setIcon(QIcon(":/icons/icons/property.png"));
    // propertyWidget = new PropertyWidget(nullptr, propertyDockWidget);
    // propertyDockWidget->setWidget(propertyWidget);
    // m_DockManager->addDockWidget(ads::RightDockWidgetArea, propertyDockWidget);
    // emit initStatus("Initialization Property Widget success");

    // connect(dataflowViewsManger, &DataflowViewsManger::sceneIsActive, this, [this](const QString& title) {
    //     if (!propertyWidget || !dataflowViewsManger) return;
    //
    //     if (propertySelectionConn) {
    //         QObject::disconnect(propertySelectionConn);
    //         propertySelectionConn = QMetaObject::Connection();
    //     }
    //
    //     auto* model = dataflowViewsManger->modelByTitle(title);
    //     propertyWidget->setModel(model);
    //
    //     auto* scene = dataflowViewsManger->sceneByTitle(title);
    //     if (!scene) {
    //         propertyWidget->update(QtNodes::InvalidNodeId);
    //         return;
    //     }
    //
    //     propertySelectionConn = QObject::connect(scene, &QGraphicsScene::selectionChanged, this, [this, title]() {
        //     if (!propertyWidget || !dataflowViewsManger) return;
        //     auto* s = dataflowViewsManger->sceneByTitle(title);
        //     if (!s) return;
        //     const auto nodes = s->selectedNodes();
        //     const QtNodes::NodeId id = nodes.empty() ? QtNodes::InvalidNodeId : nodes.front();
        //     propertyWidget->update(id);
        // });

    //     const auto nodes = scene->selectedNodes();
    //     const QtNodes::NodeId id = nodes.empty() ? QtNodes::InvalidNodeId : nodes.front();
    //     propertyWidget->update(id);
    // });
    // 外部控制器
    controller=new ExternalControler();

    emit initStatus("Initialization external controler success");
	 // http 服务器
    httpServer=new Flow::NodeHttpServer();
    //http服务器文件上传后，直接打开（若已加载过项目，则自动走重启打开）
    connect(httpServer, &Flow::NodeHttpServer::flowFileUploaded, this, &MainWindow::loadFileFromPath);
    const int port = ConfigManager::instance().getHttpServerPort();
    if (!startHttpServerWithRetry(httpServer, port, 3000)) {
        emit initStatus(tr("HTTP Server 启动失败，端口可能被占用: %1").arg(port));
    } else {
        emit initStatus("Initialization Http Server success");
    }
    if (dataflowViewsManger) {
        dataflowViewsManger->setHttpServer(httpServer);
    }
    // 更新默认布局
    connect(menuBar->saveLayout, &QAction::triggered, this, &MainWindow::updateVisualState);
    //恢复布局
    connect(menuBar->restoreLayout, &QAction::triggered, this, &MainWindow::resetVisualState);
    //恢复布局
    connect(menuBar->saveAsAction, &QAction::triggered, this, &MainWindow::saveFileToExplorer);
    //另存为
    connect(menuBar->saveAction, &QAction::triggered, this, &MainWindow::saveFileToPath);
    //保存
    connect(menuBar->loadAction, &QAction::triggered, this, &MainWindow::loadFileFromExplorer);
    // 打开
    connect(menuBar->exitAction, &QAction::triggered, this, &MainWindow::close);
    // 退出
    connect(menuBar->lockAction, &QAction::triggered,this, &MainWindow::locked_switch);
    //锁定切换
    connect(menuBar->pluginsManagerAction, &QAction::triggered, pluginsManagerDlg,
            &PluginsManagerWidget::exec);
    // 显示插件管理器
    connect(menuBar->pluginsFloderAction, &QAction::triggered, pluginsManagerDlg,&PluginsManagerWidget::openPluginsFolder);
    // 刷新菜单栏视图选项
    connect(menuBar->views, &QMenu::aboutToShow, this, [this]() {
        updateViewMenu(menuBar->views);
    });
    //清空数据流
    connect(menuBar->Clear_dataflow, &QAction::triggered, this, [this]() {
        dataflowViewsManger->resetDataflow();
    });
    //    日志清空功能
    connect(menuBar->clearAction, &QAction::triggered, logTable, &LogWidget::clearTableWidget);

    setAcceptDrops(true);

     if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icons/Flow.png"));
        trayIcon->setToolTip(tr("Flow"));
        // 托盘菜单
        trayMenu = new QMenu(this);
        trayMenu->setWindowFlags(trayMenu->windowFlags() | Qt::NoDropShadowWindowHint);
        trayMenu->setAttribute(Qt::WA_TranslucentBackground, false);
        trayExitAction    = trayMenu->addAction(tr("  退出  "));
        connect(trayExitAction,    &QAction::triggered, this, [this]() {
            // 退出前保存布局
            // saveVisualState();
            qApp->quit();
        });
        trayIcon->setContextMenu(trayMenu);

        // 双击/单击托盘图标时还原
        connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick) {
                switchVisibleFromTray();
            }
        });
        trayIcon->show();
        // 初始就显示图标，否则容易与windows任务栏显示机制有冲突，导致图标第二次无法正常显示
        //启动时加载默认布局
        resetVisualState();

    }

    setupAutosave();
}

void MainWindow::switchVisibleFromTray()
{
    if (this->isHidden()) {
        this->showMaximized();
        this->raise();
        this->activateWindow();
        syncFramelessWindowState();
    }else {
        this->hide();
    }
}

/**
 * @brief 重启进程并在新进程中打开指定项目文件
 * @param path 目标 .flow 文件路径
 * 函数级注释：
 * - 先尽量停止 HTTP Server 释放端口，避免新进程启动后网页服务端口被占用
 * - 再通过 startDetached 启动新进程，并携带 --restart-delay-ms 等待旧进程退出
 * - 置位 isRestarting，确保 closeEvent 不弹出“是否最小化到托盘”的确认框
 * - 最后退出当前进程
 */
void MainWindow::restartAndOpenFlow(const QString& path)
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

    if (autosaveManager) {
        autosaveManager->clearRecovery();
    }

    const QString exe = QCoreApplication::applicationFilePath();
    if (exe.isEmpty() || !QFileInfo::exists(exe)) {
        return;
    }

    QStringList args;
    args << "--restart-delay-ms=500";
    args << abs;

    const bool started = QProcess::startDetached(exe, args);
    if (!started) {
        QMessageBox::warning(this, "", tr("重启打开失败：无法启动新进程"));
        return;
    }

    // 新进程在 main() 入口即读取 LastShutdownClean，必须在 startDetached 成功后、
    // 旧进程 aboutToQuit 之前同步写入，否则会被误判为异常退出并弹出恢复对话框。
    ProjectPersistence::markCleanShutdown();

    isRestarting = true;
    QCoreApplication::quit();
}

/**
 * @brief 打开最近文件（菜单项触发）
 * @param path 最近文件路径
 * 函数级注释：统一走重启打开，避免运行时直接切换项目导致内存持续增长。
 */
void MainWindow::openRecentFile(const QString& path)
{
    restartAndOpenFlow(path);
}

//显示属性
// void MainWindow::initNodelist() {
//     nodeLibrary=new NodeLibraryWidget();
//     this->nodeDockLibraryWidget->setWidget(nodeLibrary);
//     emit initStatus("Initialization nodes library success");
// }

// 锁定切换
void MainWindow::locked_switch() {
    isLocked=!isLocked;
    dataflowViewsManger->setDataflowLocked(isLocked);
}
//拖拽进入
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls() && mimeData->urls().size() == 1 && !isLocked) {
        //        锁定状态下无法拖拽
        QString filePath = mimeData->urls().first().toLocalFile();
        QFileInfo fileInfo(filePath);
        if (fileInfo.suffix().toLower() == "flow") {
            //            文件后缀符合，才接收拖拽
            event->acceptProposedAction();
        }
    }
}
/**
 * 拖拽事件
 * 接收拖拽的文件路径，检查是否为 .flow 文件，若符合则清空场景并加载文件。
 */
void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    const QString filePath = mimeData->urls().at(0).toLocalFile();
    if (!QFileInfo::exists(filePath))
        return;

    restartAndOpenFlow(filePath);
    event->acceptProposedAction();

}
/**
 * 从路径打开文件（仅显示状态文字）
 * 通过 initStatus 分阶段展示状态文本（启动阶段由 main.cpp 的启动界面承接）。
 */
void MainWindow::loadFileFromPath(const QString &path)
{
    if (!currentProjectPath.isEmpty()) {
        restartAndOpenFlow(path);
        return;
    }

    ProjectLoadResolution resolution;
    if (forcedLoadResolution_) {
        resolution = *forcedLoadResolution_;
        forcedLoadResolution_.reset();
    } else if (skipRecoveryPrompt_) {
        resolution.loadPath = path;
        resolution.projectPath = path;
    } else {
        resolution = resolveRecoveryLoadPath(path);
    }

    const QString loadPath = resolution.loadPath.isEmpty() ? path : resolution.loadPath;

    QString logicalProjectPath = ProjectPersistence::normalizeProjectPath(resolution.projectPath);
    if (logicalProjectPath.isEmpty() && resolution.fromRecovery) {
        logicalProjectPath = ProjectPersistence::normalizeProjectPath(
            ProjectPersistence::readRecovery().projectPath);
    }
    if (logicalProjectPath.isEmpty()) {
        const QString normalizedArg = ProjectPersistence::normalizeProjectPath(path);
        if (!normalizedArg.isEmpty() && !ProjectPersistence::isAutosavePath(normalizedArg)) {
            logicalProjectPath = normalizedArg;
        }
    }

    struct SplashProxy {
        MainWindow* self = nullptr;
        void updateStatus(const QString& msg) {
            if (self) Q_EMIT self->initStatus(msg);
            QApplication::processEvents();
        }
        void finish(QWidget*) {}
    };

    SplashProxy splashScreen{ this };

    const auto pumpUi = []() {
        QApplication::processEvents();
    };

    const auto reportStatus = [&splashScreen, &pumpUi](const QString& msg) {
        splashScreen.updateStatus(msg);
        pumpUi();
    };

    if (loadPath.isEmpty()) {
        reportStatus(tr("Path is empty"));
        return;
    }

    const ProjectFileReadResult readResult = readProjectFile(loadPath, reportStatus);
    if (!readResult.success) {
        QMessageBox::warning(this, "", readResult.errorMessage);
        return;
    }

    if (autosaveManager) {
        autosaveManager->setPaused(true);
    }

    const auto abortLoad = [&](const QString& msg) {
        if (autosaveManager) {
            autosaveManager->setPaused(false);
        }
        reportStatus(msg);
        QMessageBox::warning(this, "", msg);
    };

    ProjectLoadTargets targets;
    targets.dataflowViewsManger = dataflowViewsManger;
    targets.timeline = timeline;
    targets.scheduledTaskWidget = scheduledTaskWidget;
    targets.dockManager = m_DockManager;
    targets.httpServer = httpServer;

    ProjectLoadOptions options;
    options.restoreVisualLayout = true;
    options.loadWebLayout = true;
    options.onMissingVisualLayout = [this]() { resetVisualState(); };

    const auto dataflowProgress = [&splashScreen, &pumpUi](const QString& sceneTitle,
                                                           const QString& phase,
                                                           int current,
                                                           int total) {
        if (total > 0) {
            const int pct = qBound(0, (current * 100) / total, 100);
            splashScreen.updateStatus(QObject::tr("DataFlow [%1] %2 %3% (%4/%5)")
                                         .arg(sceneTitle, phase)
                                         .arg(pct)
                                         .arg(current)
                                         .arg(total));
        } else {
            splashScreen.updateStatus(QObject::tr("DataFlow [%1] %2 %3")
                                         .arg(sceneTitle, phase)
                                         .arg(current));
        }
        pumpUi();
    };

    QString loadError;
    if (!loadProjectModulesFromJson(readResult.root,
                                    targets,
                                    options,
                                    reportStatus,
                                    dataflowProgress,
                                    &loadError)) {
        abortLoad(loadError);
        return;
    }

    const QString normalizedLogicalPath = logicalProjectPath;
    if (!normalizedLogicalPath.isEmpty()) {
        ConfigManager::instance().addRecentFile(normalizedLogicalPath);
        menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
        onProjectLoaded(normalizedLogicalPath);
        this->setWindowTitle(QFileInfo(normalizedLogicalPath).fileName());
    } else {
        onProjectLoaded(QString());
        this->setWindowTitle(tr("未命名项目"));
    }

    if (resolution.fromRecovery) {
        reportStatus(tr("Recovered from autosave"));
    }
    reportStatus(tr("Load flow file completed"));
}

/**
 * @brief 应用启动前已确认的加载/恢复结果
 * @param resolution 启动前 resolveProjectLoadPath 的解析结果（含是否从 recovery 恢复）
 * @param cmdFilePath 命令行传入的 .flow 路径，可为空
 * 函数级注释：在 Splash 初始化完成后调用；若用户选择恢复自动保存，则加载 recovery 文件；
 *            否则按命令行路径打开，并跳过重复的恢复弹窗。
 */
void MainWindow::applyStartupLoadResolution(const ProjectLoadResolution& resolution,
                                            const QString& cmdFilePath)
{
    if (!currentProjectPath.isEmpty()) {
        return;
    }

    if (resolution.fromRecovery) {
        forcedLoadResolution_ = resolution;
        const QString openArg = ProjectPersistence::normalizeProjectPath(resolution.projectPath);
        loadFileFromPath(openArg);
        return;
    }

    if (cmdFilePath.isEmpty()) {
        return;
    }

    skipRecoveryPrompt_ = true;
    loadFileFromPath(cmdFilePath);
    skipRecoveryPrompt_ = false;
}

//从文件管理器打开文件
/**
 * @brief 从资源管理器选择并打开 .flow 文件
 * 函数级注释：该入口用于用户主动切换项目；为稳定性起见，统一走重启打开。
 */
void MainWindow::loadFileFromExplorer() {
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Open Flow Scene"),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.flow)"));
    if (!QFileInfo::exists(fileName))
        return;

    restartAndOpenFlow(fileName);

}

/**
 * @brief 序列化当前项目为 .flow JSON 对象
 * @return 包含 DataFlow、TimeLine、布局等模块的完整项目快照
 * 函数级注释：供手动保存与自动保存共用，确保写入内容一致。
 */
QJsonObject MainWindow::serializeProject(ProjectSaveOrigin origin) const
{
    ProjectSnapshotSources sources;
    sources.dataflowViewsManger = dataflowViewsManger;
    sources.timeline = timeline;
    sources.scheduledTaskWidget = scheduledTaskWidget;
    sources.dockManager = m_DockManager;
    sources.httpServer = httpServer;
    return buildProjectSnapshot(sources, origin);
}

/**
 * @brief 初始化自动保存管理器
 * 函数级注释：按配置启用周期性 recovery 保存；退出时写入最后一次快照并标记正常关闭。
 */
void MainWindow::setupAutosave()
{
    if (autosaveManager) {
        return;
    }

    autosaveManager = new AutosaveManager(this);
    autosaveManager->setEnabled(ConfigManager::instance().isAutosaveEnabled());
    autosaveManager->setIntervalSeconds(ConfigManager::instance().getAutosaveIntervalSeconds());
    autosaveManager->setSerializer([this]() {
        return serializeProject(ProjectSaveOrigin::Autosave);
    });
    autosaveManager->setProjectPath(QString());
    autosaveManager->start();

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        if (isRestarting) {
            return;
        }
        if (httpServer && httpServer->running()) {
            httpServer->stop();
        }
        finalizeAutosave();
        ProjectPersistence::markCleanShutdown();
    });
}

/**
 * @brief 立即执行一次自动保存
 * 函数级注释：在程序退出或关闭窗口时调用，将当前项目写入 recovery 目录。
 */
void MainWindow::finalizeAutosave()
{
    if (!autosaveManager) {
        return;
    }
    autosaveManager->saveRecoveryNow();
}

/**
 * @brief 项目加载完成后的回调
 * @param projectPath 逻辑项目路径（用户保存用的 .flow 路径，非 recovery 文件路径）
 * 函数级注释：更新 currentProjectPath，并同步自动保存管理器中的项目路径；
 *            若实际加载的是 autosave.flow，则从 meta 中还原原项目路径。
 */
void MainWindow::onProjectLoaded(const QString& projectPath)
{
    QString normalizedPath = ProjectPersistence::normalizeProjectPath(projectPath);
    if (ProjectPersistence::isAutosavePath(normalizedPath)) {
        normalizedPath = ProjectPersistence::normalizeProjectPath(
            ProjectPersistence::readRecovery().projectPath);
    }
    currentProjectPath = normalizedPath;
    if (!autosaveManager) {
        return;
    }
    autosaveManager->setPaused(true);
    autosaveManager->setProjectPath(normalizedPath);
    autosaveManager->setPaused(false);
}

/**
 * @brief 解析启动/打开时应加载的文件路径
 * @param requestedPath 用户请求打开的路径，空表示启动时无命令行文件
 * @return 实际加载路径与逻辑项目路径；若需恢复则 fromRecovery 为 true
 * 函数级注释：检测到异常退出且存在 recovery 时弹出恢复对话框，供 loadFileFromPath 使用。
 */
ProjectLoadResolution MainWindow::resolveRecoveryLoadPath(const QString& requestedPath) const
{
    return resolveProjectLoadPath(requestedPath, const_cast<MainWindow*>(this));
}

//保存文件到路径
void MainWindow::saveFileToPath(){
    if(currentProjectPath.isEmpty()){
        saveFileToExplorer();
        qDebug() << "Saved data to" << currentProjectPath;
        return;
    }
    if (saveProjectSnapshotAtomic(currentProjectPath, serializeProject())) {
        qDebug() << "Saved data to" << currentProjectPath;
        if (autosaveManager) {
            autosaveManager->clearRecovery();
        }
    }
}
//保存文件到资源管理器
void MainWindow::saveFileToExplorer() {
    QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                    tr("Open Flow Scene"),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.flow)"));
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith("flow", Qt::CaseInsensitive))
            fileName += ".flow";

        if (saveProjectSnapshotAtomic(fileName, serializeProject())) {
            onProjectLoaded(fileName);
            this->setWindowTitle(QFileInfo(fileName).fileName());
            ConfigManager::instance().addRecentFile(currentProjectPath);
            menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
            qDebug() << "Saved data to" << currentProjectPath;
            if (autosaveManager) {
                autosaveManager->clearRecovery();
            }
        }
    }

}
// 保存布局
void MainWindow::updateVisualState()
{
    QFile file("./DefaultLayout.ini");
    if (file.open(QIODevice::WriteOnly))//不存在则创建,默认覆盖  | QIODevice::Append
        //if (file.open(QIODevice::WriteOnly | QIODevice::Append));//文末追加,不存在则创建
    {
        QDataStream out(&file);
        out << m_DockManager->saveState();
        file.close();
    }
}
//恢复布局
void MainWindow::resetVisualState()
{

    QFile file("./DefaultLayout.ini");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray ba;
        QDataStream in(&file);
        in >> ba;
        file.close();
        m_DockManager->restoreState(ba);
    }

}

/**
 * @brief 主窗口关闭事件
 * @param event 关闭事件
 * 函数级注释：
 * - 正常关闭：若系统托盘可用则询问是否最小化到托盘
 * - 自重启关闭：isRestarting=true 时直接 accept，避免弹窗阻塞重启链路
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
    if (isRestarting) {
        event->accept();
        return;
    }

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "",
                                  tr("是否最小化到系统托盘？"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                  QMessageBox::Cancel); // 默认按钮为 Cancel

        if (reply == QMessageBox::Yes) {
            qDebug() << "Minimize to system tray";
            this->hide();
            event->ignore(); // 保持进程运行
        } else if (reply == QMessageBox::No) {
            // 先停 HTTP/WebSocket，再打退出日志，避免日志广播 sendFrame 拖慢关闭
            if (httpServer && httpServer->running()) {
                httpServer->stop();
            }
            qDebug() << "The program exits manually";
            finalizeAutosave();
            ProjectPersistence::markCleanShutdown();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        if (httpServer && httpServer->running()) {
            httpServer->stop();
        }
        finalizeAutosave();
        ProjectPersistence::markCleanShutdown();
        event->accept();
    }
}

void MainWindow::updateViewMenu(QMenu* menu)
{
    menu->clear();
    for (auto* DockWidget : m_DockManager->dockWidgetsMap())
    {
        menu->addAction(DockWidget->toggleViewAction());
    }
}

QMenu* MainWindow::makeOptionsMenu(QWidget* parent, const QList<QAction*>& actions) {
    QMenu* menu = new QMenu(tr("Options"), parent);
    QAction* menuAct = menu->menuAction();
    menuAct->setObjectName("optionsMenu");
    menuAct->setIcon(QIcon(":/ads_icons/ads_icons/custom-menu-button.svg"));
    menu->setToolTip(menu->title());
    menu->addActions(actions);
    return menu;
}

/**
 * @brief 构建 QWindowKit 无边框标题栏并完成 WindowAgent 绑定
 *
 * 布局结构（自左向右）：
 *   [应用图标] [菜单栏 MenuBarWidget] [居中标题] [最小化] [最大化/还原] [关闭]
 *
 * 说明：
 * - 通过 setMenuWidget(windowBar) 将自定义标题栏挂到 QMainWindow 顶部
 * - WindowAgent 负责无边框拖拽、边缘缩放、Snap Layout 等 Windows 原生行为
 * - 标题栏内除 setHitTestVisible 标记的控件外，其余区域均可拖拽移动窗口
 */
void MainWindow::setupFramelessWindow()
{
    if (!windowAgent || !menuBar) {
        return;
    }

    // 供 QSS 选择器 QMenuBar#win-menu-bar 定位样式
    menuBar->setObjectName(QStringLiteral("win-menu-bar"));

    // 居中显示窗口标题（项目名 / 文件名），样式见 QSS #win-title-label
    auto *titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setObjectName(QStringLiteral("win-title-label"));

    // --- 标题栏左侧：应用图标 ---
    // WindowButton 须用 setIconNormal 设置图标，不能仅用 QPushButton::setIcon
    auto *iconButton = new QWK::WindowButton(this);
    iconButton->setObjectName(QStringLiteral("icon-button"));
    iconButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    iconButton->setIconNormal(applicationIcon());

    // --- 标题栏右侧：最小化 / 最大化(可切换) / 关闭 ---
    auto *minButton = new QWK::WindowButton(this);
    minButton->setObjectName(QStringLiteral("min-button"));
    minButton->setProperty("system-button", true);  // QSS 统一样式
    minButton->setIconSize(QSize(16, 16));
    minButton->setFixedHeight(32);
    minButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *maxButton = new QWK::WindowButton(this);
    maxButton->setCheckable(true);  // 最大化时 checked，显示还原图标
    maxButton->setObjectName(QStringLiteral("max-button"));
    maxButton->setProperty("system-button", true);
    maxButton->setIconSize(QSize(16, 16));
    maxButton->setFixedHeight(32);
    maxButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *closeButton = new QWK::WindowButton(this);
    closeButton->setObjectName(QStringLiteral("close-button"));
    closeButton->setProperty("system-button", true);
    closeButton->setIconSize(QSize(16, 16));
    closeButton->setFixedHeight(32);
    closeButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // --- 组装 WindowBar ---
    windowBar = new QWK::WindowBar(this);
    windowBar->setObjectName(QStringLiteral("win-window-bar"));
    // 必须先绑定 hostWidget，再设置 titleLabel / iconButton，
    // 这样 WindowBar 的事件过滤器才能收到 WindowTitleChange / WindowStateChange
    windowBar->setHostWidget(this);
    windowBar->setIconFollowWindow(true);  // 窗口图标变化时自动更新左上角按钮
    windowBar->setIconButton(iconButton);
    windowBar->setMinButton(minButton);
    windowBar->setMaxButton(maxButton);
    windowBar->setCloseButton(closeButton);
    windowBar->setMenuBar(menuBar);
    windowBar->setTitleLabel(titleLabel);

    // --- 注册到 QWindowKit WindowAgent ---
    windowAgent->setTitleBar(windowBar);
    // 声明各按钮的系统角色，使 Windows Snap Layout、任务栏预览等原生特性正常工作
    windowAgent->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Minimize, minButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Maximize, maxButton);
    windowAgent->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
    // 菜单栏需要接收鼠标点击；未标记 hit-test 的区域默认作为拖拽区
    windowAgent->setHitTestVisible(menuBar, true);

    setMenuWidget(windowBar);

    // --- 信号连接 ---
    // 点击左上角图标：弹出 Windows 系统菜单（还原 / 移动 / 大小 / 最小化 / 最大化 / 关闭）
    connect(iconButton, &QAbstractButton::clicked, this, [this, iconButton]() {
        if (windowAgent) {
            windowAgent->showSystemMenu(
                iconButton->mapToGlobal(QPoint(0, iconButton->height())));
        }
    });
    connect(windowBar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(windowBar, &QWK::WindowBar::maximizeRequested, this, [this, maxButton](bool max) {
        if (max) {
            showMaximized();
        } else {
            showNormal();
        }
        // 窗口状态改变后，清除最大化按钮残留的 hover 高亮
        emulateLeaveEvent(maxButton);
    });
    connect(windowBar, &QWK::WindowBar::closeRequested, this, &QWidget::close);

    syncFramelessWindowState();
}

/**
 * @brief 同步无边框标题栏的显示状态
 *
 * 在以下场景被调用：初始化完成、showEvent、窗口状态变化、
 * 激活/失活、主题切换、从托盘恢复等。
 *
 * 同步内容：
 * - 标题文字（与 windowTitle() 一致）
 * - 左上角应用图标
 * - 最大化按钮的 checked 状态（最大化 ↔ 还原图标）
 * - 标题栏激活/失活样式（QSS 属性 bar-active）
 */
void MainWindow::syncFramelessWindowState()
{
    if (!windowBar) {
        return;
    }

    if (auto *label = windowBar->titleLabel()) {
        label->setText(windowTitle());
    }
    if (auto *iconBtn = qobject_cast<QWK::WindowButton *>(windowBar->iconButton())) {
        const QIcon icon = windowIcon().isNull() ? applicationIcon() : windowIcon();
        iconBtn->setIconNormal(icon);
    }
    if (auto *maxBtn = windowBar->maxButton()) {
        maxBtn->setChecked(isMaximized());
    }
    if (auto *bar = menuWidget()) {
        bar->setProperty("bar-active", isActiveWindow());
        if (style()) {
            style()->polish(bar);
        }
    }
}

/**
 * @brief 窗口首次显示时同步标题栏状态
 * @note main.cpp 中以 showMaximized() 启动，此处确保最大化按钮图标正确
 */
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    syncFramelessWindowState();
}

/**
 * @brief 窗口状态变化（最大化 / 还原 / 最小化）时同步标题栏
 */
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        syncFramelessWindowState();
    }
}

/**
 * @brief 处理窗口激活/失活，切换标题栏前景样式
 *
 * 激活时 bar-active=true（深色主题 #2f3034 / 浅色主题 #E8E9ED），
 * 失活时 bar-active=false（颜色略暗），对应 DefaultDark/Light.qss。
 */
bool MainWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        syncFramelessWindowState();
        break;
    default:
        break;
    }
    return QMainWindow::event(event);
}

void MainWindow::applyTheme(bool isDark)
{
    const QString qss = isDark ? AppConstants::DARK_STYLESHEET
                               : AppConstants::LIGHT_STYLESHEET;
    QFile qssFile(qss);
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load QSS file:" << qssFile.errorString();
        return;
    }

    QTextStream stream(&qssFile);
    const QString styleSheet = stream.readAll();
    qssFile.close();

    if (styleSheet.isEmpty()) {
        qWarning() << "QSS file is empty or could not be read properly";
        return;
    }

    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(styleSheet);
    }
    if (m_DockManager) {
        m_DockManager->setStyleSheet(styleSheet);
    }
    setNodeEditorDarkStyle(isDark);
}
