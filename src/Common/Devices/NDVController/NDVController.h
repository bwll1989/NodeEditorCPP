/**
 * @file NDVController.h
 * @brief NDV 全局 TCP 服务端单例（握手/心跳/组包/状态解析）
 *
 * 生命周期对齐 ArtnetTransmitter：
 * - getInstance() 进程内常驻，节点析构不 delete 单例
 * - 自身 moveToThread，网络对象在工作线程创建
 * - 进程退出交给 OS 回收，避免退出期主动拆套接字触发 __fastfail
 */
#pragma once

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QDateTime>
#include <QTimer>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVariantMap>

#ifdef NDVController_LIBRARY
#define NDVController_EXPORT Q_DECL_EXPORT
#else
#define NDVController_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief 单个 NDV 客户端运行时信息
 */
struct NDVController_EXPORT NDVClientInfo
{
    int deviceId = -1;
    QString ipAddress;
    bool online = false;
    QString state = QStringLiteral("Unknown"); /**< Connected / Playing / Stopped / Unknown */
    QDateTime lastSeen;
    QDateTime lastHandshake;
    QDateTime lastProgressAt;      /**< 最近一次 0E（仅用于停播超时，不解析进度值） */
    QDateTime ignoreProgressUntil; /**< 本端 stop 后短时忽略 0E，避免回弹为 Playing */
};

Q_DECLARE_METATYPE(NDVClientInfo)

class NDVController_EXPORT NDVController : public QObject
{
    Q_OBJECT

public:
    /** 对齐 ArtnetTransmitter::getInstance：进程内唯一，永不因节点析构而销毁 */
    static NDVController *getInstance();
    /** 兼容旧 API，等同 getInstance() */
    static NDVController *instance();
    static NDVController *acquire();
    /** 兼容旧 API：仅保留接口，不销毁单例 */
    static void release();
    /** 进程 aboutToQuit 时停止定时器并阻断信号，避免向已销毁节点投递 */
    static void prepareForAppExit();

    static constexpr int FIXED_PORT = 9008;

    bool isListening() const { return m_listening; }

    Q_INVOKABLE bool isOnline(int deviceId) const;
    Q_INVOKABLE NDVClientInfo clientInfo(int deviceId) const;
    QList<NDVClientInfo> clients() const;

    /**
     * @brief 向指定设备发送控制指令（可从任意线程调用，内部投递到工作线程）
     */
    Q_INVOKABLE bool sendCommand(const QString &type, int fileIndex, int targetId);

Q_SIGNALS:
    void listeningChanged(bool ready);
    void clientListChanged();
    void clientStatusChanged(int deviceId, const NDVClientInfo &info);
    void sendCommandRequested(const QString &type, int fileIndex, int targetId);

private Q_SLOTS:
    void initializeServer();
    void silenceForExit();
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void sendHeartbeatToAll();
    void checkPlayingTimeout();
    void handleSendCommand(const QString &type, int fileIndex, int targetId);

private:
    explicit NDVController(QObject *parent = nullptr);
    ~NDVController() override;

    NDVController(const NDVController &) = delete;
    NDVController &operator=(const NDVController &) = delete;

    void upsertClient(const NDVClientInfo &info, bool emitStatus);
    void handleFrame(QTcpSocket *socket, const QByteArray &frame);
    void processBytes(QTcpSocket *socket, const QByteArray &bytes);
    QByteArray commandOpcode(const QString &type) const;
    QByteArray buildCommand(const QByteArray &opcode, int fileIndex, int targetId) const;
    bool sendRawToId(int targetId, const QByteArray &packet);
    bool sendRawToIp(const QString &ip, const QByteArray &packet);
    bool sendCommandImpl(const QString &type, int fileIndex, int targetId);
    static QString normalizeIp(const QString &ip);
    static QString peerHostOf(const QTcpSocket *socket);
    static bool hostMatches(const QString &peer, const QString &target);

    QThread *m_thread = nullptr;
    QTcpServer *m_server = nullptr;
    QList<QTcpSocket *> m_clientSockets;
    QTimer *m_heartbeatTimer = nullptr;
    QTimer *m_playingWatchTimer = nullptr;
    bool m_listening = false;
    QMap<int, NDVClientInfo> m_clients;
    mutable QMutex m_mutex;

    static NDVController *s_instance;
    static QMutex s_mutex;
};
