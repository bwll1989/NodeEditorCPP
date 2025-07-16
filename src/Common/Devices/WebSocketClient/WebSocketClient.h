#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QUrl>
#include <QThread>
#include "WebSocketClientWorker.h"
class QThread;
class WebSocketClientWorker;

class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient( QObject *parent = nullptr,QUrl dstHost = QUrl());
    ~WebSocketClient();

public slots:
    // 连接到服务器
    void connectToServer(const QUrl &dstHost);

    // 断开连接
    void disconnectFromServer();

    // 发送消息
    void sendMessage(const QString &message,const int &messageType,const int &format);

    // 停止计时器
    void stopTimer();

signals:
    // 连接状态信号
    void isReady(const bool &isready);

    // 接收消息信号
    void recMsg(const QVariantMap &Msg);
    // 发送给Worker的信号
    void connectToServerRequest(const QUrl &dstHost);
    void disconnectFromServerRequest();
    void sendMessageRequest(const QString &message,const int &messageType,const int &format);
    void stopTimerRequest();

private:
    WebSocketClientWorker *worker;
    QThread *workerThread;
    QUrl host;


};

#endif // WEBSOCKETCLIENT_H