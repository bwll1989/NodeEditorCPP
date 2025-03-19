//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_OSCSENDER_H
#define NODEEDITORCPP_OSCSENDER_H
#include <QObject>
#include "QThread"
#include <QUdpSocket>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QVariant>
#include "OSCMessage.h"

class OSCSender:public QObject {
        Q_OBJECT

public:
    explicit OSCSender(QString dstHost="127.0.0.1",quint16 port = 6001, QObject *parent = nullptr);
    ~OSCSender();
    static OSCSender* instance() {
        static OSCSender* sender = nullptr;
        if (!sender) {
            sender = new OSCSender();
        }
        return sender;
    }
    public slots:
        void processQueue();
        bool sendOSCMessageWithQueue(OSCMessage &message);
        bool sendOSCMessageDirectly(OSCMessage &message);
        void setHost(QString address,int port);
        void initializeSocket();
        void cleanup();
    private:

//        QVariantMap *result;
        QTimer *m_timer;
        quint16 mPort;
        QString mHost;
        QThread *mThread;
        QUdpSocket *mSocket;
        QMutex m_mutex;
        QQueue<OSCMessage> m_messageQueue;
        // 处理消息的间隔时间
        static const int PROCESS_INTERVAL = 16;
    };


#endif //NODEEDITORCPP_OSCSENDER_H
