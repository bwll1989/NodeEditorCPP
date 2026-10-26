#pragma once

#include <QAbstractSocket>
#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <functional>

class QNetworkAccessManager;
class QWebSocket;

/**
 * @file WebRenderCdpClient.h
 * @brief CDP 客户端：Page.startScreencast（JPEG）连续推帧
 *
 * 比轮询 Page.captureScreenshot(PNG) 轻一个数量级：
 * - Chromium 侧 JPEG 编码 + 背压（screencastFrameAck）
 * - 客户端按 Max FPS 丢帧，避免解码/上传积压
 */
class WebRenderCdpClient final : public QObject
{
    Q_OBJECT

public:
    explicit WebRenderCdpClient(QObject* parent = nullptr);
    ~WebRenderCdpClient() override;

    bool isConnected() const;
    bool isLoaded() const { return m_loaded; }
    QString currentUrl() const { return m_currentUrl; }
    QString pageTitle() const { return m_pageTitle; }
    qint64 handleUpdates() const { return m_handleUpdates; }
    QString lastError() const { return m_lastError; }

    void connectToDebugger(int debugPort);
    void disconnectFromDebugger();

    void setViewport(int width, int height, double dpiScale = 1.0);
    void setMaxRenderRate(double fps);
    void setUpdateWhenLoaded(bool enabled) { m_updateWhenLoaded = enabled; }
    /** @brief JPEG quality 1–100，默认 70（体积/画质折中） */
    void setJpegQuality(int quality);

    void navigate(const QString& url);
    void loadHtml(const QString& html, const QString& cacheDir);
    void reload();
    void resetUpdateCount() { m_handleUpdates = 0; }

    void setCapturing(bool enabled);

signals:
    void connected();
    void disconnected();
    void loadedChanged(bool loaded);
    void pageInfoChanged(const QString& url, const QString& title);
    /** @brief JPEG（或兜底 PNG）原始字节，已 base64 解码 */
    void frameCaptured(const QByteArray& imageBytes);
    void errorOccurred(const QString& message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketTextMessage(const QString& message);
    void onSocketError(QAbstractSocket::SocketError error);
    void discoverTargets();

private:
    using ResponseHandler = std::function<void(const QJsonObject& result, const QString& error)>;

    int nextCommandId();
    void sendCommand(const QString& method,
                     const QJsonObject& params = {},
                     ResponseHandler handler = {});
    void handleEvent(const QString& method, const QJsonObject& params);
    void applyViewport();
    void startScreencast();
    void stopScreencast();
    void setLoaded(bool loaded);
    void setError(const QString& message);
    void enablePageDomain();
    void emitFrameFromBase64(const QString& b64);

    QNetworkAccessManager* m_nam = nullptr;
    QWebSocket* m_socket = nullptr;

    int m_debugPort = 0;
    int m_nextId = 1;
    QHash<int, ResponseHandler> m_pending;

    int m_width = 1280;
    int m_height = 720;
    double m_dpiScale = 1.0;
    double m_maxFps = 30.0;
    int m_jpegQuality = 70;
    bool m_updateWhenLoaded = true;
    bool m_capturing = false;
    bool m_screencastActive = false;
    bool m_loaded = false;
    bool m_pageEnabled = false;

    QString m_currentUrl;
    QString m_pageTitle;
    QString m_lastError;
    qint64 m_handleUpdates = 0;

    QElapsedTimer m_frameThrottle;
    qint64 m_minFrameIntervalMs = 33;
    /** @brief 导航看门狗：页面迟迟不触发 load 事件时强制恢复推流 */
    QTimer m_navWatchdog;
};
