/**
 * @file EcpClient.h
 * @brief Q-SYS External Control Protocol (ECP) 客户端
 *
 * TCP :1702，ASCII 命令，以 '\\n' (0x0A) 结束；响应可能带 '\\r'。
 * 文档：https://help.qsys.com/q-sys_9.5/Content/External_Control_APIs/ECP/ECP_Overview.htm
 */
#pragma once

#include <QAbstractSocket>
#include <QByteArray>
#include <QObject>
#include <QSignalBlocker>
#include <QStringList>
#include <QTcpSocket>
#include <QTimer>
#include <QVector>

namespace Nodes
{
    class EcpClient : public QObject
    {
        Q_OBJECT
    public:
        static constexpr quint16 kDefaultPort = 1702;
        static constexpr int kReconnectMs = 2000;
        static constexpr int kKeepAliveMs = 30000;

        explicit EcpClient(QObject *parent = nullptr)
            : QObject(parent)
        {
            m_socket = new QTcpSocket(this);
            connect(m_socket, &QTcpSocket::connected, this, &EcpClient::onConnected);
            connect(m_socket, &QTcpSocket::disconnected, this, &EcpClient::onDisconnected);
            connect(m_socket, &QTcpSocket::readyRead, this, &EcpClient::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            connect(m_socket, &QTcpSocket::errorOccurred, this, &EcpClient::onSocketError);
#else
            connect(m_socket,
                    QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                    this,
                    &EcpClient::onSocketError);
#endif

            m_reconnectTimer = new QTimer(this);
            m_reconnectTimer->setSingleShot(true);
            connect(m_reconnectTimer, &QTimer::timeout, this, &EcpClient::tryConnect);

            m_keepAliveTimer = new QTimer(this);
            m_keepAliveTimer->setInterval(kKeepAliveMs);
            connect(m_keepAliveTimer, &QTimer::timeout, this, [this]() {
                if (isConnected()) {
                    sendLine(QStringLiteral("sg"));
                }
            });
        }

        bool isConnected() const
        {
            return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
        }

        QString host() const { return m_host; }
        quint16 port() const { return m_port; }
        void setAutoReconnect(bool enabled) { m_autoReconnect = enabled; }

        void setEndpoint(const QString &host, quint16 port = kDefaultPort)
        {
            const QString h = host.trimmed();
            if (m_host == h && m_port == port) {
                return;
            }
            m_host = h;
            m_port = port;
            if (isConnected() || m_socket->state() == QAbstractSocket::ConnectingState) {
                disconnectFromHost();
                if (m_autoReconnect) {
                    scheduleReconnect();
                }
            }
        }

        void connectToHost()
        {
            m_wantConnected = true;
            tryConnect();
        }

        void disconnectFromHost()
        {
            const bool wasOpen = m_socket
                && m_socket->state() != QAbstractSocket::UnconnectedState;
            m_wantConnected = false;
            m_reconnectTimer->stop();
            m_keepAliveTimer->stop();
            if (wasOpen) {
                // 阻断 socket 自带 disconnected，避免再走 onDisconnected→scheduleReconnect
                const QSignalBlocker blocker(m_socket);
                m_socket->abort();
            }
            m_recvBuffer.clear();
            if (wasOpen) {
                emit connectedChanged(false);
            }
        }

        ~EcpClient() override
        {
            m_wantConnected = false;
            m_autoReconnect = false;
            if (m_reconnectTimer) {
                m_reconnectTimer->stop();
            }
            if (m_keepAliveTimer) {
                m_keepAliveTimer->stop();
            }
            if (m_socket) {
                const QSignalBlocker blocker(m_socket);
                if (m_socket->state() != QAbstractSocket::UnconnectedState) {
                    m_socket->abort();
                }
            }
            m_recvBuffer.clear();
        }

        /** 发送一行命令（自动追加 '\\n'）；CONTROL_ID 含空格时请先 quoteArg */
        bool sendLine(const QString &line)
        {
            if (!isConnected()) {
                return false;
            }
            QByteArray payload = line.toUtf8();
            if (!payload.endsWith('\n')) {
                payload.append('\n');
            }
            return m_socket->write(payload) >= 0;
        }

        static QString quoteArg(const QString &s)
        {
            bool needQuote = s.isEmpty();
            for (QChar c : s) {
                if (c.isSpace() || c == QLatin1Char('"') || c == QLatin1Char('\\')) {
                    needQuote = true;
                    break;
                }
            }
            if (!needQuote) {
                return s;
            }
            QString out;
            out.reserve(s.size() + 2);
            out.append(QLatin1Char('"'));
            for (QChar c : s) {
                if (c == QLatin1Char('\\') || c == QLatin1Char('"')) {
                    out.append(QLatin1Char('\\'));
                    out.append(c);
                } else if (c == QLatin1Char('\n')) {
                    out.append(QStringLiteral("\\n"));
                } else if (c == QLatin1Char('\r')) {
                    out.append(QStringLiteral("\\r"));
                } else {
                    out.append(c);
                }
            }
            out.append(QLatin1Char('"'));
            return out;
        }

        /** 将一行 ECP 响应拆成参数（支持引号与 \\ 转义） */
        static QStringList tokenize(const QString &line)
        {
            QStringList tokens;
            const QString s = line.trimmed();
            int i = 0;
            const int n = s.size();
            while (i < n) {
                while (i < n && s.at(i).isSpace()) {
                    ++i;
                }
                if (i >= n) {
                    break;
                }
                if (s.at(i) == QLatin1Char('"')) {
                    ++i;
                    QString tok;
                    while (i < n) {
                        const QChar c = s.at(i++);
                        if (c == QLatin1Char('\\') && i < n) {
                            const QChar e = s.at(i++);
                            if (e == QLatin1Char('n')) {
                                tok.append(QLatin1Char('\n'));
                            } else if (e == QLatin1Char('r')) {
                                tok.append(QLatin1Char('\r'));
                            } else {
                                tok.append(e);
                            }
                        } else if (c == QLatin1Char('"')) {
                            break;
                        } else {
                            tok.append(c);
                        }
                    }
                    tokens.append(tok);
                } else {
                    const int start = i;
                    while (i < n && !s.at(i).isSpace()) {
                        ++i;
                    }
                    tokens.append(s.mid(start, i - start));
                }
            }
            return tokens;
        }

    Q_SIGNALS:
        void connectedChanged(bool connected);
        void socketError(const QString &message);
        /** 已按行拆好的响应（不含换行） */
        void lineReceived(const QString &line);

    private:
        void tryConnect()
        {
            if (!m_wantConnected) {
                return;
            }
            if (m_host.isEmpty()) {
                emit socketError(QStringLiteral("empty host"));
                return;
            }
            if (m_socket->state() == QAbstractSocket::ConnectedState
                || m_socket->state() == QAbstractSocket::ConnectingState) {
                return;
            }
            m_recvBuffer.clear();
            m_socket->connectToHost(m_host, m_port);
        }

        void scheduleReconnect()
        {
            if (!m_wantConnected || !m_autoReconnect) {
                return;
            }
            if (!m_reconnectTimer->isActive()) {
                m_reconnectTimer->start(kReconnectMs);
            }
        }

        void onConnected()
        {
            m_recvBuffer.clear();
            m_keepAliveTimer->start();
            emit connectedChanged(true);
        }

        void onDisconnected()
        {
            m_keepAliveTimer->stop();
            m_recvBuffer.clear();
            emit connectedChanged(false);
            scheduleReconnect();
        }

        void onSocketError(QAbstractSocket::SocketError)
        {
            emit socketError(m_socket->errorString());
            if (m_socket->state() == QAbstractSocket::UnconnectedState) {
                scheduleReconnect();
            }
        }

        void onReadyRead()
        {
            m_recvBuffer.append(m_socket->readAll());
            int nl = m_recvBuffer.indexOf('\n');
            while (nl >= 0) {
                QByteArray raw = m_recvBuffer.left(nl);
                m_recvBuffer.remove(0, nl + 1);
                if (raw.endsWith('\r')) {
                    raw.chop(1);
                }
                if (!raw.isEmpty()) {
                    emit lineReceived(QString::fromUtf8(raw));
                }
                nl = m_recvBuffer.indexOf('\n');
            }
            if (m_recvBuffer.size() > 1024 * 1024) {
                m_recvBuffer.clear();
                emit socketError(QStringLiteral("receive buffer overflow"));
            }
        }

        QTcpSocket *m_socket = nullptr;
        QTimer *m_reconnectTimer = nullptr;
        QTimer *m_keepAliveTimer = nullptr;

        QString m_host;
        quint16 m_port = kDefaultPort;
        bool m_wantConnected = false;
        bool m_autoReconnect = true;
        QByteArray m_recvBuffer;
    };
}
