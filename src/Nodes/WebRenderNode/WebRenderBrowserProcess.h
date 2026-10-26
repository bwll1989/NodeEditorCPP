#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <optional>

/**
 * @file WebRenderBrowserProcess.h
 * @brief 独立 Chromium/Edge 进程（对齐 TD Web Render TOP 的进程隔离）
 *
 * 使用本机 Edge/Chrome 的 headless + remote debugging，
 * 通过 DevToolsActivePort 发现 CDP 端口后交由 WebRenderCdpClient 抓帧。
 *
 * 启停全部异步：terminate/kill 通过 finished 信号驱动，
 * 不调用 waitForFinished/waitForStarted，避免阻塞 GUI 线程。
 */
class WebRenderBrowserProcess final : public QObject
{
    Q_OBJECT

public:
    explicit WebRenderBrowserProcess(QObject* parent = nullptr);
    ~WebRenderBrowserProcess() override;

    /** @brief 查找本机 Edge / Chrome 可执行文件 */
    static QString findBrowserExecutable();

    bool isRunning() const;
    qint64 processId() const;
    int debugPort() const { return m_debugPort; }
    QString userDataDir() const { return m_userDataDir; }
    QString browserPath() const { return m_browserPath; }

    /**
     * @brief 启动 headless 浏览器（异步：参数校验同步返回，启动失败经信号上报）
     *
     * 若旧进程仍在运行，会先异步停止，待其退出后自动以此参数重启。
     * @param userDataDir 必须唯一（TD 要求每实例独立 user cache）
     * @param width/height 初始窗口尺寸
     * @param extraOptions 额外 Chromium 参数（空格分隔）
     */
    bool start(const QString& userDataDir, int width, int height, const QString& extraOptions = {});

    /** @brief 异步停止：立即返回，terminate 不响应时 2s 后升级 kill */
    void stop();

signals:
    /** @brief CDP 端口就绪（可读 DevToolsActivePort） */
    void ready(int debugPort);
    void processDied(int exitCode, QProcess::ExitStatus status);
    void errorOccurred(const QString& message);
    /** @brief 进程启动失败（FailedToStart），供模型回滚 Active 状态 */
    void startFailed();

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void pollDevToolsPort();

private:
    struct PendingStart {
        QString userDataDir;
        int width = 1280;
        int height = 720;
        QString extraOptions;
    };

    void requestStop();
    bool launchPending();
    bool launch(const QString& userDataDir, int width, int height, const QString& extraOptions);
    bool readDevToolsActivePort(int& portOut) const;

    QProcess* m_process = nullptr;
    QTimer* m_portPollTimer = nullptr;
    QTimer* m_killUpgradeTimer = nullptr; // terminate 不响应时 2s 后升级 kill
    QString m_browserPath;
    QString m_userDataDir;
    int m_debugPort = 0;
    int m_portPollAttempts = 0;
    bool m_stopping = false;
    std::optional<PendingStart> m_pendingStart;
};
