#include "TcpClientWorker.h"

// 修改构造函数
TcpClientWorker::TcpClientWorker(QObject *parent) : QObject(parent)
{
    // 不在构造函数中创建套接字，而是在线程启动后创建
    tcpClient = nullptr;
    
    // 创建计时器
    m_timer = new QTimer(this);
    m_timer->setInterval(5000); // 每隔5秒重连一次
    
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
    host = dstHost;
    port = dstPort;
    
    // 关闭现有连接
    tcpClient->close();
    
    // 尝试连接到服务器
    tcpClient->connectToHost(host, port);
    
    if(tcpClient->waitForConnected(500))
    {
        if(m_timer->isActive()){
            m_timer->stop();
        }
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

void TcpClientWorker::sendMessage(const QString &message)
{
    if(tcpClient->state() == QAbstractSocket::ConnectedState)
    {
        tcpClient->write(message.toUtf8());
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
    dataMap.insert("hex", data.toHex());
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