#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantMap>

class CanBusWorker : public QObject
{
    Q_OBJECT

public:
    explicit CanBusWorker(QObject *parent = nullptr);
    ~CanBusWorker() override;

public slots:
    void initialize();
    void openDevice(int deviceIndex, int channel, int bitrate, int modeFlags);
    void closeDevice();
    void sendFrame(quint32 canId, const QByteArray &data, bool extended, bool rtr);

signals:
    void isReady(bool ready);
    void recMsg(const QVariantMap &msg);
    void errorOccurred(const QString &message);

private slots:
    void pollFrames();

private:
    void releaseDevice();

    void *m_device = nullptr; // candle_handle
    int m_channel = 0;
    QTimer *m_pollTimer = nullptr;
};
