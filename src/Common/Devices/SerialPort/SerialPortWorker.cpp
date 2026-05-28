#include "SerialPortWorker.h"

SerialPortWorker::SerialPortWorker(QObject *parent) : QObject(parent)
{
}

SerialPortWorker::~SerialPortWorker()
{
    closePort();
}

void SerialPortWorker::initialize()
{
    serialPort = new QSerialPort(this);
    connect(serialPort, &QSerialPort::readyRead, this, &SerialPortWorker::onReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialPortWorker::onErrorOccurred);
}

void SerialPortWorker::openPort(const QString &portName,
                                int baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits)
{
    if (!serialPort) {
        return;
    }

    if (serialPort->isOpen()) {
        serialPort->close();
    }

    this->portName = portName;
    this->baudRate = baudRate;

    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(dataBits);
    serialPort->setParity(parity);
    serialPort->setStopBits(stopBits);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    const bool opened = serialPort->open(QIODevice::ReadWrite);
    emit isReady(opened);
}

void SerialPortWorker::closePort()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    emit isReady(false);
}

void SerialPortWorker::sendMessage(const QString &message, int format)
{
    if (!serialPort || !serialPort->isOpen()) {
        return;
    }

    QByteArray data;
    switch (format) {
    case 0:
        data = QByteArray::fromHex(message.toUtf8());
        break;
    case 1:
        data = message.toUtf8();
        break;
    case 2:
        data = message.toLatin1();
        break;
    default:
        data = message.toUtf8();
        break;
    }

    serialPort->write(data);
}

void SerialPortWorker::onReadyRead()
{
    const QByteArray data = serialPort->readAll();
    if (data.isEmpty()) {
        return;
    }

    QVariantMap dataMap;
    dataMap.insert("port", serialPort->portName());
    dataMap.insert("hex", QString(data.toHex()));
    dataMap.insert("utf-8", QString::fromUtf8(data));
    dataMap.insert("ascii", QString::fromLatin1(data));
    dataMap.insert("default", data);
    emit recMsg(dataMap);
}

void SerialPortWorker::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    emit isReady(serialPort && serialPort->isOpen());
}
