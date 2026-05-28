//
// Created by 吴斌 on 2024/1/18.
//

#include <QMessageBox>
#include "MainWindow.hpp"
#include "Nodes/NodeEditorStyle.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "QFile"
#include"Widget/ConsoleWidget/LogWidget.hpp"
#include <QDropEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMimeData>
#include "Widget/NodeListWidget/NodeListWidget.hpp"
#include "Widget/PropertyWidget/PropertyWidget.hpp"
#include <QSettings>
#include <exception>
#include "../../Common/AppConfig/ConfigManager.h"
#include <QMetaObject>
using namespace ads;

/**
 * @brief 启动 HTTP 服务器并在端口占用的短窗口期内重试
 * @param server HTTP 服务器对象
 * @param port 监听端口
 * @param waitMs 最大等待时间（毫秒）
 * @return true 启动成功；false 启动失败
 * 函数级注释：用于解决“重启切换项目后端口尚未完全释放”导致网页不可访问的问题。
 */
static bool startHttpServerWithRetry(NodeStudio::NodeHttpServer* server, int port, int waitMs)
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

/**
 * @brief 全局应用样式表并强制重新抛光所有已存在控件
 * @param styleSheet 目标 QSS 内容
 * 函数级注释：用于主题切换后立即刷新所有已创建控件的显示效果。
 */
static void applyGlobalStyleSheet(const QString& styleSheet)
{
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(styleSheet);
        const auto widgets = QApplication::allWidgets();
        for (QWidget* w : widgets) {
            if (!w) continue;
            if (w->style()) {
                w->style()->unpolish(w);
                w->style()->polish(w);
            }
            w->update();
        }
    }
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {


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
    this->setMenuBar(menuBar);
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
   
    //读取主题设置
    switchTheme(ConfigManager::instance().isDefaultDarkTheme());
    // 节点编辑控件
    auto *NodeDockWidget = m_DockManager->createDockWidget("节点编辑");
    NodeDockWidget->setIcon(QIcon(":/icons/icons/genealogy.png"));
    emit initStatus("load node editor style success");
    dataflowViewsManger=new DataflowViewsManger(m_DockManager,this);
    emit initStatus("Init node editor success");
    // 插件管理器控件
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager success");
    // 节点库控件
    nodeDockLibraryWidget = m_DockManager->createDockWidget("节点库");
    nodeDockLibraryWidget->setIcon(QIcon(":/icons/icons/library.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeDockLibraryWidget);
    emit initStatus("Initialization nodeLibrary success");
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
    
    // 添加舞台控件
    auto *stageDockWidget = m_DockManager->createDockWidget("舞台");
    stageDockWidget->setObjectName("stage");
    stageDockWidget->setIcon(QIcon(":/icons/icons/stage.png"));
    stageWidget = new StageWidget();
    stageDockWidget->setWidget(stageWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, stageDockWidget);
    // 从timelinemodel中获取stage
    stageWidget->setStage(timeline->model->getStage());
     // 当 stage初始化完成或重新设置时，更新
     // connect(timeline->model, &TimelineModel::S_stageInited, [this]() {
     //     stageWidget->setStage(timeline->model->getStage());
     // });
    emit initStatus("Initialization Stage Widget success");

    // 节点列表显示控件
    auto *nodeListDockWidget = m_DockManager->createDockWidget("节点列表");
    nodeListDockWidget->setObjectName("nodeList");
    nodeListDockWidget->setIcon(QIcon(":/icons/icons/list.png"));
    nodeListWidget = new NodeListWidget(dataflowViewsManger, this);
    nodeListDockWidget->setWidget(nodeListWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, nodeListDockWidget);
    menuBar->views->addAction(nodeListDockWidget->toggleViewAction());
    emit initStatus("Initialization Node List Widget success");
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
    httpServer=new NodeStudio::NodeHttpServer();
    //http服务器文件上传后，直接打开（若已加载过项目，则自动走重启打开）
    connect(httpServer, &NodeStudio::NodeHttpServer::flowFileUploaded, this, &MainWindow::loadFileFromPath);
    const int port = ConfigManager::instance().getHttpServerPort();
    if (!startHttpServerWithRetry(httpServer, port, 3000)) {
        emit initStatus(tr("HTTP Server 启动失败，端口可能被占用: %1").arg(port));
    } else {
        emit initStatus("Initialization Http Server success");
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
    // 切换主题
    connect(menuBar->switchTheme, &QAction::triggered, this, [this]() {
        switchTheme(!isDarkTheme);
    });
    //清空所有数据流
    connect(menuBar->Clear_dataflows, &QAction::triggered, dataflowViewsManger, &DataflowViewsManger::clearAllScenes);
    //新建数据流程
    connect(menuBar->New_dataflow, &QAction::triggered, this, &MainWindow::createDataflowWidget);
    //    日志清空功能
    connect(menuBar->clearAction, &QAction::triggered, logTable, &LogWidget::clearTableWidget);

    setAcceptDrops(true);

    connect(menuBar->clearAction, &QAction::triggered, this,&MainWindow::createDataflowWidget);

     if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icons/NodeStudio.png"));
        trayIcon->setToolTip(tr("NodeStudio"));
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
}

void MainWindow::switchVisibleFromTray()
{
    if (this->isHidden()) {
        this->showMaximized();
        this->raise();
        this->activateWindow();
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
void MainWindow::initNodelist() {
    nodeLibrary=new NodeLibraryWidget();
    this->nodeDockLibraryWidget->setWidget(nodeLibrary);
    emit initStatus("Initialization nodes library success");
}

// 锁定切换
void MainWindow::locked_switch() {
    isLocked=!isLocked;
    dataflowViewsManger->setSceneLocked(isLocked);
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

    splashScreen.updateStatus(tr("Prepare to open: %1").arg(path));
    pumpUi();

    if (!path.isEmpty())
    {
        QFileInfo fileInfo(path);
        QString absolutePath = fileInfo.absoluteFilePath();

        absolutePath = absolutePath.replace("\\", "/");
        splashScreen.updateStatus(tr("Analyze path: %1").arg(absolutePath));
        pumpUi();

        QFile file(absolutePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            splashScreen.updateStatus(tr("Open file failed: %1").arg(file.errorString()));
            pumpUi();
            splashScreen.finish(this);
            QMessageBox::warning(this, "",
                                 tr("无法打开文件 %1:\n%2").arg(absolutePath).arg(file.errorString()));
            return;
        }

        const qint64 fileSize = file.size();
        if (fileSize > 2147483647LL) {
            splashScreen.updateStatus(tr("File is too large: %1 bytes").arg(fileSize));
            pumpUi();
            splashScreen.finish(this);
            QMessageBox::warning(this, "",
                                 tr("文件过大，无法加载 %1\n大小: %2 bytes").arg(absolutePath).arg(fileSize));
            return;
        }

        splashScreen.updateStatus(tr("Read file: %1").arg(absolutePath));
        pumpUi();

        QByteArray wholeFile;
        wholeFile.reserve((int)qMax<qint64>(0, fileSize));
        qint64 readBytes = 0;
        const qint64 chunkSize = 1024 * 1024;
        while (!file.atEnd()) {
            const QByteArray chunk = file.read(chunkSize);
            if (chunk.isEmpty() && file.error() != QFile::NoError) break;
            wholeFile.append(chunk);
            readBytes += chunk.size();

            if (fileSize > 0) {
                int pct = (int)((readBytes * 100) / fileSize);
                if (pct > 100) pct = 100;
                splashScreen.updateStatus(tr("Read file... %1% (%2/%3)").arg(pct).arg(readBytes).arg(fileSize));
            } else {
                splashScreen.updateStatus(tr("Read file... %1 bytes").arg(readBytes));
            }
            pumpUi();
        }

        if (file.error() != QFile::NoError) {
            splashScreen.updateStatus(tr("Read file failed: %1").arg(file.errorString()));
            pumpUi();
            splashScreen.finish(this);
            QMessageBox::warning(this, "",
                                 tr("读取文件失败 %1:\n%2").arg(absolutePath).arg(file.errorString()));
            return;
        }

        splashScreen.updateStatus(tr("Parse the flow file..."));
        pumpUi();

        auto jsonDoc = QJsonDocument::fromJson(wholeFile);
        if (jsonDoc.isNull() || !jsonDoc.isObject()) {
            splashScreen.updateStatus(tr("Parse the flow file failed"));
            pumpUi();
            splashScreen.finish(this);
            QMessageBox::warning(this, "",
                                 tr("无法解析文件 %1:\n%2").arg(absolutePath));
            return;
        }

        const QJsonObject root = jsonDoc.object();
        const auto abortLoad = [&](const QString& msg) {
            splashScreen.updateStatus(msg);
            pumpUi();
            splashScreen.finish(this);
            QMessageBox::warning(this, "", msg);
        };

        splashScreen.updateStatus(tr("Load the dataflow model ..."));
        pumpUi();

        QMetaObject::Connection dfConn;
        if (dataflowViewsManger) {
            dfConn = QObject::connect(dataflowViewsManger,
                                      &DataflowViewsManger::loadProgress,
                                      this,
                                      [&splashScreen, &pumpUi](const QString& sceneTitle, const QString& phase, int current, int total) {
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
                                      });
        }

        try {
            const QJsonValue v = root.value("DataFlow");
            if (!v.isObject()) {
                if (dfConn) QObject::disconnect(dfConn);
                abortLoad(tr("DataFlow 数据缺失或格式错误"));
                return;
            }
            dataflowViewsManger->load(v.toObject());
            if (dfConn) QObject::disconnect(dfConn);
        } catch (const std::exception& e) {
            if (dfConn) QObject::disconnect(dfConn);
            abortLoad(tr("加载 DataFlow 失败: %1").arg(QString::fromUtf8(e.what())));
            return;
        } catch (...) {
            if (dfConn) QObject::disconnect(dfConn);
            abortLoad(tr("加载 DataFlow 失败"));
            return;
        }

        splashScreen.updateStatus(tr("Load the timeline model ..."));
        pumpUi();
        try {
            const QJsonValue v = root.value("TimeLine");
            if (!v.isObject()) {
                abortLoad(tr("TimeLine 数据缺失或格式错误"));
                return;
            }
            timeline->load(v.toObject());
        } catch (const std::exception& e) {
            abortLoad(tr("加载 TimeLine 失败: %1").arg(QString::fromUtf8(e.what())));
            return;
        } catch (...) {
            abortLoad(tr("加载 TimeLine 失败"));
            return;
        }

        splashScreen.updateStatus(tr("Load the scheduled tasks model ..."));
        pumpUi();
        try {
            const QJsonValue v = root.value("ScheduledTasks");
            if (!v.isObject()) {
                abortLoad(tr("ScheduledTasks 数据缺失或格式错误"));
                return;
            }
            scheduledTaskWidget->load(v.toObject());
        } catch (const std::exception& e) {
            abortLoad(tr("加载 ScheduledTasks 失败: %1").arg(QString::fromUtf8(e.what())));
            return;
        } catch (...) {
            abortLoad(tr("加载 ScheduledTasks 失败"));
            return;
        }

        const QString layoutBase64 = jsonDoc.object()["VisualLayout"].toString();
        if (!layoutBase64.isEmpty()) {
            splashScreen.updateStatus(tr("Load the visual layout ..."));
            pumpUi();
            const QByteArray layoutBytes = QByteArray::fromBase64(layoutBase64.toLatin1());
            m_DockManager->restoreState(layoutBytes);
        }else {
            resetVisualState();
        }

        const QJsonObject webLayout = jsonDoc.object().value("WebLayout").toObject();
        if (!webLayout.isEmpty() && httpServer) {
            splashScreen.updateStatus(tr("Load the web layout..."));
            pumpUi();
            httpServer->load(webLayout);
        }

        ConfigManager::instance().addRecentFile(absolutePath);
        menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());

        currentProjectPath=absolutePath;

        this->setWindowTitle(ConfigManager::instance().getCurrentFlowPath().split("/").last());
        splashScreen.updateStatus(tr("Load flow file completed"));
        pumpUi();
        splashScreen.finish(this);
    }
    else {
        splashScreen.updateStatus(tr("Path is empty"));
        pumpUi();
        splashScreen.finish(this);
    }
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

//保存文件到路径
void MainWindow::saveFileToPath(){
    if(currentProjectPath.isEmpty()){
        saveFileToExplorer();
        return;
    }
    QFile file(currentProjectPath);
    //不存在则创建,默认覆盖
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject flowJson;
        // 保存数据流
        flowJson["DataFlow"]=dataflowViewsManger->save();
        // 保存时间轴
        flowJson["TimeLine"]=timeline->save();
        // 保存计划任务
        flowJson["ScheduledTasks"]=scheduledTaskWidget->save();
        // 保存布局信息
        const QByteArray layoutBytes = m_DockManager->saveState();
        flowJson["VisualLayout"] = QString::fromLatin1(layoutBytes.toBase64());
        // 保存网页布局（HTTP Server）
        flowJson["WebLayout"] = httpServer ? httpServer->save() : QJsonObject{};
        
        file.write(QJsonDocument(flowJson).toJson(QJsonDocument::Compact));
        file.close();
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

        QFile file(fileName);
        //不存在则创建,默认覆盖
        if (file.open(QIODevice::WriteOnly)) {
            QJsonObject flowJson;
            // 保存数据流
            flowJson["DataFlow"]=dataflowViewsManger->save();
            // 保存时间轴
            flowJson["TimeLine"]=timeline->save();
            // 保存计划任务
            flowJson["ScheduledTasks"]=scheduledTaskWidget->save();
            // 保存布局信息
            const QByteArray layoutBytes = m_DockManager->saveState();
            flowJson["VisualLayout"] = QString::fromLatin1(layoutBytes.toBase64());
            // 保存网页布局（HTTP Server）
            flowJson["WebLayout"] = httpServer ? httpServer->save() : QJsonObject{};

            file.write(QJsonDocument(flowJson).toJson(QJsonDocument::Compact));
            file.close();
            currentProjectPath=fileName;
            this->setWindowTitle(file.fileName());
            // 保存后加入最近文件并刷新菜单
            ConfigManager::instance().addRecentFile(currentProjectPath);
            menuBar->updateRecentFileActions(ConfigManager::instance().getRecentFiles());
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
            // 选择否：保存布局并退出
            qDebug() << "The program exits manually";

            event->accept();
        } else {
            event->ignore();
        }
    } else {

        event->accept();
    }
}

void MainWindow::createDataflowWidget()
{
    
    bool ok;
    QString name = QInputDialog::getText(this,
                                         tr("新建Dataflow"),
                                         tr("请输入名称（不可变更）:"),
                                         QLineEdit::Normal,
                                         tr("Dataflow"),
                                         &ok);
    if (!ok || name.isEmpty())
        return;
    // 用户取消或空名称则直接返回，重名判断在DataflowViewsManger中
    dataflowViewsManger->addNewScene(name);
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

void MainWindow::switchTheme(bool isDark) {
    isDarkTheme = isDark;
    const QString qss = isDarkTheme ? AppConstants::DARK_STYLESHEET
                                    : AppConstants::LIGHT_STYLESHEET;
    QFile qssFile(qss);
    if (!qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load QSS file:" << qssFile.errorString();
        isDarkTheme = !isDarkTheme;
        return ;
    }

    QTextStream stream(&qssFile);
    const QString styleSheet = stream.readAll();
    qssFile.close();

    if (styleSheet.isEmpty()) {
        isDarkTheme = !isDarkTheme;
        qWarning() << "QSS file is empty or could not be read properly";
        return ;
    }

    if (!styleSheet.isEmpty()) {
        applyGlobalStyleSheet(styleSheet);
         if (m_DockManager) m_DockManager->setStyleSheet(styleSheet);
    }
    // 初始化节点编辑器样式表
    setNodeEditorDarkStyle(isDark);

    // 强制刷新所有场景
    if (dataflowViewsManger) {
        dataflowViewsManger->refreshAllScenes();
    }

    if (menuBar && menuBar->switchTheme) {
        menuBar->switchTheme->setText(isDarkTheme ? tr("切换到浅色主题")
                                                  : tr("切换到深色主题"));
        menuBar->switchTheme->setIcon(isDarkTheme ? QIcon(":/icons/icons/landscape.png")
                                                  : QIcon(":/icons/icons/night_landscape.png"));
    }
    QJsonObject config;
    config["DefaultDarkTheme"] = isDarkTheme;
    ConfigManager::instance().updateConfig(config);
}
