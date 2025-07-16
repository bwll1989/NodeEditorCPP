//
// Created by bwll1 on 2024/10/10.
//

#include "WebSocketClient.h"

WebSocketClient::WebSocketClient( QObject *parent,QUrl dstHost) : QObject(parent)
{
    if (dstHost.isEmpty())
        host = QUrl("ws://127.0.0.1:2003");
    else
        host = std::move(dstHost);
    // 创建工作线程
    workerThread = new QThread();

    // 创建工作对象
    worker = new WebSocketClientWorker();

    // 将工作对象移动到工作线程
    worker->moveToThread(workerThread);

    // 连接TcpClient的信号到TcpClientWorker的槽
    connect(this, &WebSocketClient::connectToServerRequest, worker, &WebSocketClientWorker::connectToServer, Qt::QueuedConnection);
    connect(this, &WebSocketClient::disconnectFromServerRequest, worker, &WebSocketClientWorker::disconnectFromServer, Qt::QueuedConnection);
    connect(this, &WebSocketClient::sendMessageRequest, worker, &WebSocketClientWorker::sendMessage, Qt::QueuedConnection);
    connect(this, &WebSocketClient::stopTimerRequest, worker, &WebSocketClientWorker::stopTimer, Qt::QueuedConnection);

    // 连接TcpClientWorker的信号到TcpClient的信号
    connect(worker, &WebSocketClientWorker::isReady, this, &WebSocketClient::isReady, Qt::QueuedConnection);
    connect(worker, &WebSocketClientWorker::recMsg, this, &WebSocketClient::recMsg, Qt::QueuedConnection);
    // 启动工作线程
    workerThread->start();

    // 在工作线程中初始化套接字
    QMetaObject::invokeMethod(worker, "initialize", Qt::QueuedConnection);

    // 初始连接
    QMetaObject::invokeMethod(worker, "connectToServer", Qt::QueuedConnection,
                              Q_ARG(QUrl, host));

    // 连接 worker 的 isReady 信号，转发为 connected/disconnected
    connect(worker, &WebSocketClientWorker::isReady, this, [this](bool ready){
        emit isReady(ready);
    }, Qt::QueuedConnection);
    // 你可以根据需要在 worker 的 onErrorOccurred 里发 errorOccurred 信号
}

WebSocketClient::~WebSocketClient()
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

void WebSocketClient::connectToServer(const QUrl &dstHost)
{
    host = dstHost;
    emit connectToServerRequest(dstHost);
}

void WebSocketClient::disconnectFromServer()
{
    emit disconnectFromServerRequest();
}

void WebSocketClient::sendMessage(const QString &message,const int &messageType,const int &format)
{
    emit sendMessageRequest(message,messageType,format);
}

void WebSocketClient::stopTimer()
{
    emit stopTimerRequest();
}