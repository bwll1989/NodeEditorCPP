//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_OSCRECEIVER_H
#define NODEEDITORCPP_OSCRECEIVER_H
#include <QObject>

#include "Common/Devices/UdpSocket/UdpSocket.h"
class OSCReceiver:public QObject {
        Q_OBJECT

        public:
        explicit OSCReceiver(quint16 port = 12345, QObject *parent = nullptr);
        ~OSCReceiver();
        void setPort(const int &port = 12345);
    signals:
        void receiveOSC(QVariantMap &data);
        void receiveData(QString data);
        // void portChanged(const int &port);

    private slots:
        void processPendingDatagrams(QByteArray datagram);  // 处理接收到的UDP数据


    private:
        UdpSocket *m_udpSocket;  // 用于接收 UDP 数据的套接字
        QVariantMap *result;
    };


#endif //NODEEDITORCPP_EXTERNALCONTROLER_H
