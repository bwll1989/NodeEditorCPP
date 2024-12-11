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
//    void processPendingDatagrams();
    void initializeServer();
//    void setHost(QString address,int port);
    void cleanup();
    void sendMessage(const QString &message);
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void setHost(QString address,int port);
signals:

    void isReady(const bool &isready);

    void recMsg(const QVariantMap &res);
//
    void arrayMsg(QByteArray datagram);

private:
    quint16 mPort;
    QString mHost;
    QThread *mThread;
    QTcpServer *mServer;
    QList<QTcpSocket *> mClientSockets; // 存储所有客户端连接
};


#endif //NODEEDITORCPP_TCPSERVER_H
