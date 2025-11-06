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
#include "Elements/BarButton/BarButton.h"
#include "Elements/XYPad/XYPad.h"
#define ConsoleDisplay false
#define PropertytDisplay true
#define ToolsDisplay true
using namespace ads;
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {


    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    //    聚焦高亮
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, true);
    //    每个区域都有独立的关闭按钮
    ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, false);

    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);

    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);


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
    m_DockManager->setStyleSheet("");

    m_DockManager->setDockWidgetToolBarStyle(Qt::ToolButtonStyle::ToolButtonTextOnly,CDockWidget::eState::StateDocked);
    menuBar=new MenuBarWidget(this);
    this->setCentralWidget(m_DockManager);
    emit initStatus("Initialization ADS success");
    this->setMenuBar(menuBar);
    emit initStatus("Initialization MenuBar success");
// 节点编辑控件
    auto *NodeDockWidget = new ads::CDockWidget("节点编辑");
    NodeDockWidget->setIcon(QIcon(":/icons/icons/dashboard.png"));
    // 初始化节点编辑器样式表
    setNodeEditorStyle();
    emit initStatus("load node editor style success");
    dataflowViewsManger=new DataflowViewsManger(m_DockManager,this);
    // emit initStatus("Init node editor success");
// 插件管理器控件
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager");
// 节点库控件
    nodeDockLibraryWidget = new ads::CDockWidget("节点库");
    nodeDockLibraryWidget->setIcon(QIcon(":/icons/icons/library.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeDockLibraryWidget);

// 时间轴控件
    // 创建时间线模型
    timelineModel = new TimeLineModel();
    // 创建时间线部件
    timeline = new TimelineWidget(timelineModel);
    // 创建时间轴控件
    auto *timelineDockWidget = new ads::CDockWidget("时间轴");
    timelineDockWidget->setObjectName("timeline");
    timelineDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    // timeline=new timelinewidget(timelineModel);
    timelineDockWidget->setWidget(timeline);
    // 添加到左侧区域
     m_DockManager->addDockWidget(ads::LeftDockWidgetArea,  timelineDockWidget);
     // 添加到菜单栏
     emit initStatus("Initialization Timeline editor");

// 终端显示控件
    auto *logDockViewer= new ads::CDockWidget("终端显示");
    logTable=new LogWidget();
    log=new LogHandler(logTable);
    logDockViewer->setWidget(logTable);
    logDockViewer->setIcon(QIcon(":/icons/icons/bug.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, logDockViewer);
    emit initStatus("Initialization console widget");

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
    auto *calendarDockWidget = new ads::CDockWidget("计划任务");
    calendarDockWidget->setObjectName("scheduled");
    calendarDockWidget->setIcon(QIcon(":/icons/icons/scheduled.png"));
    scheduledTaskWidget=new ScheduledTaskWidget();
    calendarDockWidget->setWidget(scheduledTaskWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, calendarDockWidget);
    emit initStatus("Initialization Scheduled");
    
    // 添加舞台控件
    auto *stageDockWidget = new ads::CDockWidget("舞台");
    stageDockWidget->setObjectName("stage");
    stageDockWidget->setIcon(QIcon(":/icons/icons/stage.png"));
    stageWidget = new StageWidget();
    stageDockWidget->setWidget(stageWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, stageDockWidget);
    // 从timelinemodel中获取stage
     stageWidget->setStage(timeline->model->getStage());
     emit initStatus("Initialization Stage");
     // 当 stage初始化完成或重新设置时，更新
     // connect(timeline->model, &TimelineModel::S_stageInited, [this]() {
     //     stageWidget->setStage(timeline->model->getStage());
     // });
    emit initStatus("Initialization Stage Widget");

    // 节点列表显示控件
    auto *nodeListDockWidget = new ads::CDockWidget("节点列表");
    nodeListDockWidget->setObjectName("nodeList");
    nodeListDockWidget->setIcon(QIcon(":/icons/icons/list.png"));
    nodeListWidget = new NodeListWidget(dataflowViewsManger, this);
    nodeListDockWidget->setWidget(nodeListWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, nodeListDockWidget);
    menuBar->views->addAction(nodeListDockWidget->toggleViewAction());
    emit initStatus("Initialization Node List Widget");
    // 媒体库控件
    auto *mediaLibraryDockWidget = new ads::CDockWidget("媒体库");
    mediaLibraryDockWidget->setObjectName("mediaLibrary");
    mediaLibraryDockWidget->setIcon(QIcon(":/icons/icons/media_mange.png"));
    mediaLibraryWidget = new MediaLibraryWidget();
    mediaLibraryDockWidget->setWidget(mediaLibraryWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, mediaLibraryDockWidget);
    // menuBar->views->addAction(mediaLibraryDockWidget->toggleViewAction());
    emit initStatus("Initialization Media Library Widget");

    // 外部控制器
    controller=new ExternalControler();
    controller->setDataflowModels(dataflowViewsManger->getModel());
    controller->setTimelineModel(timelineModel);
    controller->setTimelineToolBarMap(timeline->view->m_toolbar->getOscMapping());
    emit initStatus("Initialization External Controler");

    connect(menuBar->saveLayout, &QAction::triggered, this, &MainWindow::saveVisualState);
    //保存布局
    connect(menuBar->restoreLayout, &QAction::triggered, this, &MainWindow::restoreVisualState);
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
        trayIcon->setIcon(QIcon(":/icons/icons/NodeStudio.svg"));
        trayIcon->setToolTip(tr("NodeStudio 正在后台运行"));
        // 托盘菜单
        trayMenu = new QMenu(this);
        trayExitAction    = trayMenu->addAction(tr("  退出  "));
        connect(trayExitAction,    &QAction::triggered, this, [this]() {
            // 退出前保存布局
            saveVisualState();
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
//**
// 拖拽事件
//**
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

//从路径打开文件
void MainWindow::loadFileFromPath(const QString &path)
{
    if(!path.isEmpty())
    {
        // 获取绝对路径
        QFileInfo fileInfo(path);
        QString absolutePath = fileInfo.absoluteFilePath();
        
        // 将"\"转换成"/"，因为"\"系统不认
        absolutePath = absolutePath.replace("\\", "/");
        
        QFile file(absolutePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, tr("打开失败"),
                tr("无法打开文件 %1:\n%2").arg(absolutePath).arg(file.errorString()));
            return;
        }

        //    读取.flow文件
        QByteArray const wholeFile = file.readAll();
        auto jsonDoc = QJsonDocument::fromJson(wholeFile);
        if (jsonDoc.isNull()) {
            QMessageBox::warning(this, tr("打开失败"),
                tr("无法解析文件 %1").arg(absolutePath));
            return;
        }

        // 加载数据流模型
        dataflowViewsManger->load(jsonDoc.object()["DataFlow"].toObject());
        // 加载时间轴模型
        timeline->load(jsonDoc.object()["TimeLine"].toObject());
        // 加载计划任务模型
        scheduledTaskWidget->load(jsonDoc.object()["ScheduledTasks"].toObject());
        // 设置当前项目路径
        currentProjectPath=absolutePath;
        this->setWindowTitle(file.fileName());
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
            file.write(QJsonDocument(flowJson).toJson());
            file.close();
            currentProjectPath=fileName;
            this->setWindowTitle(file.fileName());
        }
    }
}

// 保存布局
void MainWindow::saveVisualState()
{
    QDir().mkpath("cfg");
    QFile file("cfg/VisualLayout.ini");
    if (file.open(QIODevice::WriteOnly))//不存在则创建,默认覆盖  | QIODevice::Append
        //if (file.open(QIODevice::WriteOnly | QIODevice::Append));//文末追加,不存在则创建
    {
        QDataStream out(&file);
        out << m_DockManager->saveState();
        file.close();
    }
}
//恢复布局
void MainWindow::restoreVisualState()
{

    QFile file("cfg/VisualLayout.ini");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray ba;
        QDataStream in(&file);
        in >> ba;
        file.close();
        if (m_DockManager->restoreState(ba))
        {
             emit initStatus("loading layout success");
        }
        else
        {
             emit initStatus("loading layout failure");
        }
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
            QMessageBox::question(this, tr("退出确认"),
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
            saveVisualState();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        saveVisualState();
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
