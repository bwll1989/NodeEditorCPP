#include "TcpClientWorker.h"
#include <QByteArray>
#include <QHostInfo>
#include <QThread>
// 修改构造函数
TcpClientWorker::TcpClientWorker(QObject *parent) : QObject(parent)
{
    // 不在构造函数中创建套接字，而是在线程启动后创建
    tcpClient = nullptr;
    
    // 创建计时器
    m_timer = new QTimer(this);
    m_timer->setInterval(1000); // 每隔1秒重连一次
    
    // 连接计时器信号
    connect(m_timer, &QTimer::timeout, this, &TcpClientWorker::reConnect);
}

// 添加初始化方法，在工作线程中调用
void TcpClientWorker::initialize()
{
    // 在工作线程中创建套接字
    tcpClient = new QTcpSocket(this);
    
    // 连接信号和槽
    connect(tcpClient, &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
    connect(tcpClient, &QTcpSocket::connected, this, &TcpClientWorker::onConnected);
    connect(tcpClient, &QTcpSocket::disconnected, this, &TcpClientWorker::onDisconnected);
    connect(tcpClient, qOverload<QAbstractSocket::SocketError>(&QTcpSocket::errorOccurred), 
            this, &TcpClientWorker::onErrorOccurred);
}

TcpClientWorker::~TcpClientWorker()
{
    // 确保在析构前停止计时器和断开连接
    if (m_timer) {
        m_timer->stop();
    }
    disconnectFromServer();
}

void TcpClientWorker::connectToServer(const QString &dstHost, int dstPort)
{

    // 1. 停止重连定时器
    if (m_timer->isActive())
        m_timer->stop();

    // 2. 断开当前连接
    if (tcpClient->state() != QAbstractSocket::UnconnectedState) {
        tcpClient->abort();
    }

    // 3. 更新 host/port
    host = dstHost;
    port = dstPort;
    // 4. 添加空指针保护
    if (!tcpClient) {
        qCritical() << "Socket not initialized! Call initialize() first";
        return;
    }

    // 5. 检查socket状态
    if (tcpClient->state() != QAbstractSocket::UnconnectedState) {
        qWarning() << "Socket is in invalid state:" << tcpClient->state();
        return;
    }
    // 确保在当前线程操作socket（添加线程安全检查）
    if (tcpClient->thread() != this->thread()) {
        qCritical() << "Socket accessed from wrong thread!";
        return;
    }
    // qDebug() << "connecting to server at" <<host << ":" << port;
    tcpClient->connectToHost(host, port);

    if(tcpClient->waitForConnected(500))
    {
        isConnected = true;
    }
    else{
        m_timer->start();
        isConnected = false;
    }
    emit isReady(isConnected);
}

void TcpClientWorker::disconnectFromServer()
{
    // 停止重连计时器
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
    
    // 如果连接处于活动状态，主动断开
    if (tcpClient->state() != QAbstractSocket::UnconnectedState) {
        // 关闭连接
        tcpClient->abort(); // 使用abort()立即关闭，而不是优雅关闭
        
        // 更新状态
        isConnected = false;
        emit isReady(isConnected);
    }
}

void TcpClientWorker::sendMessage(const QString &message,const int &format)
{
    QByteArray data;
    switch (format) {
    case 0: // HEX格式
        data = QByteArray::fromHex(message.toUtf8());
        break;
    case 1: // UTF-8格式
        data = message.toUtf8();  // 修复冒号错误为括号
        break;
    case 2: // ASCII
        data = message.toLatin1(); // 使用本地编码（ASCII）
        break;
    default:
        data = message.toUtf8();  // 默认使用UTF-8
        break;
    }
    if(tcpClient->state() == QAbstractSocket::ConnectedState)
    {
        tcpClient->write(data);
    }
}

void TcpClientWorker::stopTimer()
{
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
}

void TcpClientWorker::onReadyRead()
{
    QByteArray data = tcpClient->readAll();
    if(data.isEmpty())
    {
        return;
    }
    QVariantMap dataMap;
    dataMap.insert("host", tcpClient->peerAddress().toString());
    dataMap.insert("hex", QString(data.toHex())); // 转换为QString类型
    dataMap.insert("utf-8", QString::fromUtf8(data)); // 修复UTF-8解码方式
    dataMap.insert("ascii", QString::fromLatin1(data)); // 修复ANSI解码方式
    dataMap.insert("default", data);
    emit recMsg(dataMap);
}

void TcpClientWorker::onConnected()
{
    isConnected = true;
    m_timer->stop();
    emit isReady(isConnected);
}

void TcpClientWorker::onDisconnected()
{
    isConnected = false;
    emit isReady(isConnected);
    m_timer->start();
}

void TcpClientWorker::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    qWarning() << "Socket error:" << socketError;
    isConnected = false;
    emit isReady(isConnected);
}

void TcpClientWorker::reConnect()
{
    qDebug() << "reconnect" << host << port;
    connectToServer(host, port);
}