#pragma once

#include <QHash>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QVariantMap>

class QThread;
class QTimer;

#ifdef FTOMVController_LIBRARY
#define FTOMVController_EXPORT Q_DECL_EXPORT
#else
#define FTOMVController_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief 安卓播放器全局传输层（DLL 多线程单例，TCP Server）
 *
 * 职责：
 * - 监听客户端、粘包解帧
 * - 识别握手并登记设备 ID，定时发送心跳
 * - 将调用方传入的 JSON 按协议帧发出
 *
 * 不内置播放/停止等业务命令；具体 JSON 由上层传入。
 */
class FTOMVController_EXPORT FTOMVController : public QObject
{
    Q_OBJECT

public:
    static FTOMVController *instance();
    static FTOMVController *acquire();
    static void release();

    /**
     * @brief 启动 / 重启 TCP Server
     * @param host 绑定地址，默认任意地址
     * @param port 监听端口，默认 9090
     */
    Q_INVOKABLE void startServer(const QString &host = QStringLiteral("0.0.0.0"),
                                 quint16 port = 9090);

    /** @brief 停止 TCP Server */
    Q_INVOKABLE void stopServer();

    /**
     * @brief 向指定设备发送 JSON（自动加帧头/长度/typeDigit）
     * @param deviceId 已握手登记的设备 ID
     * @param json 业务 JSON（字段与取值由调用方决定）
     * @param typeDigit Payload 类型字符，默认 '1'
     */
    Q_INVOKABLE bool sendJson(int deviceId,
                              const QJsonObject &json,
                              char typeDigit = '1');

    /** @brief 向指定设备发送 JSON（QVariantMap 入口，便于 QML/脚本） */
    Q_INVOKABLE bool sendJson(int deviceId,
                              const QVariantMap &json,
                              char typeDigit = '1');

    /** @brief 向所有已登记设备广播 JSON */
    Q_INVOKABLE bool broadcastJson(const QJsonObject &json, char typeDigit = '1');

    /** @brief 向所有已登记设备广播 JSON（QVariantMap） */
    Q_INVOKABLE bool broadcastJson(const QVariantMap &json, char typeDigit = '1');

    /**
     * @brief 发送已组好的完整二进制帧（调用方自行负责协议封装时使用）
     */
    Q_INVOKABLE bool sendFrame(int deviceId, const QByteArray &frame);

    /** @brief 向指定设备发送握手/心跳帧 */
    Q_INVOKABLE bool handshake(int deviceId);

    /** @brief 向所有已登记设备广播握手/心跳 */
    Q_INVOKABLE void broadcastHandshake();

    Q_INVOKABLE QList<int> connectedDeviceIds() const;
    Q_INVOKABLE bool isListening() const;
    Q_INVOKABLE quint16 port() const;

signals:
    void isReady(bool ready);
    void clientConnected(const QString &peerHost);
    void deviceRegistered(int deviceId, const QString &peerHost);
    void clientDisconnected(int deviceId, const QString &peerHost);

    /**
     * @brief 收到一帧（握手与业务帧均上报）
     *
     * 常用字段：
     * - deviceId / host / isHandshake
     * - typeDigit / json(QVariantMap) / hex
     */
    void frameReceived(const QVariantMap &msg);

    /** @brief 同 frameReceived，兼容旧命名 */
    void recMsg(const QVariantMap &msg);

private:
    explicit FTOMVController(QObject *parent = nullptr);
    ~FTOMVController() override;

    FTOMVController(const FTOMVController &) = delete;
    FTOMVController &operator=(const FTOMVController &) = delete;

    void setupWorker();
    void teardownWorker();
    bool enqueueFrame(int deviceId, const QByteArray &frame);
    bool enqueueBroadcast(const QByteArray &frame);

    static QJsonObject toJsonObject(const QVariantMap &map);

    QObject *m_worker = nullptr;
    QThread *m_thread = nullptr;
    QTimer *m_keepAliveTimer = nullptr;

    mutable QMutex m_mutex;
    QHash<int, QString> m_deviceHosts;
    bool m_listening = false;
    quint16 m_port = 9090;
    QString m_host = QStringLiteral("0.0.0.0");

    static FTOMVController *s_instance;
    static int s_refCount;
    static QMutex s_refMutex;
};
