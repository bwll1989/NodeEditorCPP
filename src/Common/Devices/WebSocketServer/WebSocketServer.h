#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QThreadPool>
#include <QMutex>
#include <QMap>
#include <QThread>


class WebSocketServer : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();

    void start(quint16 port);
    void stop();
    void broadcastMessage(const QByteArray &message,const int &messageType = 0);
    void initialize(quint16 port);
    void cleanup();
    void setPort(quint16 port);

signals:
    void newConnection(QWebSocket *socket);
    void messageReceived(QWebSocket *socket, const QByteArray &message);
    void connectionClosed(QWebSocket *socket);
    void initializeRequested(quint16 port);
    void cleanupRequested();
    void broadcastRequested(const QByteArray &message,const int &messageType = 0);

private:
    QWebSocketServer *m_server;
    QMap<QWebSocket*, QThread*> m_clientThreads;
    QMutex m_mutex;
    QThreadPool m_threadPool;
    QThread *mThread = nullptr;
    quint16 mPort = 2003;
};

class WebSocketWorker : public QObject {
    Q_OBJECT
public:
    explicit WebSocketWorker(QObject *parent = nullptr);
    ~WebSocketWorker();

public slots:
    void initialize(quint16 port) ;
    void cleanup() ;
    void broadcast(const QByteArray &msg,const int &messageType = 0) ;

private slots:
    void onNewConnection() ;
    void onMessageReceived(const QByteArray &msg) ;
    void onDisconnected();

signals:
    void newConnection(QWebSocket *socket);
    void messageReceived(QWebSocket *socket, const QByteArray &message);
    void connectionClosed(QWebSocket *socket);
private:
    QWebSocketServer *m_server = nullptr;
    QList<QWebSocket*> m_clients;
};
#endif // WEBSOCKETSERVER_H