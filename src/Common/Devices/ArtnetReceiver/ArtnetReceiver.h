//
// Created by bwll1 on 2024/9/1.
//

#pragma once

#include <QObject>
#include "QThread"
#include <QtNetwork/QUdpSocket>
#include <QVariant>
class ArtnetReceiver:public QObject {
    Q_OBJECT
public:
    explicit ArtnetReceiver(QObject *parent = nullptr);
    ~ArtnetReceiver();

signals:
    void receiveArtnet(QVariantMap &data);

public slots:
    void processPendingDatagrams();  // 处理接收到的UDP数据
    void universeFilter(const int &uni);
    void initializeSocket();
    void cleanup();
private:
    int mUniverse;
    QString mHost;
    QThread *mThread;
    QUdpSocket *mSocket;
    QVariantMap result;
};


