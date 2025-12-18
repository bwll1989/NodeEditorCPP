#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <functional>
#include <memory>
#include <utility>
#include <type_traits>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <QVariant>
#include <QVariantMap>
#include <QMetaObject>
#include <QMetaType>
#include <QPointer>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QElapsedTimer>
#include <QCoreApplication>

#ifdef MQTTCLIENT_LIBRARY
#define MQTTCLIENT_EXPORT Q_DECL_EXPORT
#else
#define MQTTCLIENT_EXPORT Q_DECL_IMPORT
#endif

class MqttClientWorker;
class MQTTCLIENT_EXPORT MqttClient : public QObject
{
    Q_OBJECT

public:
    explicit MqttClient(QObject *parent = nullptr);
    ~MqttClient();

    /**
     * @brief 连接到MQTT服务器
     * @param host 服务器地址
     * @param port 端口号
     * @param username 用户名（可选）
     * @param password 密码（可选）
     */
    void connectToHost(const QString &host, int port, const QString &username = QString(), const QString &password = QString());
    
    /**
     * @brief 断开与服务器的连接
     */
    void disconnectFromHost();
    
    /**
     * @brief 发布消息
     * @param topic 主题
     * @param message 消息内容
     * @param qos 服务质量 (0, 1, 2)
     * @param retain 是否保留消息
     */
    void publish(const QString &topic, const QString &message, int qos = 0, bool retain = false);
    
    /**
     * @brief 订阅主题
     * @param topic 主题
     * @param qos 服务质量
     */
    void subscribe(const QString &topic, int qos = 0);
    
    /**
     * @brief 取消订阅
     * @param topic 主题
     */
    void unsubscribe(const QString &topic);

signals:
    void isConnectedChanged(bool connected);
    void messageReceived(const QString &topic, const QByteArray &payload);
    void errorOccurred(const QString &error);

    // Internal signals to worker
    void connectToHostRequest(const QString &host, int port, const QString &username, const QString &password);
    void disconnectFromHostRequest();
    void publishRequest(const QString &topic, const QString &message, int qos, bool retain);
    void subscribeRequest(const QString &topic, int qos);
    void unsubscribeRequest(const QString &topic);
    void stopTimerRequest();

private:
    QThread *m_thread = nullptr;
    MqttClientWorker *m_worker = nullptr;
};
