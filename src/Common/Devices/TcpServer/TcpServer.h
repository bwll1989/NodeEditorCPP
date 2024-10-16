//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_TCPSERVER_H
#define NODEEDITORCPP_TCPSERVER_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(int port=2000, QObject *parent = nullptr);
    int listenPort;
    QList<QTcpSocket *> m_clients; // 存储所有客户端连接
    ~TcpServer();
public slots:
    void stopServer();

    void startServer();

    void sendMessage(const QString &client,const QString &message);

    void setPort(const int &port);

signals:
        void serverError(const QString &error);
        void serverMessage(const QString &message);
        void serverStoped();
        void clientInserted(const QString &message);
        void clientRemoved(const QString &message);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};


#endif //NODEEDITORCPP_TCPSERVER_H
