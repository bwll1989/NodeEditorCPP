#ifndef NODEEDITORCPP_SERIALPORTWORKER_H
#define NODEEDITORCPP_SERIALPORTWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QVariantMap>

class SerialPortWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortWorker(QObject *parent = nullptr);
    ~SerialPortWorker();

public slots:
    void initialize();
    void openPort(const QString &portName,
                  int baudRate,
                  QSerialPort::DataBits dataBits,
                  QSerialPort::Parity parity,
                  QSerialPort::StopBits stopBits);
    void closePort();
    void sendMessage(const QString &message, int format = 0);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

signals:
    void isReady(bool ready);
    void recMsg(const QVariantMap &msg);

private:
    QSerialPort *serialPort = nullptr;
    QString portName;
    int baudRate = 9600;
};

#endif //NODEEDITORCPP_SERIALPORTWORKER_H
