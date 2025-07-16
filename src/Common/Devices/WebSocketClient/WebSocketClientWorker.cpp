#include "WebSocketClientWorker.h"
#include <QByteArray>
#include <QHostInfo>
#include <QThread>
// 修改构造函数
WebSocketClientWorker::WebSocketClientWorker(QObject *parent ) : QObject(parent)
{

    // 不在构造函数中创建套接字，而是在线程启动后创建
    m_socket = nullptr;

    // 创建计时器
    m_timer = new QTimer(this);
    m_timer->setInterval(5000); // 每隔5秒重连一次

    // 连接计时器信号
    connect(m_timer, &QTimer::timeout, this, &WebSocketClientWorker::reConnect);
}

// 添加初始化方法，在工作线程中调用
void WebSocketClientWorker::initialize()
{
    m_socket = new QWebSocket();
    connect(m_socket, &QWebSocket::connected, this, &WebSocketClientWorker::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocketClientWorker::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClientWorker::onTextMessageReceived);
    connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocketClientWorker::onBinaryMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &WebSocketClientWorker::onErrorOccurred);
}

WebSocketClientWorker::~WebSocketClientWorker()
{
    // 确保在析构前停止计时器和断开连接
    if (m_timer) {
        m_timer->stop();
    }
    disconnectFromServer();
}

void WebSocketClientWorker::connectToServer(const QUrl &url)
{

    // 1. 停止重连定时器
    if (m_timer->isActive())
        m_timer->stop();

    // 2. 断开当前连接
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

    m_url = url;
    // 4. 添加空指针保护
    if (!m_socket) {
        qCritical() << "Socket not initialized! Call initialize() first";
        return;
    }

    // 5. 检查socket状态
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        qWarning() << "Socket is in invalid state:" << m_socket->state();
        return;
    }
    // 确保在当前线程操作socket（添加线程安全检查）
    if (m_socket->thread() != this->thread()) {
        qCritical() << "Socket accessed from wrong thread!";
        return;
    }

    m_socket->open(m_url); // 用 open
    // QWebSocket 是异步连接，连接结果由 onConnected/onDisconnected/onErrorOccurred 槽处理
    // 不再调用 waitForConnected，也不在这里 emit isReady
}

void WebSocketClientWorker::disconnectFromServer()
{
    // 停止重连计时器
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }

    // 如果连接处于活动状态，主动断开
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        // 关闭连接
        m_socket->abort(); // 使用abort()立即关闭，而不是优雅关闭

        // 更新状态
        isConnected = false;
        emit isReady(isConnected);
    }
}

void WebSocketClientWorker::sendMessage(const QString &message,const int &messageType, const int &format)
{
    if(m_socket->state() == QAbstractSocket::ConnectedState)
    {
        switch (messageType) {
        case 0: // 文本
            switch (format)
            {
            case 0: // HEX
                m_socket->sendTextMessage(QByteArray::fromHex(message.toUtf8()));
                break;
            case 1: // 文本
                m_socket->sendTextMessage(message.toUtf8());
                break;
            case 2: // ASCII
                m_socket->sendTextMessage(message.toLatin1());
                break;
            default:
                m_socket->sendTextMessage(message.toUtf8());
                break;
            }
            break;
        case 1: // 二进制
            switch (format)
            {
            case 0: // HEX
                m_socket->sendBinaryMessage(QByteArray::fromHex(message.toUtf8()));
                break;
            case 1: // 文本
                m_socket->sendBinaryMessage(message.toUtf8());
                break;
            case 2: // ASCII
                m_socket->sendBinaryMessage(message.toLatin1());
                break;
            default:
                m_socket->sendBinaryMessage(message.toUtf8());
                break;
            }
        }

    }
}

void WebSocketClientWorker::stopTimer()
{
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
}

void WebSocketClientWorker::onTextMessageReceived(const QString &message)
{
    QVariantMap dataMap;
    dataMap.insert("type", "BinaryMessage");
    dataMap.insert("url", m_socket->peerAddress().toString());
    dataMap.insert("hex", message.toUtf8().toHex());
    dataMap.insert("utf-8", message);
    dataMap.insert("default", message);
    emit recMsg(dataMap);
}

void WebSocketClientWorker::onBinaryMessageReceived(const QByteArray &message)
{
    QVariantMap dataMap;
    dataMap.insert("type", "TextMessage");
    dataMap.insert("url", m_socket->peerAddress().toString());
    dataMap.insert("hex", QString(message.toHex()));
    dataMap.insert("utf-8", QString::fromUtf8(message));
    dataMap.insert("ascii", QString::fromLatin1(message));
    dataMap.insert("default", QString::fromUtf8(message));
    emit recMsg(dataMap);
}

void WebSocketClientWorker::onConnected()
{
    isConnected = true;
    m_timer->stop();
    emit isReady(isConnected);
}

void WebSocketClientWorker::onDisconnected()
{
    isConnected = false;
    emit isReady(isConnected);
    m_timer->start();
}

void WebSocketClientWorker::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qWarning() << "Socket error:" << socketError;
    isConnected = false;
    emit isReady(isConnected);
}

void WebSocketClientWorker::reConnect()
{

    connectToServer(m_url);
}