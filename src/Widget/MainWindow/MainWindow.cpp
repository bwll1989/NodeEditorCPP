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
#include <QWidgetAction>
#include "Common/GUI/Elements/MartixWidget/MatrixWidget.h"
#include "Eigen/Core"
#include "Widget/NodeListWidget/NodeListWidget.hpp"
#include "BaseTimeLineModel.h"
#include <QSettings>
#include "Elements/BarButton/BarButton.h"
#include "Elements/XYPad/XYPad.h"
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
using namespace ads;
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {


    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    //    聚焦高亮
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, true);
    //    每个区域都有独立的关闭按钮
    ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, false);

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
    //禁用ads内置样式表
    m_DockManager->setStyleSheet(":/styles/styles/DefaultDark.qss");

    m_DockManager->setDockWidgetToolBarStyle(Qt::ToolButtonStyle::ToolButtonIconOnly,CDockWidget::eState::StateDocked);
    this->setCentralWidget(m_DockManager);
    emit initStatus("Initialization ADS success");
    // 终端显示控件
    auto *logDockViewer= new ads::CDockWidget(m_DockManager,"终端显示");
    logTable=new LogWidget();
    log=new LogHandler(logTable);
    logDockViewer->setWidget(logTable);
    logDockViewer->setIcon(QIcon(":/icons/icons/bug.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, logDockViewer);
    //添加菜单项
    QMenu* OptionsMenu = makeOptionsMenu(logTable, logTable->getActions());
    logDockViewer->setTitleBarActions({OptionsMenu->menuAction()});
    emit initStatus("Initialization console widget");
    // 菜单条控件
    menuBar=new MenuBarWidget(this);
    connect(menuBar, &MenuBarWidget::recentFileTriggered,
             this, &MainWindow::openRecentFile);
     // 初始化时刷新最近文件菜单
    menuBar->updateRecentFileActions(getRecentFiles());
    this->setMenuBar(menuBar);
    emit initStatus("Initialization MenuBar success");
    // 节点编辑控件
    auto *NodeDockWidget = new ads::CDockWidget(m_DockManager,"节点编辑");
    NodeDockWidget->setIcon(QIcon(":/icons/icons/genealogy.png"));
    // 初始化节点编辑器样式表
    setNodeEditorStyle();
    emit initStatus("load node editor style success");
    dataflowViewsManger=new DataflowViewsManger(m_DockManager,this);
    emit initStatus("Init node editor success");
    // 插件管理器控件
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager success");
    // 节点库控件
    nodeDockLibraryWidget = new ads::CDockWidget(m_DockManager,"节点库");
    nodeDockLibraryWidget->setIcon(QIcon(":/icons/icons/library.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeDockLibraryWidget);
    emit initStatus("Initialization nodeLibrary success");
    // 时间轴控件
    // 创建时间线模型
    timelineModel = new TimeLineModel();
    // 创建时间线部件
    timeline = new TimelineWidget(timelineModel);
    // 创建时间轴控件
    auto *timelineDockWidget = new ads::CDockWidget(m_DockManager,"时间轴");
    timelineDockWidget->setObjectName("timeline");
    timelineDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    // timeline=new timelinewidget(timelineModel);
    timelineDockWidget->setWidget(timeline);
    // 添加到左侧区域
     m_DockManager->addDockWidget(ads::LeftDockWidgetArea,  timelineDockWidget);
     // 添加到菜单栏
     emit initStatus("Initialization Timeline editor success");



    // auto *DockView= new ads::CDockWidget("显示");
    //
    // auto *w =new MatrixWidget(6, 6);
    // DockView->setWidget(w);
    // DockView->setIcon(QIcon(":/icons/icons/chartLine.png"));
    // m_DockManager->addDockWidget(ads::BottomDockWidgetArea, DockView);
    // menuBar->views->addAction(DockView->toggleViewAction());
    // Eigen::MatrixXd mMat(6,2);
    // mMat << 0.11,0.11,
    //         0.3,0.5,
    //         0.8,0.3,
    //         0.3,0.3,
    //         0.5,0.8,
    //         0.3,0.3;
    // w->setValuesFromMatrix(mMat);
    auto *calendarDockWidget = new ads::CDockWidget(m_DockManager,"计划任务");
    calendarDockWidget->setObjectName("scheduled");
    calendarDockWidget->setIcon(QIcon(":/icons/icons/scheduled.png"));
    scheduledTaskWidget=new ScheduledTaskWidget();
    calendarDockWidget->setWidget(scheduledTaskWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, calendarDockWidget);
    emit initStatus("Initialization Scheduled Task success");
    
    // 添加舞台控件
    auto *stageDockWidget = new ads::CDockWidget(m_DockManager,"舞台");
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
    auto *nodeListDockWidget = new ads::CDockWidget(m_DockManager,"节点列表");
    nodeListDockWidget->setObjectName("nodeList");
    nodeListDockWidget->setIcon(QIcon(":/icons/icons/list.png"));
    nodeListWidget = new NodeListWidget(dataflowViewsManger, this);
    nodeListDockWidget->setWidget(nodeListWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, nodeListDockWidget);
    menuBar->views->addAction(nodeListDockWidget->toggleViewAction());
    emit initStatus("Initialization Node List Widget success");
    // 媒体库控件
    auto *mediaLibraryDockWidget = new ads::CDockWidget(m_DockManager,"媒体库");
    mediaLibraryDockWidget->setObjectName("mediaLibrary");
    mediaLibraryDockWidget->setIcon(QIcon(":/icons/icons/media_mange.png"));
    mediaLibraryWidget = new MediaLibraryWidget();
    mediaLibraryDockWidget->setWidget(mediaLibraryWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, mediaLibraryDockWidget);
    // menuBar->views->addAction(mediaLibraryDockWidget->toggleViewAction());
    emit initStatus("Initialization Media Library Widget success");

    // 外部控制器
    controller=new ExternalControler();
    controller->setDataflowModels(dataflowViewsManger->getModel());
    controller->setTimelineModel(timelineModel);
    controller->setTimelineToolBarMap(timeline->view->m_toolbar->getOscMapping());
    emit initStatus("Initialization External Controler");
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

void MainWindow::openRecentFile(const QString& path)
{

    loadFileFromPath(path);
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
    QString filePath = mimeData->urls().at(0).toLocalFile();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return ;

    dataflowViewsManger->clearAllScenes();
    //    场景清空
    loadFileFromPath(filePath);
    event->acceptProposedAction();

}
/**
 * 从路径打开文件（仅显示状态文字）
 * 使用 CustomSplashScreen 分阶段展示状态文本，避免界面卡顿。
 */
void MainWindow::loadFileFromPath(const QString &path)
{
    CustomSplashScreen splashScreen;
    splashScreen.updateStatus(tr("Prepare to open: %1").arg(path));

    if (!path.isEmpty())
    {
        // 获取绝对路径
        QFileInfo fileInfo(path);
        QString absolutePath = fileInfo.absoluteFilePath();

        // 将"\"转换成"/"，因为"\"系统不认
        absolutePath = absolutePath.replace("\\", "/");
        splashScreen.updateStatus(tr("Analyze path: %1").arg(absolutePath));

        QFile file(absolutePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            splashScreen.updateStatus(tr("Open file failed: %1").arg(file.errorString()));
            splashScreen.finish(this);
            QMessageBox::warning(this, "",
                                 tr("无法打开文件 %1:\n%2").arg(absolutePath).arg(file.errorString()));
            return;
        }

        splashScreen.updateStatus(tr("Read file: %1").arg(absolutePath));

        // 读取.flow文件
        QByteArray const wholeFile = file.readAll();

        splashScreen.updateStatus(tr("Parse the flow file..."));

        auto jsonDoc = QJsonDocument::fromJson(wholeFile);
        if (jsonDoc.isNull()) {
            splashScreen.updateStatus(tr("Parse the flow file failed"));
            splashScreen.finish(this);
            QMessageBox::warning(this, "",
                                 tr("无法解析文件 %1:\n%2").arg(absolutePath));
            return;
        }

        splashScreen.updateStatus(tr("Load the dataflow model..."));
        // 加载数据流模型
        dataflowViewsManger->load(jsonDoc.object()["DataFlow"].toObject());

        splashScreen.updateStatus(tr("Load the timeline model..."));
        // 加载时间轴模型
        timeline->load(jsonDoc.object()["TimeLine"].toObject());

        splashScreen.updateStatus(tr("Load the scheduled tasks model..."));
        // 加载计划任务模型
        scheduledTaskWidget->load(jsonDoc.object()["ScheduledTasks"].toObject());

        // 加载布局信息
        const QString layoutBase64 = jsonDoc.object()["VisualLayout"].toString();
        if (!layoutBase64.isEmpty()) {
            const QByteArray layoutBytes = QByteArray::fromBase64(layoutBase64.toLatin1());
            m_DockManager->restoreState(layoutBytes);
        }else {
            resetVisualState();
        }

        // 设置当前项目路径
        currentProjectPath = absolutePath;
        this->setWindowTitle(file.fileName());

        // 加入最近文件并刷新菜单
        addToRecentFiles(absolutePath);
        menuBar->updateRecentFileActions(getRecentFiles());

        splashScreen.updateStatus(tr("Load flow file completed"));
        splashScreen.finish(this);
    }
    else {
        splashScreen.updateStatus(tr("Path is empty"));
        splashScreen.finish(this);
    }
}
//从文件管理器打开文件
void MainWindow::loadFileFromExplorer() {
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Open Flow Scene"),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.flow)"));
    if (!QFileInfo::exists(fileName))
        return;
    loadFileFromPath(fileName);

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
        
        file.write(QJsonDocument(flowJson).toJson());
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

            file.write(QJsonDocument(flowJson).toJson());
            file.close();
            currentProjectPath=fileName;
            this->setWindowTitle(file.fileName());
            // 保存后加入最近文件并刷新菜单
            addToRecentFiles(currentProjectPath);
            menuBar->updateRecentFileActions(getRecentFiles());
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
 * @brief 处理主窗口的关闭事件
 * - 如果系统托盘可用，则弹出询问框：
 *   - Yes：最小化到托盘，忽略关闭事件（保持进程运行）
 *   - No：保存布局并退出，接受关闭事件
 *   - Cancel 或关闭对话框（X/Esc）：忽略关闭事件（不退出）
 * - 如果系统托盘不可用：直接保存布局并退出
 * @param event 关闭事件对象
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
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

/**
 * 函数级注释：载入最近文件列表
 * - 从 cfg/RecentFiles.ini 读取键 "files"
 * - 返回 QStringList（若不存在返回空）
 */
QStringList MainWindow::getRecentFiles() const
{

   
    QSettings settings(AppConstants::RECENT_FILES_STORAGE_DIR+"/RecentFiles.ini", QSettings::IniFormat);
    const QStringList files = settings.value("files").toStringList();
    return files;
}

/**
 * 函数级注释：保存最近文件列表
 * - 写入 cfg/RecentFiles.ini 键 "files"
 */
void MainWindow::saveRecentFiles(const QStringList& files) const
{
    QDir().mkpath(AppConstants::RECENT_FILES_STORAGE_DIR);
    QSettings settings(AppConstants::RECENT_FILES_STORAGE_DIR+"/RecentFiles.ini", QSettings::IniFormat);
    settings.setValue("files", files);
}

/**
 * 函数级注释：将新路径加入最近文件列表
 * - 规则：去重后插入到首位；保留最多 MenuBarWidget::MaxRecentFiles 个
 * - 同步写入配置文件
 */
void MainWindow::addToRecentFiles(const QString& path)
{
    if (path.isEmpty())
        return;

    QString abs = path;
    abs = abs.replace("\\", "/"); // 统一分隔符
    QStringList files = getRecentFiles();
    files.removeAll(abs);
    files.prepend(abs);
    while (files.size() > AppConstants::MaxRecentFiles) {
        files.removeLast();
    }
    saveRecentFiles(files);
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