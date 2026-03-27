//
// Created by 吴斌 on 2024/1/18.
//

#include <QMessageBox>
#include <QApplication>
#include <QJsonParseError>
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
#include "../../Common/AppConfig/ConfigManager.h"
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
    DockHub::instance().setDockManager(m_DockManager);
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
    // controller->setDataflowModels(dataflowViewsManger->getModel());
    // controller->setTimelineModel(timelineModel);
    // controller->setTimelineToolBarMap(timeline->view->m_toolbar->getOscMapping());
    emit initStatus("Initialization external controler success");
	 // http 服务器
    httpServer=new NodeStudio::NodeHttpServer();
    httpServer->start(ConfigManager::instance().getHttpServerPort());
    //http服务器文件上传后，直接打开
    connect(httpServer, &NodeStudio::NodeHttpServer::flowFileUploaded, this, &MainWindowHeadLess::loadFileFromPath);
    emit initStatus("Initialization Http Server success");
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
    CustomSplashScreen splashScreen;

    const auto pumpUi = []() {
        QApplication::processEvents();
    };

    const auto update = [&](const QString& msg) {
        emit initStatus(msg);
        splashScreen.updateStatus(msg);
        pumpUi();
    };

    update(tr("Prepare to open: %1").arg(path));

    if (path.isEmpty()) {
        update(tr("Path is empty"));
        splashScreen.finish(this);
        return false;
    }

    QFileInfo fileInfo(path);
    QString absolutePath = fileInfo.absoluteFilePath();
    absolutePath = absolutePath.replace("\\", "/");
    update(tr("Analyze path: %1").arg(absolutePath));

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        update(tr("Open file failed: %1").arg(file.errorString()));
        splashScreen.finish(this);
        return false;
    }

    const qint64 fileSize = file.size();
    if (fileSize > 2147483647LL) {
        update(tr("File is too large: %1 bytes").arg(fileSize));
        splashScreen.finish(this);
        return false;
    }

    update(tr("Read file: %1").arg(absolutePath));

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
            update(tr("Read file... %1% (%2/%3)").arg(pct).arg(readBytes).arg(fileSize));
        } else {
            update(tr("Read file... %1 bytes").arg(readBytes));
        }
    }

    if (file.error() != QFile::NoError) {
        update(tr("Read file failed: %1").arg(file.errorString()));
        splashScreen.finish(this);
        return false;
    }

    update(tr("Parse the flow file..."));

    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(wholeFile, &parseError);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        const QString reason = parseError.error == QJsonParseError::NoError
                                   ? tr("Unknown error")
                                   : parseError.errorString();
        update(tr("Parse the flow file failed: %1").arg(reason));
        splashScreen.finish(this);
        return false;
    }

    const QJsonObject root = jsonDoc.object();

    update(tr("Load the dataflow model ..."));
    const QJsonValue df = root.value("DataFlow");
    if (!df.isObject()) {
        update(tr("DataFlow 数据缺失或格式错误"));
        splashScreen.finish(this);
        return false;
    }
    if (!dataflowViewsManger) {
        update(tr("DataflowViewsManger is null"));
        splashScreen.finish(this);
        return false;
    }

    QMetaObject::Connection dfConn;
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

    dataflowViewsManger->load(df.toObject());

    if (dfConn) {
        QObject::disconnect(dfConn);
    }

    update(tr("Load the timeline model ..."));
    const QJsonValue tl = root.value("TimeLine");
    if (!tl.isObject()) {
        update(tr("TimeLine 数据缺失或格式错误"));
        splashScreen.finish(this);
        return false;
    }
    if (!timeline) {
        update(tr("TimelineWidget is null"));
        splashScreen.finish(this);
        return false;
    }
    timeline->load(tl.toObject());

    update(tr("Load the scheduled tasks model ..."));
    const QJsonValue st = root.value("ScheduledTasks");
    if (!st.isObject()) {
        update(tr("ScheduledTasks 数据缺失或格式错误"));
        splashScreen.finish(this);
        return false;
    }
    if (!scheduledTaskWidget) {
        update(tr("ScheduledTaskWidget is null"));
        splashScreen.finish(this);
        return false;
    }
    scheduledTaskWidget->load(st.toObject());

    const QJsonObject webLayout = root.value("WebLayout").toObject();
    if (!webLayout.isEmpty() && httpServer) {
        update(tr("Load the web layout ..."));
        httpServer->load(webLayout);
    }

    ConfigManager::instance().addRecentFile(absolutePath);

    update(tr("Load flow file completed"));
    splashScreen.finish(this);

    if (trayIcon) {
        trayIcon->setToolTip(tr("正在运行 %1").arg(QFileInfo(absolutePath).fileName()));
    }
    qDebug() << tr("正在运行 %1").arg(QFileInfo(absolutePath).fileName());

    return true;
}

bool MainWindowHeadLess::loadRecentFile() {
    auto recentFile = ConfigManager::instance().getRecentFiles()[0];
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

