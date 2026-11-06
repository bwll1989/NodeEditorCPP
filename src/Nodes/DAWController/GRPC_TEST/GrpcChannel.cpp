#include "GrpcChannel.h"

#include "GrpcHpack.h"

#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>

namespace Nodes {

namespace {

constexpr char kClientPreface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

constexpr quint8 FRAME_DATA = 0x0;
constexpr quint8 FRAME_HEADERS = 0x1;
constexpr quint8 FRAME_RST_STREAM = 0x3;
constexpr quint8 FRAME_SETTINGS = 0x4;
constexpr quint8 FRAME_PING = 0x6;
constexpr quint8 FRAME_GOAWAY = 0x7;
constexpr quint8 FRAME_WINDOW_UPDATE = 0x8;
constexpr quint8 FRAME_CONTINUATION = 0x9;

constexpr quint8 FLAG_ACK = 0x1;
constexpr quint8 FLAG_END_STREAM = 0x1;
constexpr quint8 FLAG_END_HEADERS = 0x4;
constexpr quint8 FLAG_PADDED = 0x8;
constexpr quint8 FLAG_PRIORITY = 0x20;

constexpr quint16 SETTING_INITIAL_WINDOW_SIZE = 0x4;
constexpr quint16 SETTING_MAX_FRAME_SIZE = 0x5;

constexpr quint32 ERROR_CANCEL = 8;

// gRPC 规范中的 HTTP 状态码映射
int grpcStatusFromHttp(int httpStatus)
{
    switch (httpStatus) {
    case 400: return 13; // INTERNAL
    case 401: return 16; // UNAUTHENTICATED
    case 403: return 7;  // PERMISSION_DENIED
    case 404: return 14; // UNAVAILABLE
    case 429:
    case 502:
    case 503:
    case 504: return 14; // UNAVAILABLE
    default: return 2;   // UNKNOWN
    }
}

// grpc-message 采用百分号编码（字节层面），先解字节再按 UTF-8 转换
QString decodeGrpcMessage(const QByteArray &raw)
{
    QByteArray out;
    out.reserve(raw.size());
    for (int i = 0; i < raw.size(); ++i) {
        if (raw.at(i) == '%' && i + 2 < raw.size()) {
            bool ok = false;
            const int hex = raw.mid(i + 1, 2).toInt(&ok, 16);
            if (ok) {
                out.append(char(hex));
                i += 2;
                continue;
            }
        }
        out.append(raw.at(i));
    }
    return QString::fromUtf8(out);
}

} // namespace

GrpcChannel::GrpcChannel(QObject *parent)
    : QObject(parent)
    , m_hpack(new GrpcHpack::Decoder(4096))
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        sendInitialSettings();
    });
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        failAllStreams(14, QStringLiteral("connection closed"));
        resetConnectionState();
        if (!m_userShutdown && !m_suppressReconnect) {
            scheduleReconnect();
        }
    });
    connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
        m_readBuf += m_socket->readAll();
        while (true) {
            if (m_readBuf.size() < 9) {
                break;
            }
            const quint32 len = (quint32(quint8(m_readBuf.at(0))) << 16)
                | (quint32(quint8(m_readBuf.at(1))) << 8)
                | quint32(quint8(m_readBuf.at(2)));
            if (m_readBuf.size() < 9 + int(len)) {
                break;
            }
            const quint8 type = quint8(m_readBuf.at(3));
            const quint8 flags = quint8(m_readBuf.at(4));
            const quint32 streamId = readU32(m_readBuf.constData() + 5) & 0x7FFFFFFFu;
            const QByteArray payload = m_readBuf.mid(9, int(len));
            m_readBuf.remove(0, 9 + int(len));
            processFrame(type, flags, streamId, payload);
        }
    });
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qWarning() << "[GrpcChannel] socket error:" << int(error) << m_socket->errorString();
        if (m_connected) {
            return; // disconnected 信号统一收尾
        }
        failAllStreams(14, m_socket->errorString());
        resetConnectionState();
        if (!m_userShutdown && !m_suppressReconnect) {
            scheduleReconnect();
        }
    });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(1000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &GrpcChannel::startConnect);
}

GrpcChannel::~GrpcChannel()
{
    m_userShutdown = true;
    failAllStreams(14, QStringLiteral("channel destroyed"));
    delete m_hpack;
    m_hpack = nullptr;
}

void GrpcChannel::connectTo(const QString &host, quint16 port)
{
    m_userShutdown = false;
    m_host = host;
    m_port = port;
    m_reconnectTimer->stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_suppressReconnect = true;
        m_socket->abort(); // 丢弃旧连接（可能触发同步的 disconnected）
        m_suppressReconnect = false;
    }
    startConnect();
}

void GrpcChannel::shutdown()
{
    m_userShutdown = true;
    m_reconnectTimer->stop();
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void GrpcChannel::startConnect()
{
    if (m_host.isEmpty() || m_port == 0) {
        return;
    }
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_socket->connectToHost(m_host, m_port);
    }
}

void GrpcChannel::scheduleReconnect()
{
    if (!m_userShutdown && !m_host.isEmpty()) {
        qWarning() << "[GrpcChannel] reconnect scheduled:" << m_host << m_port;
        m_reconnectTimer->start();
    }
}

void GrpcChannel::resetConnectionState()
{
    m_connected = false;
    m_gotServerSettings = false;
    m_inHeadersBlock = false;
    m_headersBlock.clear();
    m_readBuf.clear();
    qDeleteAll(m_streams);
    m_streams.clear();
    m_pendingData.clear();
    m_connSendWindow = 65535;
    m_peerMaxFrameSize = 16384;
    m_peerInitialWindow = 65535;
    m_nextStreamId = 1;
}

void GrpcChannel::abortConnection(const QString &reason)
{
    failAllStreams(13, QStringLiteral("protocol error: %1").arg(reason));
    m_socket->abort();
    resetConnectionState();
    if (!m_userShutdown && !m_suppressReconnect) {
        scheduleReconnect();
    }
}

void GrpcChannel::call(const QString &path, const QByteArray &message,
                       UnaryCallback cb, int timeoutMs)
{
    if (path.isEmpty() || !cb) {
        return;
    }
    auto *s = new Stream;
    s->id = m_nextStreamId;
    m_nextStreamId += 2;
    s->path = path;
    s->cb = std::move(cb);
    s->streamSendWindow = m_peerInitialWindow;

    // gRPC 消息帧：1 字节压缩标志（0）+ 4 字节大端长度 + 消息体
    s->body.reserve(5 + message.size());
    s->body.append(char(0)); // 压缩标志：未压缩
    appendU32(s->body, quint32(message.size()));
    s->body.append(message);

    m_streams.insert(s->id, s);

    if (timeoutMs > 0) {
        auto *timer = new QTimer(this); // 挂在 channel 上，流销毁时停止
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, id = s->id, timeoutMs]() {
            completeStream(id, 4, QStringLiteral("deadline exceeded"), QByteArray(), true);
        });
        timer->start(timeoutMs);
        s->timeoutTimer = timer;
    }

    if (m_connected) {
        sendRequest(s);
    }
}

void GrpcChannel::sendInitialSettings()
{
    // HTTP/2 客户端连接前导 = 固定魔数 + SETTINGS 帧（RFC 7540 §3.5）
    if (m_socket) {
        m_socket->write(kClientPreface, int(sizeof(kClientPreface) - 1));
    }
    writeFrame(FRAME_SETTINGS, 0, 0, QByteArray());
}

void GrpcChannel::sendRequest(Stream *s)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState || s->headersSent) {
        return;
    }
    s->headersSent = true;

    QList<GrpcHpack::Header> headers;
    headers.append(GrpcHpack::Header(":method", "POST"));
    headers.append(GrpcHpack::Header(":scheme", "http"));
    headers.append(GrpcHpack::Header(":path", s->path.toUtf8()));
    headers.append(GrpcHpack::Header(":authority",
                                     QString::fromLatin1("%1:%2").arg(m_host).arg(m_port).toUtf8()));
    headers.append(GrpcHpack::Header("content-type", "application/grpc"));
    headers.append(GrpcHpack::Header("te", "trailers"));

    writeFrame(FRAME_HEADERS, FLAG_END_HEADERS, s->id, GrpcHpack::encodeHeaderBlock(headers));
    sendStreamData(s);
}

void GrpcChannel::sendStreamData(Stream *s)
{
    if (s->finished) {
        return;
    }
    while (s->bodySent < s->body.size()) {
        const qint64 remaining = s->body.size() - s->bodySent;
        qint64 chunk = qMin<qint64>(m_peerMaxFrameSize, remaining);
        chunk = qMin(chunk, m_connSendWindow);
        chunk = qMin(chunk, s->streamSendWindow);
        if (chunk <= 0) {
            queueSend(s);
            return;
        }
        const bool last = (chunk == remaining);
        writeFrame(FRAME_DATA, last ? FLAG_END_STREAM : quint8(0), s->id,
                   s->body.mid(int(s->bodySent), int(chunk)));
        s->bodySent += chunk;
        m_connSendWindow -= chunk;
        s->streamSendWindow -= chunk;
    }
    if (s->body.isEmpty()) {
        // 空请求也必须以 END_STREAM 结束
        writeFrame(FRAME_DATA, FLAG_END_STREAM, s->id, QByteArray());
    }
}

void GrpcChannel::queueSend(Stream *s)
{
    if (s->queuedForSend) {
        return;
    }
    s->queuedForSend = true;
    qsizetype offset = s->bodySent;
    if (s->body.isEmpty()) {
        PendingData p;
        p.streamId = s->id;
        p.endStream = true;
        m_pendingData.append(p);
        return;
    }
    while (offset < s->body.size()) {
        const qint64 chunk = qMin<qint64>(m_peerMaxFrameSize, s->body.size() - offset);
        PendingData p;
        p.streamId = s->id;
        p.payload = s->body.mid(int(offset), int(chunk));
        p.endStream = (offset + chunk >= s->body.size());
        m_pendingData.append(p);
        offset += chunk;
    }
}

void GrpcChannel::flushSendQueue()
{
    while (!m_pendingData.isEmpty()) {
        const PendingData p = m_pendingData.first();
        Stream *s = m_streams.value(p.streamId, nullptr);
        if (!s || s->finished) {
            m_pendingData.removeFirst();
            continue;
        }
        if (p.payload.size() > m_connSendWindow || p.payload.size() > s->streamSendWindow) {
            return; // 窗口不足，等待 WINDOW_UPDATE
        }
        m_pendingData.removeFirst();
        writeFrame(FRAME_DATA, p.endStream ? FLAG_END_STREAM : quint8(0), p.streamId, p.payload);
        m_connSendWindow -= p.payload.size();
        s->streamSendWindow -= p.payload.size();
        if (p.endStream) {
            s->bodySent = s->body.size();
            s->queuedForSend = false;
        }
    }
}

void GrpcChannel::writeFrame(quint8 type, quint8 flags, quint32 streamId, const QByteArray &payload)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    QByteArray header;
    appendU24(header, quint32(payload.size()));
    header.append(char(type));
    header.append(char(flags));
    appendU32(header, streamId & 0x7FFFFFFFu);
    m_socket->write(header);
    if (!payload.isEmpty()) {
        m_socket->write(payload);
    }
}

void GrpcChannel::sendWindowUpdate(quint32 streamId, qint64 increment)
{
    if (increment <= 0) {
        return;
    }
    QByteArray payload;
    appendU32(payload, quint32(increment));
    writeFrame(FRAME_WINDOW_UPDATE, 0, streamId, payload);
}

quint32 GrpcChannel::readU32(const char *p)
{
    return (quint32(quint8(p[0])) << 24) | (quint32(quint8(p[1])) << 16)
        | (quint32(quint8(p[2])) << 8) | quint32(quint8(p[3]));
}

void GrpcChannel::appendU24(QByteArray &out, quint32 v)
{
    out.append(char((v >> 16) & 0xFF));
    out.append(char((v >> 8) & 0xFF));
    out.append(char(v & 0xFF));
}

void GrpcChannel::appendU32(QByteArray &out, quint32 v)
{
    out.append(char((v >> 24) & 0xFF));
    out.append(char((v >> 16) & 0xFF));
    out.append(char((v >> 8) & 0xFF));
    out.append(char(v & 0xFF));
}

void GrpcChannel::processFrame(quint8 type, quint8 flags, quint32 streamId, const QByteArray &payload)
{
    switch (type) {
    case FRAME_SETTINGS:
        if (flags & FLAG_ACK) {
            return;
        }
        m_gotServerSettings = true;
        for (int i = 0; i + 6 <= payload.size(); i += 6) {
            const quint16 id = quint16((quint8(payload.at(i)) << 8) | quint8(payload.at(i + 1)));
            const quint32 value = readU32(payload.constData() + i + 2);
            if (id == SETTING_INITIAL_WINDOW_SIZE) {
                const qint64 delta = qint64(value) - m_peerInitialWindow;
                m_peerInitialWindow = qint64(value);
                for (Stream *s : m_streams) {
                    s->streamSendWindow += delta;
                }
            } else if (id == SETTING_MAX_FRAME_SIZE) {
                m_peerMaxFrameSize = qBound<quint32>(16384u, value, 16777215u);
            }
        }
        writeFrame(FRAME_SETTINGS, FLAG_ACK, 0, QByteArray());
        if (!m_connected) {
            m_connected = true;
            emit connectedChanged(true);
            // 补发在连接完成前排队的调用
            const QList<Stream *> waiting = m_streams.values();
            for (Stream *s : waiting) {
                if (!s->headersSent) {
                    sendRequest(s);
                }
            }
        }
        flushSendQueue();
        return;

    case FRAME_HEADERS: {
        if (m_inHeadersBlock) {
            abortConnection(QStringLiteral("unexpected HEADERS inside header block"));
            return;
        }
        int offset = 0;
        int padLen = 0;
        if (flags & FLAG_PADDED) {
            if (payload.isEmpty()) {
                abortConnection(QStringLiteral("bad HEADERS padding"));
                return;
            }
            padLen = quint8(payload.at(0));
            offset = 1;
        }
        if (flags & FLAG_PRIORITY) {
            offset += 5; // 流依赖(4B) + 权重(1B)
        }
        int end = payload.size();
        if (flags & FLAG_PADDED) {
            end -= padLen;
        }
        if (end < offset) {
            abortConnection(QStringLiteral("bad HEADERS padding"));
            return;
        }
        const QByteArray block = payload.mid(offset, end - offset);
        if (flags & FLAG_END_HEADERS) {
            QList<GrpcHpack::Header> headers;
            if (!m_hpack->decode(block, headers)) {
                abortConnection(QStringLiteral("HPACK decode failed"));
                return;
            }
            onHeaders(streamId, (flags & FLAG_END_STREAM) != 0, headers);
        } else {
            m_inHeadersBlock = true;
            m_headersStream = streamId;
            m_headersEndStream = (flags & FLAG_END_STREAM) != 0;
            m_headersBlock = block;
        }
        return;
    }

    case FRAME_CONTINUATION:
        if (!m_inHeadersBlock || m_headersStream != streamId) {
            abortConnection(QStringLiteral("unexpected CONTINUATION"));
            return;
        }
        m_headersBlock += payload;
        if (flags & FLAG_END_HEADERS) {
            m_inHeadersBlock = false;
            QList<GrpcHpack::Header> headers;
            if (!m_hpack->decode(m_headersBlock, headers)) {
                abortConnection(QStringLiteral("HPACK decode failed"));
                return;
            }
            const bool endStream = m_headersEndStream;
            m_headersBlock.clear();
            onHeaders(streamId, endStream, headers);
        }
        return;

    case FRAME_DATA: {
        Stream *s = m_streams.value(streamId, nullptr);
        if (!s || s->finished) {
            return;
        }
        QByteArray data = payload;
        if (flags & FLAG_PADDED) {
            if (data.isEmpty()) {
                abortConnection(QStringLiteral("bad DATA padding"));
                return;
            }
            const int padLen = quint8(data.at(0));
            data.remove(0, 1);
            if (padLen > data.size()) {
                abortConnection(QStringLiteral("bad DATA padding"));
                return;
            }
            data.truncate(data.size() - padLen);
        }
        // 及时归还接收窗口
        sendWindowUpdate(0, qint64(payload.size()));
        sendWindowUpdate(streamId, qint64(payload.size()));
        if (data.isEmpty()) {
            return;
        }
        s->recvBuffer += data;
        while (true) {
            if (s->recvBuffer.size() < 5) {
                break;
            }
            const quint32 msgLen = readU32(s->recvBuffer.constData() + 1);
            if (msgLen > (32u << 20)) {
                completeStream(s->id, 8, QStringLiteral("response message too large"), QByteArray(), true);
                return;
            }
            if (s->recvBuffer.size() < 5 + int(msgLen)) {
                break;
            }
            if (quint8(s->recvBuffer.at(0)) != 0) {
                completeStream(s->id, 12, QStringLiteral("compressed response not supported"), QByteArray(), true);
                return;
            }
            const QByteArray msg = s->recvBuffer.mid(5, int(msgLen));
            s->recvBuffer.remove(0, 5 + int(msgLen));
            if (!s->gotResponse) {
                s->gotResponse = true;
                s->response = msg;
            }
        }
        return;
    }

    case FRAME_RST_STREAM: {
        Stream *s = m_streams.value(streamId, nullptr);
        if (s && !s->finished) {
            const quint32 code = payload.size() >= 4 ? readU32(payload.constData()) : 2u;
            completeStream(streamId, 2, QStringLiteral("stream reset by peer (error %1)").arg(code), QByteArray(), false);
        }
        return;
    }

    case FRAME_WINDOW_UPDATE: {
        if (payload.size() < 4) {
            return;
        }
        const qint64 inc = qint64(readU32(payload.constData()) & 0x7FFFFFFFu);
        if (streamId == 0) {
            m_connSendWindow += inc;
            flushSendQueue();
        } else if (Stream *s = m_streams.value(streamId, nullptr)) {
            s->streamSendWindow += inc;
            flushSendQueue();
        }
        return;
    }

    case FRAME_PING:
        if (!(flags & FLAG_ACK)) {
            writeFrame(FRAME_PING, FLAG_ACK, 0, payload);
        }
        return;

    case FRAME_GOAWAY:
        if (payload.size() >= 8) {
            qWarning() << "[GrpcChannel] GOAWAY lastStreamId:"
                       << (readU32(payload.constData()) & 0x7FFFFFFFu)
                       << "errorCode:" << readU32(payload.constData() + 4);
        }
        failAllStreams(14, QStringLiteral("goaway received"));
        m_socket->disconnectFromHost();
        return;

    default:
        return;
    }
}

void GrpcChannel::onHeaders(quint32 streamId, bool endStream, const QList<GrpcHpack::Header> &headers)
{
    int httpStatus = 0;
    QByteArray grpcStatusRaw;
    QByteArray grpcMessageRaw;

    for (const auto &h : headers) {
        if (h.first == ":status") {
            httpStatus = h.second.toInt();
        } else if (h.first == "grpc-status") {
            grpcStatusRaw = h.second;
        } else if (h.first == "grpc-message") {
            grpcMessageRaw = h.second;
        }
    }

    Stream *s = m_streams.value(streamId, nullptr);
    if (!s || s->finished) {
        return;
    }

    if (endStream) {
        // trailers（或 trailers-only 响应）
        if (!grpcStatusRaw.isEmpty()) {
            bool ok = false;
            const int status = grpcStatusRaw.toInt(&ok);
            completeStream(streamId, ok ? status : 2, decodeGrpcMessage(grpcMessageRaw), s->response, false);
        } else if (httpStatus == 200) {
            // 缺 grpc-status：无法确认服务端语义，按内部错误处理
            completeStream(streamId, 13, QStringLiteral("missing grpc-status in trailers"), QByteArray(), false);
        } else {
            completeStream(streamId, grpcStatusFromHttp(httpStatus),
                           QStringLiteral("HTTP %1").arg(httpStatus), QByteArray(), false);
        }
        return;
    }

    // initial 响应头：仅记录 HTTP 状态，等待 DATA 与 trailers
    s->httpStatus = httpStatus;
}

void GrpcChannel::completeStream(quint32 streamId, int status, const QString &message,
                                 const QByteArray &data, bool sendReset)
{
    Stream *s = m_streams.value(streamId, nullptr);
    if (!s || s->finished) {
        return;
    }
    m_streams.remove(streamId);
    s->finished = true;
    if (sendReset && m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray payload;
        appendU32(payload, ERROR_CANCEL);
        writeFrame(FRAME_RST_STREAM, 0, streamId, payload);
    }
    if (s->timeoutTimer) {
        s->timeoutTimer->stop();
        s->timeoutTimer->deleteLater();
        s->timeoutTimer = nullptr;
    }
    if (s->cb) {
        s->cb(status, message, data);
    }
    delete s; // 回调可能触发新调用，但不允许再访问 s 本身
}

void GrpcChannel::failAllStreams(int status, const QString &message)
{
    const QList<quint32> ids = m_streams.keys();
    for (const quint32 id : ids) {
        completeStream(id, status, message, QByteArray(), false);
    }
    m_pendingData.clear();
}

} // namespace Nodes
