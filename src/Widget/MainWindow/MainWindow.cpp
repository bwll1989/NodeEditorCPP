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
#include"Widget/ConsoleWidget/LogWidget.hpp"
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>
#include <QWidgetAction>
#include <QLineEdit>
#include "Common/GUI/Elements/MartixWidget/MatrixWidget.h"
#include "Eigen/Core"
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
    awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();


    m_DockManager = new ads::CDockManager(this);
//    m_DockManager->setStyleSheet(" ");
    menuBar=new MenuBarWidget(this);
    this->setCentralWidget(m_DockManager);
    emit initStatus("Initialization ADS success");
    this->setMenuBar(menuBar);
    emit initStatus("Initialization MenuBar success");

    auto *NodeWidget = new ads::CDockWidget("节点编辑");
    view = new CustomGraphicsView();
    view->setObjectName("Canvas");
    view->setContextMenuPolicy(Qt::ActionsContextMenu);
    emit initStatus("Initialization view success");
    model=new CustomDataFlowGraphModel(PluginsManager::instance()->registry());
    // std::shared_ptr<NodeDelegateModelRegistry> _register;
    // model=new CustomDataFlowGraphModel(_register);
    emit initStatus("Initialization Model success");

    scene=new CustomFlowGraphicsScene(*model);
    view->setScene(scene);
    emit initStatus("Initialization scene success");
    NodeWidget->setWidget(view);
    m_DockManager->setCentralWidget(NodeWidget);
    emit initStatus("Init node editor success");

    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization pluginsManager");

    nodeListWidget = new ads::CDockWidget("节点库");
    nodeListWidget->setIcon(awesome->icon("fa-solid fa-rectangle-list"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, nodeListWidget);
    menuBar->views->addAction(nodeListWidget->toggleViewAction());

    auto *propertyWidget = new ads::CDockWidget("组件属性");
    propertyWidget->setObjectName("NodeProperty");
    propertyWidget->setIcon(awesome->icon("fa-solid fa-chart-gantt"));
    property=new PropertyWidget(model);
    propertyWidget->setWidget(property);
    m_DockManager->addDockWidget(ads::LeftDockWidgetArea, propertyWidget);
    menuBar->views->addAction(propertyWidget->toggleViewAction());
    emit initStatus("Initialization node property editor");

    auto *portEdits = new ads::CDockWidget("端口编辑");
    portEdits->setObjectName("NodePortEdit");
    portEdits->setIcon(awesome->icon("fa-solid fa-shuffle"));
    portEdit=new PortEditWidget(model);
    portEdits->setWidget(portEdit);
    m_DockManager->addDockWidget(ads::LeftDockWidgetArea, portEdits);
    menuBar->views->addAction(portEdits->toggleViewAction());
    emit initStatus("Initialization node port editor");

    auto *logViewer= new ads::CDockWidget("终端显示");
    logTable=new LogWidget();
    log=new LogHandler(logTable);
    logViewer->setWidget(logTable);
    logViewer->setIcon(awesome->icon("fa-solid fa-terminal"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, logViewer);
    menuBar->views->addAction(logViewer->toggleViewAction());
    emit initStatus("Initialization console widget");
//    initPythonInterpreter();

    auto *View= new ads::CDockWidget("显示");
//    auto *fader=new XYPad(this);
//    auto *bar1=new BarButton(this);
//    auto *bar2=new BarButton(this);

    auto *w =new MatrixWidget(6, 2);
    View->setWidget(w);
    View->setIcon(awesome->icon("fa-solid fa-terminal"));
    m_DockManager->addDockWidget(ads::BottomDockWidgetArea, View);
    menuBar->views->addAction(View->toggleViewAction());
    Eigen::MatrixXd mMat(6,2);
    mMat << 0.11,0.11,
            0.3,0.5,
            0.8,0.3,
            0.3,0.3,
            0.5,0.8,
            0.3,0.3;
    w->setValuesFromMatrix(mMat);

    connect(menuBar->saveLayout, &QAction::triggered, this, &MainWindow::saveVisualState);
    //保存布局
    connect(menuBar->restoreLayout, &QAction::triggered, this, &MainWindow::restoreVisualState);
    //恢复布局
    connect(menuBar->aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    //关于软件窗口
    connect(menuBar->saveAction, &QAction::triggered, this, &MainWindow::saveFileToExplorer);
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
    connect(scene, &CustomFlowGraphicsScene::nodeClicked, property,&PropertyWidget::update);
    connect(scene, &CustomFlowGraphicsScene::nodeClicked,portEdit,&PortEditWidget::update);
    //    节点删除时更新属性显示
    connect(model,&CustomDataFlowGraphModel::nodeDeleted, property,&PropertyWidget::update);
    connect(model,&CustomDataFlowGraphModel::nodeDeleted,portEdit,&PortEditWidget::update);
    //    日志清空功能
    connect(menuBar->clearAction, &QAction::triggered, logTable, &LogWidget::clearTableWidget);
//    导入文件，视图回到中心
    connect(scene, &CustomFlowGraphicsScene::sceneLoaded, view, &CustomGraphicsView::centerScene);

    // 恢复布局
    setAcceptDrops(true);

}
//显示属性
void MainWindow::initNodelist() {
    nodeList=new NodeListWidget(model,view,scene);
    this->nodeListWidget->setWidget(nodeList);
    emit initStatus("Initialization nodes list success");
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
    model->setNodesLocked(isLocked);
    //    显示锁定
    model->setDetachPossible(!isLocked);
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
//拖拽事件
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
    model->load(QJsonDocument::fromJson(wholeFile).object());

    event->acceptProposedAction();

}
//从路径打开文件
void MainWindow::loadFileFromPath(QString *path) {

    if(!path->isEmpty())
    {
        // 将“\”转换成"/"，因为"\"系统不认
        QFile file(path->replace("\\", "/"));
        if (!file.open(QIODevice::ReadOnly))
        {
            return ;
        }
        scene->clearScene();
        //    场景清空
        QByteArray const wholeFile = file.readAll();
        //    读取.flow文件
        model->load(QJsonDocument::fromJson(wholeFile).object());
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
        model->load(QJsonDocument::fromJson(wholeFile).object());
        emit scene->sceneLoaded();

}
//保存文件到路径
void MainWindow::savFileToPath(const QString *path){
    if (!path->isEmpty()) {
        if (!path->endsWith("flow", Qt::CaseInsensitive))
            return;
        QFile file(*path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(model->save()).toJson());
            file.close();
        }
    }
    return ;
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
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(model->save()).toJson());
            file.close();
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
        // 用户点击“是”，接受事件，应用程序关闭
//    结束日志记录器
        this->close();
        m_DockManager->deleteLater();
//    结束ADS

        event->accept();
    } else {
        // 用户点击“否”，忽略事件，应用程序保持运行
        event->ignore();
    }
}
