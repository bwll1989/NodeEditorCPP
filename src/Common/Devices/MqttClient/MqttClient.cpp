#include "MqttClient.h"
#include "MqttClientWorker.h"

MqttClient::MqttClient(QObject *parent) : QObject(parent)
{
    m_thread = new QThread();
    m_worker = new MqttClientWorker();
    m_worker->moveToThread(m_thread);

    // Signals from Client to Worker
    connect(this, &MqttClient::connectToHostRequest, m_worker, &MqttClientWorker::connectToHost, Qt::QueuedConnection);
    connect(this, &MqttClient::disconnectFromHostRequest, m_worker, &MqttClientWorker::disconnectFromHost, Qt::QueuedConnection);
    connect(this, &MqttClient::publishRequest, m_worker, &MqttClientWorker::publish, Qt::QueuedConnection);
    connect(this, &MqttClient::subscribeRequest, m_worker, &MqttClientWorker::subscribe, Qt::QueuedConnection);
    connect(this, &MqttClient::unsubscribeRequest, m_worker, &MqttClientWorker::unsubscribe, Qt::QueuedConnection);
    connect(this, &MqttClient::stopTimerRequest, m_worker, &MqttClientWorker::stopTimer, Qt::QueuedConnection);

    // Signals from Worker to Client
    connect(m_worker, &MqttClientWorker::isConnectedChanged, this, &MqttClient::isConnectedChanged, Qt::QueuedConnection);
    connect(m_worker, &MqttClientWorker::messageReceived, this, &MqttClient::messageReceived, Qt::QueuedConnection);
    connect(m_worker, &MqttClientWorker::errorOccurred, this, &MqttClient::errorOccurred, Qt::QueuedConnection);

    m_thread->start();

    // Initialize worker
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection);
}

MqttClient::~MqttClient()
{
    // Stop timer and disconnect
    emit stopTimerRequest();
    emit disconnectFromHostRequest();

    // Clean up thread
    // Request worker deletion via event loop to ensure thread safety
    m_worker->deleteLater();
    
    if (m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
    
    delete m_thread;
}

void MqttClient::connectToHost(const QString &host, int port, const QString &username, const QString &password)
{
    emit connectToHostRequest(host, port, username, password);
}

void MqttClient::disconnectFromHost()
{
    emit disconnectFromHostRequest();
}

void MqttClient::publish(const QString &topic, const QString &message, int qos, bool retain)
{
    emit publishRequest(topic, message, qos, retain);
}

void MqttClient::subscribe(const QString &topic, int qos)
{
    emit subscribeRequest(topic, qos);
}

void MqttClient::unsubscribe(const QString &topic)
{
    emit unsubscribeRequest(topic);
}
