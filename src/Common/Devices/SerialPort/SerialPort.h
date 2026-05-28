//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_SERIALPORT_H
#define NODEEDITORCPP_SERIALPORT_H

#include <QObject>
#include <QThread>
#include <QVariantMap>
#include "SerialPortWorker.h"

class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

public slots:
    void openPort(const QString &portName,
                  int baudRate,
                  QSerialPort::DataBits dataBits = QSerialPort::Data8,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    void closePort();
    void sendMessage(const QString &message, int format = 0);

signals:
    void isReady(bool ready);
    void recMsg(const QVariantMap &msg);

    void openPortRequest(const QString &portName,
                         int baudRate,
                         QSerialPort::DataBits dataBits,
                         QSerialPort::Parity parity,
                         QSerialPort::StopBits stopBits);
    void closePortRequest();
    void sendMessageRequest(const QString &message, int format);

private:
    SerialPortWorker *worker;
    QThread *workerThread;
};

#endif //NODEEDITORCPP_SERIALPORT_H
