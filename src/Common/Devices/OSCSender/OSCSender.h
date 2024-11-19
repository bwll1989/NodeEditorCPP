//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_OSCSENDER_H
#define NODEEDITORCPP_OSCSENDER_H
#include <QObject>
#include "QThread"
#include <QUdpSocket>
class OSCSender:public QObject {
        Q_OBJECT

    public:
        explicit OSCSender(QString dstHost="127.0.0.1",quint16 port = 6001, QObject *parent = nullptr);
        ~OSCSender();

    public slots:
        bool sendMessage(QVariantMap &data);
        void setHost(QString address,int port);
        void initializeSocket();
        void cleanup();
    private:

//        QVariantMap *result;
        quint16 mPort;
        QString mHost;
        QThread *mThread;
        QUdpSocket *mSocket;
    };


#endif //NODEEDITORCPP_OSCSENDER_H
