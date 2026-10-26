#include "WebRenderCdpClient.h"

#include <QAbstractSocket>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

WebRenderCdpClient::WebRenderCdpClient(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    m_frameThrottle.start();

    // 导航看门狗：流媒体/SPA/挂起脚本可能一直不触发 load 事件，
    // "Only Update when Loaded" 门控会让画面永久冻结——超时后强制恢复推流
    m_navWatchdog.setSingleShot(true);
    m_navWatchdog.setInterval(15000);
    connect(&m_navWatchdog, &QTimer::timeout, this, [this]() {
        if (isConnected() && m_capturing) {
            setLoaded(true);
            setError(QStringLiteral("页面加载超时，已恢复推流"));
        }
    });

    connect(m_socket, &QWebSocket::connected, this, &WebRenderCdpClient::onSocketConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebRenderCdpClient::onSocketDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebRenderCdpClient::onSocketTextMessage);
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this,
            &WebRenderCdpClient::onSocketError);
}

WebRenderCdpClient::~WebRenderCdpClient()
{
    disconnectFromDebugger();
}

bool WebRenderCdpClient::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void WebRenderCdpClient::connectToDebugger(int debugPort)
{
    disconnectFromDebugger();
    m_debugPort = debugPort;
    if (m_debugPort <= 0) {
        setError(QStringLiteral("无效的 CDP 端口"));
        return;
    }
    discoverTargets();
}

void WebRenderCdpClient::disconnectFromDebugger()
{
    setCapturing(false);
    m_navWatchdog.stop();
    m_pending.clear();
    m_pageEnabled = false;
    setLoaded(false);

    if (m_socket) {
        m_socket->blockSignals(true);
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->close();
        }
        m_socket->blockSignals(false);
    }
}

void WebRenderCdpClient::setViewport(int width, int height, double dpiScale)
{
    m_width = qMax(16, width);
    m_height = qMax(16, height);
    m_dpiScale = qBound(0.25, dpiScale, 4.0);
    if (isConnected() && m_pageEnabled) {
        applyViewport();
        // 分辨率变化后重启 screencast，使 maxWidth/maxHeight 生效
        if (m_capturing) {
            stopScreencast();
            startScreencast();
        }
    }
}

void WebRenderCdpClient::setMaxRenderRate(double fps)
{
    m_maxFps = qBound(0.5, fps, 120.0);
    m_minFrameIntervalMs = qMax(qint64(1), qint64(1000.0 / m_maxFps));
}

void WebRenderCdpClient::setJpegQuality(int quality)
{
    m_jpegQuality = qBound(1, quality, 100);
    if (m_capturing && m_screencastActive) {
        stopScreencast();
        startScreencast();
    }
}

void WebRenderCdpClient::navigate(const QString& url)
{
    const QString trimmed = url.trimmed();
    if (trimmed.isEmpty()) {
        setError(QStringLiteral("URL 为空"));
        return;
    }
    if (!isConnected()) {
        setError(QStringLiteral("CDP 未连接"));
        return;
    }

    setLoaded(false);
    m_currentUrl = trimmed;
    emit pageInfoChanged(m_currentUrl, m_pageTitle);
    m_navWatchdog.start();

    sendCommand(QStringLiteral("Page.navigate"),
                QJsonObject{{QStringLiteral("url"), trimmed}},
                [this](const QJsonObject&, const QString& error) {
                    if (!error.isEmpty()) {
                        setError(QStringLiteral("导航失败: %1").arg(error));
                    }
                });
}

void WebRenderCdpClient::loadHtml(const QString& html, const QString& cacheDir)
{
    if (cacheDir.isEmpty()) {
        setError(QStringLiteral("缓存目录无效，无法写入 HTML"));
        return;
    }
    QDir().mkpath(cacheDir);
    const QString path = QDir(cacheDir).filePath(QStringLiteral("webrender_source.html"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(QStringLiteral("无法写入 HTML 临时文件"));
        return;
    }
    file.write(html.toUtf8());
    file.close();

    navigate(QUrl::fromLocalFile(path).toString());
}

void WebRenderCdpClient::reload()
{
    if (!isConnected()) {
        return;
    }
    setLoaded(false);
    m_navWatchdog.start();
    sendCommand(QStringLiteral("Page.reload"),
                QJsonObject{{QStringLiteral("ignoreCache"), true}});
}

void WebRenderCdpClient::setCapturing(bool enabled)
{
    m_capturing = enabled;
    if (!enabled) {
        stopScreencast();
        return;
    }
    if (isConnected() && m_pageEnabled) {
        startScreencast();
    }
}

void WebRenderCdpClient::startScreencast()
{
    if (!isConnected() || !m_pageEnabled || m_screencastActive) {
        return;
    }

    // everyNthFrame：浏览器侧粗节流；精细节流在客户端按 Max FPS 丢帧
    const int everyNth = m_maxFps >= 50.0 ? 1 : (m_maxFps >= 25.0 ? 1 : 2);

    sendCommand(
        QStringLiteral("Page.startScreencast"),
        QJsonObject{
            {QStringLiteral("format"), QStringLiteral("jpeg")},
            {QStringLiteral("quality"), m_jpegQuality},
            {QStringLiteral("maxWidth"), m_width},
            {QStringLiteral("maxHeight"), m_height},
            {QStringLiteral("everyNthFrame"), everyNth},
        },
        [this](const QJsonObject&, const QString& error) {
            if (!error.isEmpty()) {
                setError(QStringLiteral("startScreencast 失败: %1").arg(error));
                m_screencastActive = false;
                return;
            }
            m_screencastActive = true;
            m_frameThrottle.restart();
        });
}

void WebRenderCdpClient::stopScreencast()
{
    if (!m_screencastActive) {
        return;
    }
    m_screencastActive = false;
    if (isConnected()) {
        sendCommand(QStringLiteral("Page.stopScreencast"));
    }
}

void WebRenderCdpClient::discoverTargets()
{
    const QUrl url(QStringLiteral("http://127.0.0.1:%1/json/list").arg(m_debugPort));
    QNetworkRequest req(url);

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setError(QStringLiteral("获取 CDP targets 失败: %1").arg(reply->errorString()));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            setError(QStringLiteral("CDP /json/list 响应无效"));
            return;
        }

        QString pageWs;
        QString anyWs;
        const QJsonArray arr = doc.array();
        for (const QJsonValue& v : arr) {
            const QJsonObject obj = v.toObject();
            const QString type = obj.value(QStringLiteral("type")).toString();
            const QString ws = obj.value(QStringLiteral("webSocketDebuggerUrl")).toString();
            if (ws.isEmpty()) {
                continue;
            }
            if (anyWs.isEmpty()) {
                anyWs = ws;
            }
            if (type == QLatin1String("page")) {
                pageWs = ws;
                break;
            }
        }

        const QString wsUrl = !pageWs.isEmpty() ? pageWs : anyWs;
        if (wsUrl.isEmpty()) {
            QTimer::singleShot(200, this, [this]() {
                if (m_debugPort > 0 && !isConnected()) {
                    discoverTargets();
                }
            });
            return;
        }

        m_socket->open(QUrl(wsUrl));
    });
}

void WebRenderCdpClient::onSocketConnected()
{
    m_lastError.clear();
    enablePageDomain();
    emit connected();
}

void WebRenderCdpClient::onSocketDisconnected()
{
    m_pageEnabled = false;
    m_screencastActive = false;
    m_capturing = false;
    setLoaded(false);
    emit disconnected();
}

void WebRenderCdpClient::onSocketError(QAbstractSocket::SocketError)
{
    setError(QStringLiteral("CDP WebSocket 错误: %1").arg(m_socket->errorString()));
}

void WebRenderCdpClient::onSocketTextMessage(const QString& message)
{
    // screencast 帧很大：先快速判断是否为事件，再 parse
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject root = doc.object();

    if (root.contains(QStringLiteral("id"))) {
        const int id = root.value(QStringLiteral("id")).toInt();
        const auto it = m_pending.find(id);
        if (it == m_pending.end()) {
            return;
        }
        ResponseHandler handler = it.value();
        m_pending.erase(it);

        QString error;
        if (root.contains(QStringLiteral("error"))) {
            error = root.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        }
        if (handler) {
            handler(root.value(QStringLiteral("result")).toObject(), error);
        }
        return;
    }

    const QString method = root.value(QStringLiteral("method")).toString();
    if (!method.isEmpty()) {
        handleEvent(method, root.value(QStringLiteral("params")).toObject());
    }
}

void WebRenderCdpClient::handleEvent(const QString& method, const QJsonObject& params)
{
    if (method == QLatin1String("Page.screencastFrame")) {
        const int sessionId = params.value(QStringLiteral("sessionId")).toInt();
        // 立刻 Ack，否则 Chromium 会停推（背压）
        sendCommand(QStringLiteral("Page.screencastFrameAck"),
                    QJsonObject{{QStringLiteral("sessionId"), sessionId}});

        if (!m_capturing) {
            return;
        }
        if (m_updateWhenLoaded && !m_loaded) {
            return;
        }
        // 客户端按 Max FPS 丢帧，避免解码/GPU 上传跟不上
        if (m_frameThrottle.isValid() && m_frameThrottle.elapsed() < m_minFrameIntervalMs) {
            return;
        }
        m_frameThrottle.restart();

        emitFrameFromBase64(params.value(QStringLiteral("data")).toString());
        return;
    }

    if (method == QLatin1String("Page.loadEventFired")
        || method == QLatin1String("Page.frameStoppedLoading")) {
        setLoaded(true);
        return;
    }

    if (method == QLatin1String("Page.frameStartedLoading")) {
        setLoaded(false);
        return;
    }

    if (method == QLatin1String("Page.navigatedWithinDocument")
        || method == QLatin1String("Page.frameNavigated")) {
        const QJsonObject frame = params.value(QStringLiteral("frame")).toObject();
        const QString url = frame.value(QStringLiteral("url")).toString();
        if (!url.isEmpty()) {
            m_currentUrl = url;
            emit pageInfoChanged(m_currentUrl, m_pageTitle);
        }
    }
}

void WebRenderCdpClient::emitFrameFromBase64(const QString& b64)
{
    if (b64.isEmpty()) {
        return;
    }
    const QByteArray bytes = QByteArray::fromBase64(b64.toLatin1());
    if (bytes.isEmpty()) {
        return;
    }
    ++m_handleUpdates;
    emit frameCaptured(bytes);
}

void WebRenderCdpClient::enablePageDomain()
{
    sendCommand(QStringLiteral("Page.enable"), {}, [this](const QJsonObject&, const QString& error) {
        if (!error.isEmpty()) {
            setError(QStringLiteral("Page.enable 失败: %1").arg(error));
            return;
        }
        m_pageEnabled = true;
        applyViewport();
        sendCommand(QStringLiteral("Emulation.setDefaultBackgroundColorOverride"),
                    QJsonObject{{QStringLiteral("color"),
                                 QJsonObject{{QStringLiteral("r"), 255},
                                             {QStringLiteral("g"), 255},
                                             {QStringLiteral("b"), 255},
                                             {QStringLiteral("a"), 255}}}});
        if (m_capturing) {
            startScreencast();
        }
    });
}

void WebRenderCdpClient::applyViewport()
{
    sendCommand(QStringLiteral("Emulation.setDeviceMetricsOverride"),
                QJsonObject{
                    {QStringLiteral("width"), m_width},
                    {QStringLiteral("height"), m_height},
                    {QStringLiteral("deviceScaleFactor"), m_dpiScale},
                    {QStringLiteral("mobile"), false},
                });
}

int WebRenderCdpClient::nextCommandId()
{
    return m_nextId++;
}

void WebRenderCdpClient::sendCommand(const QString& method, const QJsonObject& params, ResponseHandler handler)
{
    if (!isConnected()) {
        if (handler) {
            handler({}, QStringLiteral("not connected"));
        }
        return;
    }

    const int id = nextCommandId();
    if (handler) {
        m_pending.insert(id, std::move(handler));
    }

    QJsonObject msg;
    msg.insert(QStringLiteral("id"), id);
    msg.insert(QStringLiteral("method"), method);
    if (!params.isEmpty()) {
        msg.insert(QStringLiteral("params"), params);
    }
    m_socket->sendTextMessage(QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact)));
}

void WebRenderCdpClient::setLoaded(bool loaded)
{
    if (m_loaded == loaded) {
        return;
    }
    m_loaded = loaded;
    if (loaded) {
        m_navWatchdog.stop();
    }
    emit loadedChanged(m_loaded);
}

void WebRenderCdpClient::setError(const QString& message)
{
    m_lastError = message;
    emit errorOccurred(message);
}
