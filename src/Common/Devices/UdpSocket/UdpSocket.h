//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_UDPSOCKET_H
#define NODEEDITORCPP_UDPSOCKET_H
#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QThread>
class UdpSocket : public QObject
{
    Q_OBJECT

public:
    explicit UdpSocket(QString dstHost="127.0.0.1", int dstPort=2001, QObject *parent = nullptr);
    ~UdpSocket();

public slots:
    void processPendingDatagrams();
    void initializeSocket();
    void setHost(QString address,int port);
    void cleanup();
    void sendMessage(const QString &host,const int &port,const QString &message,const int &format=0);


signals:

    void isReady(const bool &isready);

    void recMsg(QVariantMap &Msg);

    void arrayMsg(QByteArray datagram);

private:
    quint16 mPort;
    QString mHost;
    QThread *mThread;
    QUdpSocket *mSocket;

};


#endif //NODEEDITORCPP_UDPSOCKET_H
