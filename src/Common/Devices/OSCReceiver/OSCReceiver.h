//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_OSCRECEIVER_H
#define NODEEDITORCPP_OSCRECEIVER_H
#include <QObject>
#include "QThread"
#include <QtNetwork/QUdpSocket>
#include "OSCMessage.h"
#include "tinyosc.h"
class OSCReceiver:public QObject {
    Q_OBJECT
public:
    explicit OSCReceiver(quint16 port = 6000, QObject *parent = nullptr);
    ~OSCReceiver();

signals:
    void receiveOSCMessage(const OSCMessage& message);
    void receiveOSC(const QVariantMap& data);

public slots:
    void processPendingDatagrams();  // 处理接收到的UDP数据

    void setPort(const int &port);
    void initializeSocket();
    void cleanup();
private:
    quint16 mPort;
    QString mHost;
    QThread *mThread;
    QUdpSocket *mSocket;
    QVariantMap result;
    OSCMessage message;
};


#endif //NODEEDITORCPP_EXTERNALCONTROLER_H
