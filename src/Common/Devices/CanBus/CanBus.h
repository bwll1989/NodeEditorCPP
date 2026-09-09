#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QVariantMap>
#include <QList>

#include "CanBusWorker.h"

struct CanBusDeviceInfo
{
    int index = -1;
    QString displayName;
    QString path;
};

class CanBus : public QObject
{
    Q_OBJECT

public:
    explicit CanBus(QObject *parent = nullptr);
    ~CanBus() override;

    /** Enumerate candleLight / gs_usb WinUSB devices with readable display names. */
    static QList<CanBusDeviceInfo> availableDevices();

public slots:
    void openDevice(int deviceIndex, int channel, int bitrate, int modeFlags);
    void closeDevice();
    void sendFrame(quint32 canId, const QByteArray &data, bool extended = false, bool rtr = false);

signals:
    void isReady(bool ready);
    void recMsg(const QVariantMap &msg);
    void errorOccurred(const QString &message);

    void openDeviceRequest(int deviceIndex, int channel, int bitrate, int modeFlags);
    void closeDeviceRequest();
    void sendFrameRequest(quint32 canId, const QByteArray &data, bool extended, bool rtr);

private:
    CanBusWorker *worker = nullptr;
    QThread *workerThread = nullptr;
};
