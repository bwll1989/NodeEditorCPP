#include "SerialPort.h"

SerialPort::SerialPort(QObject *parent) : QObject(parent)
{
    workerThread = new QThread();
    worker = new SerialPortWorker();
    worker->moveToThread(workerThread);

    connect(this, &SerialPort::openPortRequest, worker, &SerialPortWorker::openPort, Qt::QueuedConnection);
    connect(this, &SerialPort::closePortRequest, worker, &SerialPortWorker::closePort, Qt::QueuedConnection);
    connect(this, &SerialPort::sendMessageRequest, worker, &SerialPortWorker::sendMessage, Qt::QueuedConnection);

    connect(worker, &SerialPortWorker::isReady, this, &SerialPort::isReady, Qt::QueuedConnection);
    connect(worker, &SerialPortWorker::recMsg, this, &SerialPort::recMsg, Qt::QueuedConnection);

    workerThread->start();
    QMetaObject::invokeMethod(worker, "initialize", Qt::QueuedConnection);
}

SerialPort::~SerialPort()
{
    emit closePortRequest();
    worker->deleteLater();

    if (workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }

    delete workerThread;
}

void SerialPort::openPort(const QString &portName,
                          int baudRate,
                          QSerialPort::DataBits dataBits,
                          QSerialPort::Parity parity,
                          QSerialPort::StopBits stopBits)
{
    emit openPortRequest(portName, baudRate, dataBits, parity, stopBits);
}

void SerialPort::closePort()
{
    emit closePortRequest();
}

void SerialPort::sendMessage(const QString &message, int format)
{
    emit sendMessageRequest(message, format);
}
