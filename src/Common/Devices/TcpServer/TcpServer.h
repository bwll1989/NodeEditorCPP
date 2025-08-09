//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_TCPSERVER_H
#define NODEEDITORCPP_TCPSERVER_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QString dstHost="127.0.0.1",int port=2001, QObject *parent = nullptr);

    ~TcpServer();

public slots:
    void initializeServer();
    void cleanup();
    void sendMessage(const QString &message,const int &format=0);
    void sendByteArray(const QByteArray &byteArray);
    void sendBytesArrayToHost(const QByteArray &byteArray,const QString &host);
    void setHost(QString address,int port);

signals:
    // 对外信号
    void isReady(const bool &isready);
    void recMsg(const QVariantMap &res);
    void arrayMsg(QByteArray datagram);
    
    // 内部信号，用于线程间通信
    void initializeRequested(const QString &host, int port);
    void cleanupRequested();
    void sendMessageRequested(const QString &message,const int &format);
    void sendByteArrayRequested(const QByteArray &byteArray);
    void sendByteArrayToHostRequested(const QByteArray &byteArray,const QString &host);
private:
    quint16 mPort;
    QString mHost;
    QThread *mThread;
    QTcpServer *mServer;
    QList<QTcpSocket *> mClientSockets; // 存储所有客户端连接
};


#endif //NODEEDITORCPP_TCPSERVER_H
