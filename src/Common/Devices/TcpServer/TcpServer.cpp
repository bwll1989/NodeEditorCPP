//
// Created by bwll1 on 2024/10/10.
//

#include "TcpServer.h"
#include "QThread"

// 创建一个工作类来处理所有网络操作
class TcpWorker : public QObject
{
    Q_OBJECT
public:
    explicit TcpWorker(QObject *parent = nullptr) : QObject(parent), mServer(nullptr) {}
    
    ~TcpWorker() {
        cleanup();
    }

public slots:
    void initialize(const QString &host, int port) {
        cleanup(); // 确保清理旧的连接
        
        mServer = new QTcpServer(this);
        connect(mServer, &QTcpServer::newConnection, this, &TcpWorker::onNewConnection);

        if (mServer->listen(QHostAddress(host), port)) {
            qDebug() << "Server listening on port" << port;
            emit serverReady(true);
        } else {
            qWarning() << "Failed to start server";
            emit serverReady(false);
        }
    }
    
    void cleanup() {
        // 关闭所有客户端连接
        for (QTcpSocket *clientSocket : mClientSockets) {
            if (clientSocket->isOpen()) {
                clientSocket->close();
            }
            clientSocket->deleteLater();
        }
        mClientSockets.clear();
        
        // 关闭服务器并删除
        if (mServer) {
            mServer->close();
            mServer->deleteLater();
            mServer = nullptr;
        }
    }
    
    void sendMessageToClients(const QString &message,const int &format) {
        QByteArray data;

        switch (format) {
        case 0: // HEX格式
            data = QByteArray::fromHex(message.toUtf8());
            break;
        case 1: // UTF-8格式
            data = message.toUtf8();  // 修复冒号错误为括号
            break;
        case 2: // 新增ASCII格式
            data = message.toLatin1(); // 使用本地编码（ASCII）
            break;
        default:
            data = message.toUtf8();  // 默认使用UTF-8
            break;
        }
        for (QTcpSocket *clientSocket : mClientSockets) {

            clientSocket->write(data);
        }
    }

    void sendByteArrayToClients(const QByteArray &byteArray)
    {
        for (QTcpSocket *clientSocket : mClientSockets) {

            clientSocket->write(byteArray);
        }
    }
    void sendByteArrayToClient(const QByteArray &byteArray,const QString &host)
    {
        for (QTcpSocket *clientSocket : mClientSockets) {
            if(clientSocket->peerAddress().toString() == host)
            {
                clientSocket->write(byteArray);
                return;
            }
            else
            {
                qDebug() << "Host not found";
            }

        }
    }
private slots:
    void onNewConnection() {
        QTcpSocket *clientSocket = mServer->nextPendingConnection();
        qDebug() << "New tcp client connected"<< clientSocket->peerAddress().toString();
        connect(clientSocket, &QTcpSocket::readyRead, this, &TcpWorker::onReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected, this, &TcpWorker::onDisconnected);
        mClientSockets.append(clientSocket); // Store the client socket
    }
    
    void onReadyRead() {
        for (QTcpSocket *clientSocket : mClientSockets) {
            if (clientSocket->bytesAvailable() > 0) {
                QByteArray data = clientSocket->readAll();
                QVariantMap dataMap;
                
                dataMap.insert("host", clientSocket->peerAddress().toString());
                dataMap.insert("hex", QString(data.toHex())); // 转换为QString类型
                dataMap.insert("utf-8", QString::fromUtf8(data)); // 修复UTF-8解码方式
                dataMap.insert("ascii", QString::fromLatin1(data)); // 修复ascii解码方式
                dataMap.insert("default", data);
                
                emit messageReceived(data);
                emit dataReceived(dataMap);
            }
        }
    }
    
    void onDisconnected() {
        QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
        if (clientSocket) {
            mClientSockets.removeAll(clientSocket);
            clientSocket->deleteLater();
        }
    }

signals:
    void serverReady(bool ready);
    void messageReceived(const QByteArray &data);
    void dataReceived(const QVariantMap &data);

private:
    QTcpServer *mServer;
    QList<QTcpSocket *> mClientSockets;
};

// TcpServer 实现
TcpServer::TcpServer(QString dstHost, int dstPort, QObject *parent)
        : QObject(parent), mPort(dstPort), mHost(dstHost), mServer(nullptr)
{
    // 创建工作线程
    mThread = new QThread(this);
    
    // 创建工作对象
    TcpWorker *worker = new TcpWorker();
    worker->moveToThread(mThread);
    
    // 连接信号和槽
    connect(mThread, &QThread::started, [=]() {
        worker->initialize(mHost, mPort);
    });
    connect(mThread, &QThread::finished, worker, &TcpWorker::deleteLater);
    
    // 连接工作对象的信号到 TcpServer 的信号
    connect(worker, &TcpWorker::serverReady, this, &TcpServer::isReady);
    connect(worker, &TcpWorker::messageReceived, this, &TcpServer::arrayMsg);
    connect(worker, &TcpWorker::dataReceived, this, &TcpServer::recMsg);
    
    // 连接 TcpServer 的槽到工作对象的槽
    connect(this, &TcpServer::initializeRequested, worker, &TcpWorker::initialize);
    connect(this, &TcpServer::cleanupRequested, worker, &TcpWorker::cleanup);
    connect(this, &TcpServer::sendMessageRequested, worker, &TcpWorker::sendMessageToClients);
    connect(this, &TcpServer::sendByteArrayRequested, worker, &TcpWorker::sendByteArrayToClients);
    connect(this, &TcpServer::sendByteArrayToHostRequested, worker, &TcpWorker::sendByteArrayToClient);
    
    // 启动线程
    mThread->start();
}

TcpServer::~TcpServer()
{
    // 发送清理信号
    emit cleanupRequested();
    
    // 停止线程
    mThread->quit();
    mThread->wait();
}

void TcpServer::initializeServer() {
    emit initializeRequested(mHost, mPort);
}

void TcpServer::cleanup()
{
    emit cleanupRequested();
}

void TcpServer::sendMessage(const QString &message,const int &format)
{
    emit sendMessageRequested(message,format);
}
void TcpServer::sendByteArray(const QByteArray &byteArray)
{
    emit sendByteArrayRequested(byteArray);
}

void TcpServer::sendBytesArrayToHost(const QByteArray &byteArray,const QString &host)
{
    emit sendByteArrayToHostRequested(byteArray,host);
}

void TcpServer::setHost(QString address, int port) {
    mHost = address;
    mPort = port;
    emit initializeRequested(mHost, mPort);
}
 #include "TcpServer.moc"