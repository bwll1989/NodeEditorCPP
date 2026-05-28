//
// Created by 吴斌 on 2024/1/18.
//

#include "MainWindowHeadLess.hpp"
#include "Nodes/NodeEditorStyle.hpp"
#include "Widget/ConsoleWidget/LogHandler.hpp"
#include "QFile"
#include "Widget/NodeListWidget/NodeListWidget.hpp"
#include "BaseTimeLineModel.h"
#include <QSettings>
#include "Widget/SplashWidget/CustomSplashScreen.hpp"
#include "../../Common/AppConfig/ConfigManager.h"
using namespace ads;

/**
 * @brief 启动 HTTP 服务器并在端口占用的短窗口期内重试
 * @param server HTTP 服务器对象
 * @param port 监听端口
 * @param waitMs 最大等待时间（毫秒）
 * @return true 启动成功；false 启动失败
 * 函数级注释：用于解决“进程刚重启、旧进程端口尚未完全释放”的时序问题。
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

MainWindowHeadLess::MainWindowHeadLess(QWidget *parent): QMainWindow(parent) {

}
MainWindowHeadLess::~MainWindowHeadLess()
{
    delete controller;
    delete timeline;
}

/**
 * @brief 初始化无头模式所需的后台组件
 * 函数级注释：
 * - 创建 DockManager（复用数据结构，不显示 UI）
 * - 初始化日志、数据流管理器、时间线/舞台、计划任务、外部控制
 * - 启动内置 HTTP 服务器用于网页访问与文件上传
 * - 初始化系统托盘（便于后台常驻与退出）
 */
void MainWindowHeadLess::init()
{
    m_DockManager = new ads::CDockManager(this);
    DockHub::instance().setDockManager(m_DockManager);
    emit initStatus("Initialization ADS success");
    // 无头模式：仅写入文件日志，不创建终端 UI
    log = new LogHandler(nullptr);
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
    emit initStatus("Initialization external controler success");
	 // http 服务器
    httpServer=new NodeStudio::NodeHttpServer();
    const int port = ConfigManager::instance().getHttpServerPort();
    if (!startHttpServerWithRetry(httpServer, port, 3000)) {
        emit initStatus(tr("HTTP Server 启动失败，端口可能被占用: %1").arg(port));
    }
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
        trayIcon->show();
    }
}

/**
 * 从路径打开文件（仅显示状态文字）
 * 使用 CustomSplashScreen 分阶段展示状态文本，避免界面卡顿。
 */
/**
 * @brief 从路径加载 flow 文件
 * @param path 待加载的 .flow 文件路径
 * @return true 成功；false 失败
 * 函数级注释：
 * - 首次加载（currentProjectPath 为空）：在当前进程中解析并加载项目。
 * - 二次打开（currentProjectPath 非空）：不直接卸载并重建节点，而是触发重启打开，避免第三方节点内存泄漏累积。
 */
bool MainWindowHeadLess::loadFileFromPath(const QString &path)
{
    if (!currentProjectPath.isEmpty()) {
        restartAndOpenFlow(path);
        return true;
    }
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
    currentProjectPath = absolutePath;

    update(tr("Load flow file completed"));
    splashScreen.finish(this);

    if (trayIcon) {
        trayIcon->setToolTip(tr("正在运行 %1").arg(QFileInfo(absolutePath).fileName()));
    }
    qDebug() << tr("正在运行 %1").arg(QFileInfo(absolutePath).fileName());

    return true;
}

/**
 * @brief 重启并打开指定项目文件
 * @param path 目标 .flow 文件路径
 * 函数级注释：
 * - 重启前主动 stop HTTP Server，尽量释放端口，避免新进程启动后网页不可访问
 * - 通过 startDetached 启动新进程，并携带 --restart-delay-ms 让新进程在启动前延迟等待旧进程退出
 * - 启动成功后退出当前进程
 */
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
    args << "--restart-delay-ms=200";
    args << abs;

    const bool started = QProcess::startDetached(exe, args);
    if (!started) {
        emit initStatus(tr("重启打开失败：无法启动新进程"));
        return;
    }

    qApp->quit();
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

