//
// Created by bwll1 on 2024/9/1.
//

#ifndef NODEEDITORCPP_ARTNETRECEIVER_H
#define NODEEDITORCPP_ARTNETRECEIVER_H
#include <QObject>
#include "QThread"
#include <QtNetwork/QUdpSocket>
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


#endif //NODEEDITORCPP_EXTERNALCONTROLER_H
