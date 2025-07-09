//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_TCPCLIENT_H
#define NODEEDITORCPP_TCPCLIENT_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include "QTimer"
#include "QThread"
class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QString dstHost="127.0.0.1",int dstPort=2001,QObject *parent = nullptr);

    ~TcpClient();

public slots:
    void connectToServer(const QString &dstHost,int dstPort);

    void sendMessage(const QString &message);

    void onReadyRead();

    void onConnected();

    void onDisconnected();

    void onErrorOccurred(QAbstractSocket::SocketError socketError);

    void reConnect();

signals:

    void isReady(const bool &isready);

    void recMsg(const QVariantMap &Msg);

private:
    QTcpSocket *tcpClient;
    QTimer *m_timer=new QTimer(this);
    QString host;
    int port;
    bool isConnected= false;
    QThread *mThread;
};


#endif //NODEEDITORCPP_TCPCLIENT_H
