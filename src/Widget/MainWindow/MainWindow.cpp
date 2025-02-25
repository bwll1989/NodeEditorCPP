//
// Created by 吴斌 on 2024/1/18.
//

#include <QMessageBox>
#include "MainWindow.h"
#include "Nodes/NodeStyle.hpp"
#include "Widget/CurveWidget/CurveEditor.h"
#include "Widget/AboutWidget/AboutWidget.hpp"
#include "Widget/ConsoleWidget/LogHandler.h"
#include "QFile"
#include "QTextStream"
#include"Widget/ConsoleWidget/LogWidget.h"
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QWidgetAction>
#include <QLineEdit>
#include <QTimer>
#include "Common/GUI/Elements/MartixWidget/MatrixWidget.h"
#include "Eigen/Core"
#include "Widget/NodeListWidget/NodeListWidget.hpp"
#include "Widget/TimeLineWidget/timelineview.hpp"
#include "Widget/ClipPropertyWidget/ClipPropertyWidget.hpp"
#include "Widget/TimeLineWidget/abstractclipmodel.hpp"
#define ConsoleDisplay false
#define PropertytDisplay true
#define ToolsDisplay true
using namespace ads;
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    ::setStyle();


    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    //    聚焦高亮
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, true);
    //    每个区域都有独立的关闭按钮
    ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, false);

    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);

    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);

}
void MainWindow::init()
{
    // awesome = new fa::QtAwesome(this);
    // awesome->initFontAwesome();


    m_DockManager = new ads::CDockManager(this);
//    m_DockManager->setStyleSheet(" ");
    menuBar=new MenuBarWidget(this);
    this->setCentralWidget(m_DockManager);
    emit initStatus("Initialization ADS success");
    this->setMenuBar(menuBar);
    emit initStatus("Initialization MenuBar success");
// 节点编辑控件
    auto *NodeDockWidget = new ads::CDockWidget("节点编辑");
    NodeDockWidget->setIcon(QIcon(":/icons/icons/dashboard.png"));
    view = new CustomGraphicsView();
    view->setObjectName("Canvas");
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    menuBar->views->addAction(NodeDockWidget->toggleViewAction());
    emit initStatus("Initialization view success");
    //初始化数据流模型
    dataFlowModel=new CustomDataFlowGraphModel(PluginsManager::instance()->registry());
    emit initStatus("Initialization Model success");
    //初始化场景
    scene=new CustomFlowGraphicsScene(*dataFlowModel);
    //设置场景
    view->setScene(scene);
    emit initStatus("Initialization scene success");
    //设置节点编辑控件
    NodeDockWidget->setWidget(view);
    //添加到中心区域
    m_DockManager->addDockWidget(ads::CenterDockWidgetArea, NodeDockWidget);
    emit initStatus("Init node editor success");
// 插件管理器控件
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager");
// 节点库控件
    nodeDockLibraryWidget = new ads::CDockWidget("节点库");
    nodeDockLibraryWidget->setIcon(QIcon(":/icons/icons/library.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeDockLibraryWidget);
    menuBar->views->addAction(nodeDockLibraryWidget->toggleViewAction());
// 时间轴控件
    auto *timelineDockWidget = new ads::CDockWidget("时间轴");
    timelineDockWidget->setObjectName("timeline");
    timelineDockWidget->setIcon(QIcon(":/icons/icons/timeline.png"));
    timeline=new TimelineWidget();
    timelineDockWidget->setWidget(timeline);
     m_DockManager->addDockWidget(ads::LeftDockWidgetArea,  timelineDockWidget);
     menuBar->views->addAction(timelineDockWidget->toggleViewAction());
     emit initStatus("Initialization Timeline editor");

// 端口编辑控件
    auto *portEdits = new ads::CDockWidget("端口编辑");
    portEdits->setObjectName("NodePortEdit");
    portEdits->setIcon(QIcon(":/icons/icons/portEdit.png"));
    portEdit=new PortEditWidget(dataFlowModel);
    portEdits->setWidget(portEdit);
    m_DockManager->addDockWidget(ads::LeftDockWidgetArea, portEdits);
    menuBar->views->addAction(portEdits->toggleViewAction());
    emit initStatus("Initialization node port editor");
// 终端显示控件
    auto *logDockViewer= new ads::CDockWidget("终端显示");
    logTable=new LogWidget();
    log=new LogHandler(logTable);
    logDockViewer->setWidget(logTable);
    logDockViewer->setIcon(QIcon(":/icons/icons/bug.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, logDockViewer);
    menuBar->views->addAction(logDockViewer->toggleViewAction());
    emit initStatus("Initialization console widget");

    auto *DockView= new ads::CDockWidget("显示");
//    auto *fader=new XYPad(this);
//    auto *bar1=new BarButton(this);
//    auto *bar2=new BarButton(this);

    auto *w =new MatrixWidget(6, 2);
    DockView->setWidget(w);
    DockView->setIcon(QIcon(":/icons/icons/chartLine.png"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, DockView);
    menuBar->views->addAction(DockView->toggleViewAction());
    Eigen::MatrixXd mMat(6,2);
    mMat << 0.11,0.11,
            0.3,0.5,
            0.8,0.3,
            0.3,0.3,
            0.5,0.8,
            0.3,0.3;
    w->setValuesFromMatrix(mMat);

    // 添加片段属性面板
    auto *clipPropertyDockWidget = new ads::CDockWidget("片段属性");
    clipPropertyDockWidget->setObjectName("clipProperty");
    clipPropertyDockWidget->setIcon(QIcon(":/icons/icons/property.png"));
    clipProperty = new ClipPropertyWidget(this);
    clipPropertyDockWidget->setWidget(clipProperty);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, clipPropertyDockWidget);
    menuBar->views->addAction(clipPropertyDockWidget->toggleViewAction());
    emit initStatus("Initialization Clip Property editor");

    // 连接时间线的片段选择信号到属性面板
     // 在适当的位置添加连接
    connect(timeline->view, &TimelineView::currentClipChanged,[this](AbstractClipModel* clip){
        clipProperty->setClip(clip, timeline->model);
    });
    // connect(timeline, &TimelineWidget::clipSelected, 
    //         [this](AbstractClipModel* clip) {
    //             clipProperty->setClip(clip, timeline->model);
    //         });

    // 添加舞台控件
    auto *stageDockWidget = new ads::CDockWidget("舞台");
    stageDockWidget->setObjectName("stage");
    stageDockWidget->setIcon(QIcon(":/icons/icons/stage.png"));
    stageWidget = new StageWidget();
    stageDockWidget->setWidget(stageWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, stageDockWidget);
    menuBar->views->addAction(stageDockWidget->toggleViewAction());
    //从timelinemodel中获取stage
    stageWidget->setStage(timeline->model->getStage());
    emit initStatus("Initialization Stage");
    // 当 stage 改变时更新
    connect(timeline->model, &TimelineModel::stageChanged, [this]() {
        stageWidget->setStage(timeline->model->getStage());
    });
    emit initStatus("Initialization Stage Widget");

    // 节点列表显示控件
    auto *nodeListDockWidget = new ads::CDockWidget("节点列表");
    nodeListDockWidget->setObjectName("nodeList");
    nodeListDockWidget->setIcon(QIcon(":/icons/icons/list.png"));
    nodeListWidget = new NodeListWidget(dataFlowModel, scene, this);
    nodeListDockWidget->setWidget(nodeListWidget);
    m_DockManager->addDockWidget(ads::RightDockWidgetArea, nodeListDockWidget);
    menuBar->views->addAction(nodeListDockWidget->toggleViewAction());
    emit initStatus("Initialization Node List Widget");

    connect(menuBar->saveLayout, &QAction::triggered, this, &MainWindow::saveVisualState);
    //保存布局
    connect(menuBar->restoreLayout, &QAction::triggered, this, &MainWindow::restoreVisualState);
    //恢复布局
    connect(menuBar->aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    //关于软件窗口
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
    // 插件管理器
    connect(menuBar->pluginsFloderAction, &QAction::triggered, pluginsManagerDlg,&PluginsManagerWidget::openPluginsFolder);
    //鼠标点击时显示属性
    // connect(scene, &CustomFlowGraphicsScene::nodeClicked, property,&PropertyWidget::update);
    connect(scene, &CustomFlowGraphicsScene::nodeClicked,portEdit,&PortEditWidget::update);
    //    节点删除时更新属性显示
    // connect(model,&CustomDataFlowGraphModel::nodeDeleted, property,&PropertyWidget::update);
    connect(dataFlowModel,&CustomDataFlowGraphModel::nodeDeleted,portEdit,&PortEditWidget::update);
    //    日志清空功能
    connect(menuBar->clearAction, &QAction::triggered, logTable, &LogWidget::clearTableWidget);
//    导入文件，视图回到中心
    connect(scene, &CustomFlowGraphicsScene::sceneLoaded, view, &CustomGraphicsView::centerScene);

    // 恢复布局
    setAcceptDrops(true);

   

}
//显示属性
void MainWindow::initNodelist() {
    nodeLibrary=new NodeLibraryWidget(dataFlowModel,view,scene);
    this->nodeDockLibraryWidget->setWidget(nodeLibrary);
    emit initStatus("Initialization nodes library success");
}
//软件关于
void MainWindow::showAboutDialog() {

    AboutWidget aboutDialog(":/docs/README.txt");
//    关于窗口显示文档设置
    aboutDialog.exec();
}
// 锁定切换
void MainWindow::locked_switch() {
    isLocked=!isLocked;
    dataFlowModel->setNodesLocked(isLocked);
    //    显示锁定
    dataFlowModel->setDetachPossible(!isLocked);
    //    不可分离连接

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

    scene->clearScene();
    //    场景清空
    QByteArray const wholeFile = file.readAll();
    //    读取.flow文件
    dataFlowModel->load(QJsonDocument::fromJson(wholeFile).object());

    event->acceptProposedAction();

}
//从路径打开文件
void MainWindow::loadFileFromPath(QString *path)
{
    if(!path->isEmpty())
    {
        // 获取绝对路径
        QFileInfo fileInfo(*path);
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
        
        scene->clearScene();
        //    场景清空
        QByteArray const wholeFile = file.readAll();
        //    读取.flow文件
        dataFlowModel->load(QJsonDocument::fromJson(wholeFile).object());
        currentProjectPath = absolutePath;
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

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return;

        scene->clearScene();
        //    场景清空
        QByteArray const wholeFile = file.readAll();
        //    读取.flow文件
        auto senceFile=QJsonDocument::fromJson(wholeFile).object();
        // 加载数据流
        dataFlowModel->load(senceFile["DataFlow"].toObject());
        // 加载时间轴
        timeline->load(senceFile["TimeLine"].toObject());
        // 设置当前项目路径
        currentProjectPath=fileName;
        emit scene->sceneLoaded();

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
        flowJson["DataFlow"]=dataFlowModel->save();
        // 保存时间轴
        flowJson["TimeLine"]=timeline->save();
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
            flowJson["DataFlow"]=dataFlowModel->save();
            // 保存时间轴
            flowJson["TimeLine"]=timeline->save();
            file.write(QJsonDocument(flowJson).toJson());
            file.close();
            currentProjectPath=fileName;
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


//退出确认
void MainWindow::closeEvent(QCloseEvent *event) {
    // 弹出确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "退出确认", "您确定要退出吗？",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        qDebug()<<"The program exits manually";
        // 用户点击"是"，接受事件，应用程序关闭
//    结束日志记录器
        this->close();
        m_DockManager->deleteLater();
//    结束ADS

        event->accept();
    } else {
        // 用户点击"否"，忽略事件，应用程序保持运行
        event->ignore();
    }
}
