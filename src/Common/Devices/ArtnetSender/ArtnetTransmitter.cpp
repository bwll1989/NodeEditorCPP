//
// Created by bwll1 on 2024/9/1.
//

#include "ArtnetTransmitter.h"
#include <QHostAddress>
#include <QtEndian>
#include <QMutexLocker>
#include <QDebug>

// 声明全局静态实例
ArtnetTransmitter* ArtnetTransmitter::artnetInstance = nullptr;

/**
 * @brief 导出获取Art-Net传输器单例实例的函数
 */
ARTNETTRANSMITTER_EXPORT ArtnetTransmitter* getArtnetTransmitterInstance()
{
    return ArtnetTransmitter::getInstance();
}

/**
 * @brief 私有构造函数，自动启动UDP套接字服务
 */
ArtnetTransmitter::ArtnetTransmitter(QObject* parent)
    : QObject(parent), mQueueTimer (new QTimer(this))
{
    /**
     * @brief 构造函数中设置发送间隔为25ms，并在独立线程中初始化定时器与套接字
     */
    // 注册信号传递数值类型
    qRegisterMetaType<ArtnetFrame>("ArtnetFrame");
    qRegisterMetaType<ArtnetFrame>("ArtnetFrame&");
    qRegisterMetaType<QList<ArtnetFrame>>("QList<ArtnetFrame>");

    // 创建工作线程
    mThread = new QThread(this);
    this->moveToThread(mThread);

    mQueueTimer ->setInterval(mSendInterval);
    mQueueTimer ->moveToThread(mThread);
    connect(mThread , &QThread::started, this, &ArtnetTransmitter::initializeSocket);
    connect(mThread , &QThread::finished, this, &ArtnetTransmitter::cleanup);
    connect(mQueueTimer , &QTimer::timeout, this, &ArtnetTransmitter::processQueue);
    
    // 自动启动传输器
    mThread->start();
    // 在主线程中启动定时器
    QMetaObject::invokeMethod(mQueueTimer, "start", Qt::QueuedConnection);
    // 连接线程信号
  

    // 启动线程，自动初始化套接字
    mThread->start();
}

/**
 * @brief 析构函数
 */
ArtnetTransmitter::~ArtnetTransmitter()
{
    if (mThread) {
        mThread->quit();
        mThread->wait();
    }
}
void ArtnetTransmitter::initializeSocket() {
    mSocket = new QUdpSocket(this);
    if (mSocket->bind(QHostAddress::AnyIPv4, 0,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        // connect(mQueueTimer, &QTimer::timeout, this, &ArtnetTransmitter::onQueueTimer);
    }
}
/**
 * @brief 获取单例实例
 */
ArtnetTransmitter* ArtnetTransmitter::getInstance()
{
    if (artnetInstance == nullptr) {
        artnetInstance = new ArtnetTransmitter();
    }
    return artnetInstance;
}



/**
 * @brief 清理资源
 */
void ArtnetTransmitter::cleanup()
{
    if (mQueueTimer) {
        mQueueTimer->stop();
        mQueueTimer->deleteLater();
        mQueueTimer = nullptr;
    }
    
    if (mSocket) {
        mSocket->deleteLater();
        mSocket = nullptr;
    }
    
    qDebug() << "Art-Net transmitter cleaned up";
}

/**
 * @brief 批量添加Art-Net数据帧到发送队列
 */
void ArtnetTransmitter::enqueueFrames(const QList<ArtnetFrame>& frames)
{
    QMutexLocker locker(&mMutex);

    for (const ArtnetFrame& frame : frames) {
        if (frame.isValid()) {
            mSendQueue.enqueue(frame);
        } else {
            qWarning() << "Skipping invalid frame in batch";
        }
    }
    
    int queueSize = mSendQueue.size();
    locker.unlock();
    
    emit queueSizeChanged(queueSize);
    //
    // qDebug() << "Enqueued" << frames.size() << "frames, queue size:" << queueSize;
}
/**
 * @brief 单独添加Art-Net数据帧到发送队列
 */
void ArtnetTransmitter::enqueueFrame(const ArtnetFrame& frame)
{
    QMutexLocker locker(&mMutex);


    if (frame.isValid()) {
        mSendQueue.enqueue(frame);
    } else {
        qWarning() << "Skipping invalid frame in batch";
    }


    int queueSize = mSendQueue.size();
    locker.unlock();

    emit queueSizeChanged(queueSize);
}
/**
 * @brief 清空发送队列
 */
void ArtnetTransmitter::clearQueue()
{
    QMutexLocker locker(&mMutex);
    mSendQueue.clear();
    locker.unlock();
    
    emit queueSizeChanged(0);
    qDebug() << "Send queue cleared";
}

/**
 * @brief 获取队列中待发送的帧数量
 */
int ArtnetTransmitter::getQueueSize() const
{
    QMutexLocker locker(&mMutex);
    return mSendQueue.size();
}

    /**
     * @brief 批量发送队列中的所有帧
     *
     * 为减少锁持有时间，先将当前队列中的帧拷贝到本地列表，再释放锁后逐一发送。
     */
void ArtnetTransmitter::processQueue()
{

    QMutexLocker locker(&mMutex);

    if (mSendQueue.isEmpty()) {
        return;
    }

    // 将当前队列全部取出到本地列表
    QList<ArtnetFrame> framesToSend;
    framesToSend.reserve(mSendQueue.size());
    while (!mSendQueue.isEmpty()) {
        framesToSend.append(mSendQueue.dequeue());
    }
    int queueSize = mSendQueue.size(); // 此处应为0
    locker.unlock();

    // 逐一发送
    for (const ArtnetFrame& f : framesToSend) {
        bool success = sendFrameInternal(f);
        if (!success) {
            qWarning() << "Failed to send frame, dropping";
            // 可选：失败重试或重新入队，这里选择丢弃
        }
    }

    // 队列被清空后广播队列大小
    emit queueSizeChanged(queueSize);
}

 /**
     * @brief 定时器回调：每25ms检查队列并批量发送
     */
void ArtnetTransmitter::onQueueTimer()
{
   
    processQueue();
}

/**
 * @brief 实际发送数据包的内部函数
 */
bool ArtnetTransmitter::sendFrameInternal(const ArtnetFrame& frame)
{
    if (!mSocket) {
        emit frameSendFailed(frame, "Socket not initialized");
        return false;
    }
    
    // 创建Art-Net数据包
    QByteArray packet = createArtnetPacket(frame);
    
    // 发送数据包
    QHostAddress targetAddress(frame.host);
    qint64 bytesWritten = mSocket->writeDatagram(packet, targetAddress, frame.port);
    
    if (bytesWritten == -1) {
        QString errorString = mSocket->errorString();
        qWarning() << "Failed to send Art-Net packet:" << errorString;
        emit frameSendFailed(frame, errorString);
        return false;
    } else {
        emit frameSent(frame, bytesWritten);
        return true;
    }
}

/**
 * @brief 创建Art-Net数据包
 */
QByteArray ArtnetTransmitter::createArtnetPacket(const ArtnetFrame& frame)
{
    QByteArray packet;
    packet.resize(18 + 512); // Art-Net头部18字节 + DMX数据512字节
    
    // Art-Net标识符 "Art-Net\0"
    packet[0] = 'A'; packet[1] = 'r'; packet[2] = 't'; packet[3] = '-';
    packet[4] = 'N'; packet[5] = 'e'; packet[6] = 't'; packet[7] = 0x00;
    
    // 操作码 OpDmx (0x5000) - 小端序
    qToLittleEndian<quint16>(0x5000, reinterpret_cast<uchar*>(packet.data() + 8));
    
    // 协议版本 (0x000E) - 大端序
    qToBigEndian<quint16>(0x000E, reinterpret_cast<uchar*>(packet.data() + 10));
    
    // 序列号
    packet[12] = frame.sequence;
    
    // 物理端口
    packet[13] = 0x00;
    
    // 宇宙编号 - 小端序
    qToLittleEndian<quint16>(frame.universe, reinterpret_cast<uchar*>(packet.data() + 14));
    
    // 数据长度 (512) - 大端序
    qToBigEndian<quint16>(512, reinterpret_cast<uchar*>(packet.data() + 16));
    
    // DMX数据
    QByteArray dmxData = frame.getFormattedDmxData();
    packet.replace(18, 512, dmxData);
    
    return packet;
}


