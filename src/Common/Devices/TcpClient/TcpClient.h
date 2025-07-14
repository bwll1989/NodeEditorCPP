//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_TCPCLIENT_H
#define NODEEDITORCPP_TCPCLIENT_H

#include <QObject>
#include <QThread>
#include <QVariantMap>
#include "TcpClientWorker.h"

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QString dstHost="127.0.0.1", int dstPort=2001, QObject *parent = nullptr);
    ~TcpClient();

public slots:
    // 连接到服务器
    void connectToServer(const QString &dstHost, int dstPort);
    
    // 断开连接
    void disconnectFromServer();
    
    // 发送消息
    void sendMessage(const QString &message,const int &format);
    
    // 停止计时器
    void stopTimer();

signals:
    // 连接状态信号
    void isReady(const bool &isready);
    
    // 接收消息信号
    void recMsg(const QVariantMap &Msg);
    
    // 发送给Worker的信号
    void connectToServerRequest(const QString &dstHost, int dstPort);
    void disconnectFromServerRequest();
    void sendMessageRequest(const QString &message,const int &format);
    void stopTimerRequest();

private:
    TcpClientWorker *worker;
    QThread *workerThread;
    QString host;
    int port;
};

#endif //NODEEDITORCPP_TCPCLIENT_H
