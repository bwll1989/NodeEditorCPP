//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_UDPSOCKET_H
#define NODEEDITORCPP_UDPSOCKET_H

#include <QtNetwork/QUdpSocket>

class UdpSocket : public QUdpSocket
{
    Q_OBJECT

public:
    explicit UdpSocket(QString dstHost="127.0.0.1", int dstPort=2001, QObject *parent = nullptr);

    ~UdpSocket();

public slots:
    void processPendingDatagrams();

    void startSocket(const QString &dstHost,int dstPort);

    void sendMessage(const QString &message);

signals:

    void isReady(const bool &isready);

    void recMsg(const QString &Msg);

private:
    QHostAddress  host;
    quint16 port;
};


#endif //NODEEDITORCPP_UDPSOCKET_H
