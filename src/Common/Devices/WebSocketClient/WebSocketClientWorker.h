#ifndef WEBSOCKETCLIENTWORKER_H
#define WEBSOCKETCLIENTWORKER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QUrl>

class WebSocketClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClientWorker(QObject *parent = nullptr);
    ~WebSocketClientWorker();

public slots:
    // 在TcpClientWorker.h中添加
    void initialize();

    // 连接到服务器
    void connectToServer(const QUrl &url);

    // 断开连接
    void disconnectFromServer();

    // 发送消息
    void sendMessage(const QString &message, const int &messageType = 0,const int &format = 0);

    // 停止重连计时器
    void stopTimer();

private slots:
    // 处理套接字事件的槽函数
    // void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
    void reConnect();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);

signals:
    // 连接状态信号
    void isReady(const bool &isReady);

    // 接收消息信号
    void recMsg(const QVariantMap &Msg);

private:
    QWebSocket *m_socket;
    QTimer *m_timer;
    QUrl m_url; // 修正为 QUrl
    bool isConnected = false;
};

#endif // WEBSOCKETCLIENTWORKER_H