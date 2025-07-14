//
// Created by bwll1 on 2024/10/10.
//

#include "TcpClient.h"

TcpClient::TcpClient(QString dstHost, int dstPort, QObject *parent) : QObject(parent)
{
    host = std::move(dstHost);
    port = dstPort;
    
    // 创建工作线程
    workerThread = new QThread();
    
    // 创建工作对象
    worker = new TcpClientWorker();
    
    // 将工作对象移动到工作线程
    worker->moveToThread(workerThread);
    
    // 连接TcpClient的信号到TcpClientWorker的槽
    connect(this, &TcpClient::connectToServerRequest, worker, &TcpClientWorker::connectToServer, Qt::QueuedConnection);
    connect(this, &TcpClient::disconnectFromServerRequest, worker, &TcpClientWorker::disconnectFromServer, Qt::QueuedConnection);
    connect(this, &TcpClient::sendMessageRequest, worker, &TcpClientWorker::sendMessage, Qt::QueuedConnection);
    connect(this, &TcpClient::stopTimerRequest, worker, &TcpClientWorker::stopTimer, Qt::QueuedConnection);
    
    // 连接TcpClientWorker的信号到TcpClient的信号
    connect(worker, &TcpClientWorker::isReady, this, &TcpClient::isReady, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::recMsg, this, &TcpClient::recMsg, Qt::QueuedConnection);
    
    // 启动工作线程
    workerThread->start();
    
    // 在工作线程中初始化套接字
    QMetaObject::invokeMethod(worker, "initialize", Qt::QueuedConnection);
    
    // 初始连接
    QMetaObject::invokeMethod(worker, "connectToServer", Qt::QueuedConnection,
                              Q_ARG(QString, host), Q_ARG(int, port));
}

TcpClient::~TcpClient()
{
    // 请求断开连接
    emit disconnectFromServerRequest();
    
    // 等待一小段时间确保断开连接的消息被处理
    QThread::msleep(50);
    
    // 停止并删除线程和工作对象
    if (workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }
    
    worker->deleteLater();
    delete workerThread;
}

void TcpClient::connectToServer(const QString &dstHost, int dstPort)
{
    host = dstHost;
    port = dstPort;
    emit connectToServerRequest(dstHost, dstPort);
}

void TcpClient::disconnectFromServer()
{
    emit disconnectFromServerRequest();
}

void TcpClient::sendMessage(const QString &message,const int &format)
{
    emit sendMessageRequest(message,format);
}

void TcpClient::stopTimer()
{
    emit stopTimerRequest();
}