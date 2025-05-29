#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QThreadPool>
#include <QMutex>
#include <QMap>

class WebSocketServer : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();

    void start(quint16 port);
    void stop();

signals:
    void newConnection(QWebSocket *socket);
    void messageReceived(QWebSocket *socket, const QByteArray &message);
    void connectionClosed(QWebSocket *socket);

private slots:
    void onNewConnection();
    void onMessageReceived(const QByteArray &message);
    void onSocketDisconnected();

private:
    QWebSocketServer *m_server;
    QMap<QWebSocket*, QThread*> m_clientThreads;
    QMutex m_mutex;
    QThreadPool m_threadPool;
};

#endif // WEBSOCKETSERVER_H