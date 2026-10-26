#pragma once

#include <QList>
#include <QMutex>
#include <QString>
#include <QThread>

#include <atomic>
#include <memory>

#include <opencv2/core.hpp>

/**
 * 屏幕捕获源。id 在会话之间保持稳定（显示器设备名 / 窗口 exe+类名+标题），
 * nativeHandle 只对当前会话有效（HMONITOR 或 HWND）。
 */
struct ScreenSource {
    enum class Kind { Display, Window };

    Kind kind = Kind::Display;
    QString id;
    QString label;
    quintptr nativeHandle = 0;
};

class ScreenCaptureThread : public QThread {
    Q_OBJECT

public:
    explicit ScreenCaptureThread(QObject* parent = nullptr);
    ~ScreenCaptureThread() override;

    static QList<ScreenSource> listDisplays();
    static QList<ScreenSource> listWindows();

    /** 按 id 重新解析句柄。窗口标题变化时，若 exe+类名只匹配到一个窗口则仍算命中。 */
    static bool resolve(ScreenSource& source);

    void startCapture(const ScreenSource& source, bool captureCursor, int fps);
    void setFps(int fps);
    void stop();

signals:
    void frameAvailable(const cv::Mat& frame);
    void captureStateChanged(bool capturing, const QString& method);
    void errorOccurred(const QString& message);

protected:
    void run() override;

private:
    struct WgcImpl;

    void publishFrame(cv::Mat frame);
    bool startWgc(const ScreenSource& source, bool captureCursor, QString& error);
    void stopWgc();
    void fallbackLoop(const ScreenSource& source);

    QMutex m_mutex;
    ScreenSource m_source;
    std::atomic<bool> m_abort{false};
    std::atomic<bool> m_captureCursor{true};
    std::atomic<int> m_fps{30};
    std::unique_ptr<WgcImpl> m_wgc;
};
