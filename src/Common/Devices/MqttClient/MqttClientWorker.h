#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include "qmqtt.h"

class MqttClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit MqttClientWorker(QObject *parent = nullptr);
    ~MqttClientWorker();

public slots:
    /**
     * @brief 初始化Worker，创建QMQTT客户端实例
     */
    void initialize();
    
    /**
     * @brief 连接到主机
     */
    void connectToHost(const QString &host, int port, const QString &username, const QString &password);
    
    /**
     * @brief 断开连接
     */
    void disconnectFromHost();
    
    /**
     * @brief 发布消息
     */
    void publish(const QString &topic, const QString &message, int qos = 0, bool retain = false);
    
    /**
     * @brief 订阅主题
     */
    void subscribe(const QString &topic, int qos = 0);
    
    /**
     * @brief 取消订阅
     */
    void unsubscribe(const QString &topic);
    
    /**
     * @brief 停止重连定时器
     */
    void stopTimer();

private slots:
    void onConnected();
    void onDisconnected();
    void onReceived(const QMQTT::Message &message);
    void onError(QMQTT::ClientError error);
    void reConnect();

signals:
    void isConnectedChanged(bool connected);
    void messageReceived(const QString &topic, const QByteArray &payload);
    void errorOccurred(const QString &error);

private:
    QMQTT::Client *m_client = nullptr;
    QTimer *m_timer = nullptr;
    QString m_host;
    int m_port = 1883;
    QString m_username;
    QString m_password;
    bool m_isConnected = false;
};
