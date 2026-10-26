#include "WebRenderBrowserProcess.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

WebRenderBrowserProcess::WebRenderBrowserProcess(QObject* parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_portPollTimer(new QTimer(this))
    , m_killUpgradeTimer(new QTimer(this))
{
    m_portPollTimer->setInterval(100);
    connect(m_portPollTimer, &QTimer::timeout, this, &WebRenderBrowserProcess::pollDevToolsPort);

    m_killUpgradeTimer->setSingleShot(true);
    m_killUpgradeTimer->setInterval(2000);
    connect(m_killUpgradeTimer, &QTimer::timeout, this, [this]() {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            m_process->kill();
        }
    });

    connect(m_process, &QProcess::started, this, &WebRenderBrowserProcess::onProcessStarted);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &WebRenderBrowserProcess::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &WebRenderBrowserProcess::onProcessError);
}

WebRenderBrowserProcess::~WebRenderBrowserProcess()
{
    m_pendingStart.reset();
    // 析构兜底：硬杀并短暂等待，避免遗留 headless 浏览器进程
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1500);
    }
}

QString WebRenderBrowserProcess::findBrowserExecutable()
{
    const QStringList candidates = {
#ifdef Q_OS_WIN
        QStringLiteral("C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe"),
        QStringLiteral("C:/Program Files/Microsoft/Edge/Application/msedge.exe"),
        QStringLiteral("C:/Program Files/Google/Chrome/Application/chrome.exe"),
        QStringLiteral("C:/Program Files (x86)/Google/Chrome/Application/chrome.exe"),
#else
        QStringLiteral("/usr/bin/google-chrome"),
        QStringLiteral("/usr/bin/google-chrome-stable"),
        QStringLiteral("/usr/bin/chromium"),
        QStringLiteral("/usr/bin/chromium-browser"),
        QStringLiteral("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"),
        QStringLiteral("/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge"),
#endif
    };

    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return QDir::toNativeSeparators(path);
        }
    }

#ifdef Q_OS_WIN
    // 从 PATH 再试一次
    const QString edge = QStandardPaths::findExecutable(QStringLiteral("msedge"));
    if (!edge.isEmpty()) {
        return edge;
    }
    const QString chrome = QStandardPaths::findExecutable(QStringLiteral("chrome"));
    if (!chrome.isEmpty()) {
        return chrome;
    }
#endif

    return {};
}

bool WebRenderBrowserProcess::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

qint64 WebRenderBrowserProcess::processId() const
{
    return m_process ? m_process->processId() : 0;
}

bool WebRenderBrowserProcess::start(const QString& userDataDir, int width, int height, const QString& extraOptions)
{
    // 记录待启动参数：若旧进程仍在运行则先异步停止，退出后自动以此参数重启
    m_pendingStart = PendingStart{ userDataDir, qMax(16, width), qMax(16, height), extraOptions };

    if (isRunning()) {
        requestStop();
        return true;
    }
    return launchPending();
}

void WebRenderBrowserProcess::stop()
{
    m_pendingStart.reset();
    requestStop();
}

void WebRenderBrowserProcess::requestStop()
{
    m_stopping = true;
    m_portPollTimer->stop();
    m_killUpgradeTimer->stop();
    m_debugPort = 0;

    if (!m_process || m_process->state() == QProcess::NotRunning) {
        m_stopping = false;
        return;
    }

    m_process->terminate();
    // headless 浏览器经常不响应 terminate()：2s 后升级为 kill()
    m_killUpgradeTimer->start();
}

bool WebRenderBrowserProcess::launchPending()
{
    if (!m_pendingStart) {
        return false;
    }
    const PendingStart params = *m_pendingStart;
    m_pendingStart.reset();
    return launch(params.userDataDir, params.width, params.height, params.extraOptions);
}

bool WebRenderBrowserProcess::launch(const QString& userDataDir, int width, int height, const QString& extraOptions)
{
    m_browserPath = findBrowserExecutable();
    if (m_browserPath.isEmpty()) {
        emit errorOccurred(QStringLiteral("未找到 Edge/Chrome，请安装 Microsoft Edge 或 Google Chrome"));
        return false;
    }

    if (userDataDir.isEmpty()) {
        emit errorOccurred(QStringLiteral("user-data-dir 不能为空"));
        return false;
    }
    m_userDataDir = userDataDir;
    QDir().mkpath(m_userDataDir);
    // 清理可能残留的 DevToolsActivePort，避免读到旧端口
    QFile::remove(QDir(m_userDataDir).filePath(QStringLiteral("DevToolsActivePort")));

    const int w = qMax(16, width);
    const int h = qMax(16, height);

    // 对齐 TD：启用 GPU 合成。旧版 --disable-gpu 会把整页逼到软光栅，30fps 必卡。
    QStringList args;
    args << QStringLiteral("--headless=new")
         << QStringLiteral("--enable-gpu")
         << QStringLiteral("--ignore-gpu-blocklist")
         << QStringLiteral("--enable-gpu-rasterization")
         << QStringLiteral("--disable-software-rasterizer")
#ifdef Q_OS_WIN
         << QStringLiteral("--use-angle=d3d11")
#endif
         << QStringLiteral("--hide-scrollbars")
         << QStringLiteral("--disable-background-networking")
         << QStringLiteral("--disable-extensions")
         << QStringLiteral("--disable-sync")
         << QStringLiteral("--no-first-run")
         << QStringLiteral("--no-default-browser-check")
         << QStringLiteral("--disable-popup-blocking")
         << QStringLiteral("--remote-allow-origins=*")
         << QStringLiteral("--remote-debugging-port=0")
         << QStringLiteral("--user-data-dir=%1").arg(QDir::toNativeSeparators(m_userDataDir))
         << QStringLiteral("--window-size=%1,%2").arg(w).arg(h);

    const QString extras = extraOptions.trimmed();
    if (!extras.isEmpty()) {
        // 简易按空格拆分；带引号的复杂参数可后续再增强
        args.append(extras.split(QLatin1Char(' '), Qt::SkipEmptyParts));
    }

    args << QStringLiteral("about:blank");

    m_debugPort = 0;
    m_portPollAttempts = 0;
    m_stopping = false;
    m_killUpgradeTimer->stop();

    m_process->setProgram(m_browserPath);
    m_process->setArguments(args);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    // 不 waitForStarted：同步等待最长阻塞 GUI 8s。
    // 启动失败（FailedToStart）会经 errorOccurred/startFailed 信号上报。
    m_process->start();

    return true;
}

void WebRenderBrowserProcess::onProcessStarted()
{
    m_portPollAttempts = 0;
    m_killUpgradeTimer->stop();
    m_portPollTimer->start();
}

void WebRenderBrowserProcess::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_portPollTimer->stop();
    m_killUpgradeTimer->stop();
    m_debugPort = 0;

    if (m_pendingStart) {
        // 旧进程已退出（含 stop 流程）：稍候让 profile 文件锁释放，再按待启动参数重启
        m_stopping = false;
        QTimer::singleShot(300, this, [this]() { launchPending(); });
        return;
    }

    const bool intentional = m_stopping;
    m_stopping = false;
    if (!intentional) {
        emit processDied(exitCode, status);
    }
}

void WebRenderBrowserProcess::onProcessError(QProcess::ProcessError error)
{
    if (m_stopping) {
        return;
    }
    if (error == QProcess::FailedToStart) {
        m_portPollTimer->stop();
        emit errorOccurred(QStringLiteral("浏览器进程启动失败"));
        emit startFailed();
    }
}

void WebRenderBrowserProcess::pollDevToolsPort()
{
    int port = 0;
    if (readDevToolsActivePort(port) && port > 0) {
        m_portPollTimer->stop();
        m_debugPort = port;
        emit ready(port);
        return;
    }

    ++m_portPollAttempts;
    if (m_portPollAttempts > 100) { // ~10s
        m_portPollTimer->stop();
        emit errorOccurred(QStringLiteral("等待 CDP 端口超时（DevToolsActivePort）"));
        stop();
    }
}

bool WebRenderBrowserProcess::readDevToolsActivePort(int& portOut) const
{
    const QString path = QDir(m_userDataDir).filePath(QStringLiteral("DevToolsActivePort"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    const QString line = stream.readLine().trimmed();
    bool ok = false;
    const int port = line.toInt(&ok);
    if (!ok || port <= 0) {
        return false;
    }
    portOut = port;
    return true;
}
