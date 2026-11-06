// 函数级注释：最小化 HTTP/2 (h2c) + gRPC 一元调用客户端
// - 基于 QTcpSocket 事件驱动，单线程，无外部 gRPC 依赖
// - 支持：连接前导/SETTINGS 握手、多路复用流、gRPC 5 字节消息分帧、
//         流量控制（连接级 + 流级）、PING/PONG、GOAWAY、RST_STREAM、
//         HPACK 解码（含 Huffman）、每调用超时、断线自动重连
// - 仅支持明文 h2c，不支持 TLS 与服务端推送
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <functional>

#include "GrpcHpack.h"

class QTcpSocket;

namespace Nodes {

class GrpcChannel : public QObject
{
    Q_OBJECT
public:
    // grpcStatus：gRPC 状态码（0 = OK）；grpcMessage：服务器消息（已解码）；
    // response：第一条响应消息原始字节（protobuf 序列化）
    using UnaryCallback = std::function<void(int grpcStatus, const QString &grpcMessage, const QByteArray &response)>;

    explicit GrpcChannel(QObject *parent = nullptr);
    ~GrpcChannel() override;

    void connectTo(const QString &host, quint16 port);
    void shutdown();
    bool isConnected() const { return m_connected; }

    // 发起一次一元调用；未完成 SETTINGS 握手前调用会排队，就绪后自动发出
    void call(const QString &path, const QByteArray &message,
              UnaryCallback cb, int timeoutMs = 5000);

signals:
    void connectedChanged(bool connected);

private:
    struct Stream
    {
        quint32 id = 0;            // HTTP/2 流 id
        QString path;              // gRPC 方法路径
        QByteArray body;           // gRPC 帧：1B 压缩标志 + 4B 长度 + 消息
        qsizetype bodySent = 0;
        bool headersSent = false;
        bool finished = false;
        bool queuedForSend = false;
        int httpStatus = 200;
        UnaryCallback cb;
        QTimer *timeoutTimer = nullptr;

        QByteArray recvBuffer;     // DATA 累积（按 gRPC 帧切分）
        QByteArray response;       // 第一条完整响应消息
        bool gotResponse = false;

        qint64 streamSendWindow = 65535;
    };

    void startConnect();
    void scheduleReconnect();
    void sendInitialSettings();
    void resetConnectionState();
    void abortConnection(const QString &reason);
    void failAllStreams(int status, const QString &message);
    void completeStream(quint32 streamId, int status, const QString &message,
                        const QByteArray &data, bool sendReset);

    void sendRequest(Stream *s);
    void sendStreamData(Stream *s);
    void queueSend(Stream *s);
    void flushSendQueue();

    void writeFrame(quint8 type, quint8 flags, quint32 streamId, const QByteArray &payload);
    void sendWindowUpdate(quint32 streamId, qint64 increment);

    void processFrame(quint8 type, quint8 flags, quint32 streamId, const QByteArray &payload);
    void onHeaders(quint32 streamId, bool endStream, const QList<GrpcHpack::Header> &headers);

    static quint32 readU32(const char *p);
    static void appendU24(QByteArray &out, quint32 v);
    static void appendU32(QByteArray &out, quint32 v);

    QTcpSocket *m_socket = nullptr;
    QByteArray m_readBuf;
    QString m_host;
    quint16 m_port = 0;

    bool m_connected = false;
    bool m_userShutdown = false;
    bool m_suppressReconnect = false;
    bool m_gotServerSettings = false;
    bool m_inHeadersBlock = false;
    quint32 m_headersStream = 0;
    bool m_headersEndStream = false;
    QByteArray m_headersBlock;

    quint32 m_nextStreamId = 1;
    QHash<quint32, Stream *> m_streams;

    GrpcHpack::Decoder *m_hpack = nullptr;

    qint64 m_connSendWindow = 65535;
    quint32 m_peerMaxFrameSize = 16384;
    qint64 m_peerInitialWindow = 65535;

    struct PendingData
    {
        quint32 streamId;
        QByteArray payload;
        bool endStream;
    };
    QList<PendingData> m_pendingData;

    QTimer *m_reconnectTimer = nullptr;
};

} // namespace Nodes
