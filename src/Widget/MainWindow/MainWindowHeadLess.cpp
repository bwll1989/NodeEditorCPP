//
// Created by 吴斌 on 2024/1/18.
//

#include <QMessageBox>
#include "MainWindowHeadLess.hpp"
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
MainWindowHeadLess::MainWindowHeadLess(QWidget *parent): QMainWindow(parent) {

}
MainWindowHeadLess::~MainWindowHeadLess()
{
    delete controller;
    delete timeline;
}

void MainWindowHeadLess::init()
{
    m_DockManager = new ads::CDockManager(this);
    emit initStatus("Initialization ADS success");
    // 终端显示控件
    logTable=new LogWidget();
    logTable->resize(800,200);
    log=new LogHandler(logTable);
    emit initStatus("load nodes success");
    dataflowViewsManger=new DataflowViewsManger(m_DockManager,this);

    // 插件管理器
    pluginsManagerDlg=new PluginsManagerWidget();
    emit initStatus("initialization plugins success");
    // 创建时间线模型
    timelineModel = new TimeLineModel();
    // 创建时间线部件
    timeline = new TimelineWidget(timelineModel);
    emit initStatus("Initialization timeline success");
    // 计划任务控件
    scheduledTaskWidget=new ScheduledTaskWidget();
    emit initStatus("Initialization scheduled task success");
    // 时间轴控件
    stageWidget = new StageWidget();
    stageWidget->setStage(timeline->model->getStage());
    emit initStatus("Initialization stages success");
    // 媒体库控件
    mediaLibraryWidget = new MediaLibraryWidget();
    emit initStatus("Initialization media library success");
    // 外部控制器
    controller=new ExternalControler();
    controller->setDataflowModels(dataflowViewsManger->getModel());
    controller->setTimelineModel(timelineModel);
    controller->setTimelineToolBarMap(timeline->view->m_toolbar->getOscMapping());
    emit initStatus("Initialization external controler success");
    // 系统托盘图标
     if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/icons/NodeStudioRed.png"));
        // 托盘菜单
        trayMenu = new QMenu(this);
        trayExitAction    = trayMenu->addAction(tr("  退出  "));
        connect(trayExitAction,    &QAction::triggered, this, [this]() {
            qApp->quit();
            });
        trayIcon->setContextMenu(trayMenu);
        // 双击/单击托盘图标时还原
        connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick) {
             logTable->show();
            }
            });
        trayIcon->show();
    }
}

/**
 * 从路径打开文件（仅显示状态文字）
 * 使用 CustomSplashScreen 分阶段展示状态文本，避免界面卡顿。
 */
bool MainWindowHeadLess::loadFileFromPath(const QString &path)
{

    emit initStatus(tr("Prepare to open: %1").arg(path));

    if (!path.isEmpty())
    {
        // 获取绝对路径
        QFileInfo fileInfo(path);
        QString absolutePath = fileInfo.absoluteFilePath();

        // 将"\"转换成"/"，因为"\"系统不认
        absolutePath = absolutePath.replace("\\", "/");
        emit initStatus(tr("Analyze path: %1").arg(absolutePath));

        QFile file(absolutePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            emit initStatus(tr("Open file failed: %1").arg(file.errorString()));
            return false;
        }

        emit initStatus(tr("Read file: %1").arg(absolutePath));

        // 读取.flow文件
        QByteArray const wholeFile = file.readAll();

        emit initStatus(tr("Parse the flow file..."));

        auto jsonDoc = QJsonDocument::fromJson(wholeFile);
        if (jsonDoc.isNull()) {
           emit initStatus(tr("Parse the flow file failed"));

            return false;
        }

        emit initStatus(tr("Load the dataflow model..."));
        // 加载数据流模型
        dataflowViewsManger->load(jsonDoc.object()["DataFlow"].toObject());

        emit initStatus(tr("Load the timeline model..."));
        // 加载时间轴模型
        timeline->load(jsonDoc.object()["TimeLine"].toObject());

        emit initStatus(tr("Load the scheduled tasks model..."));
        // 加载计划任务模型
        scheduledTaskWidget->load(jsonDoc.object()["ScheduledTasks"].toObject());

        // 设置当前项目路径
        currentProjectPath = absolutePath;
        m_splash->updateStatus(tr("Load flow file completed"));
        m_splash->finish(this);
        // 更新托盘图标工具提示
        if (trayIcon)
            trayIcon->setToolTip(tr("正在运行 %1").arg(currentProjectPath));
        qDebug()<<tr("正在运行 %1").arg(currentProjectPath);
        return true;
    }
    else {
        emit initStatus(tr("Path is empty"));
        return false;
    }
}

bool MainWindowHeadLess::loadRecentFile() {
    auto recentFile = getRecentFiles()[0];
    if (!recentFile.isEmpty()) {
        return loadFileFromPath(recentFile);
    }
    return false;
}


void MainWindowHeadLess::closeEvent(QCloseEvent* event)
{
    qDebug() << "The program(Headless) exits manually";
    event->accept();
}

/**
 * 函数级注释：载入最近文件列表
 * - 从 cfg/RecentFiles.ini 读取键 "files"
 * - 返回 QStringList（若不存在返回空）
 */
QStringList MainWindowHeadLess::getRecentFiles() const
{

   
    QSettings settings(AppConstants::RECENT_FILES_STORAGE_DIR+"/RecentFiles.ini", QSettings::IniFormat);
    const QStringList files = settings.value("files").toStringList();
    return files;
}

