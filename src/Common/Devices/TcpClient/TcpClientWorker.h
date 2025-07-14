#ifndef NODEEDITORCPP_TCPCLIENTWORKER_H
#define NODEEDITORCPP_TCPCLIENTWORKER_H

#include <QtNetwork/QTcpSocket>
#include <QTimer>
#include <QObject>
#include <QVariantMap>

class TcpClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit TcpClientWorker(QObject *parent = nullptr);
    ~TcpClientWorker();

public slots:
    // 在TcpClientWorker.h中添加
    void initialize();
    
    // 连接到服务器
    void connectToServer(const QString &dstHost, int dstPort);
    
    // 断开连接
    void disconnectFromServer();
    
    // 发送消息
    void sendMessage(const QString &message);
    
    // 停止重连计时器
    void stopTimer();

private slots:
    // 处理套接字事件的槽函数
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void reConnect();

signals:
    // 连接状态信号
    void isReady(const bool &isready);
    
    // 接收消息信号
    void recMsg(const QVariantMap &Msg);

private:
    QTcpSocket *tcpClient;
    QTimer *m_timer;
    QString host;
    int port;
    bool isConnected = false;
};

#endif //NODEEDITORCPP_TCPCLIENTWORKER_H