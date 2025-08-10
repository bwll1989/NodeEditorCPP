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
    : QObject(parent), mThread(nullptr), mSocket(nullptr), mQueueTimer(nullptr), mSendInterval(20)
{
    // 注册信号传递数值类型
    qRegisterMetaType<ArtnetFrame>("ArtnetFrame");
    qRegisterMetaType<ArtnetFrame>("ArtnetFrame&");
    qRegisterMetaType<QList<ArtnetFrame>>("QList<ArtnetFrame>");
    
    // 创建工作线程
    mThread = new QThread(this);
    this->moveToThread(mThread);
    
    // 连接线程信号
    connect(mThread, &QThread::started, this, &ArtnetTransmitter::initializeSocket);
    connect(mThread, &QThread::finished, this, &ArtnetTransmitter::cleanup);
    
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
 * @brief 初始化UDP套接字
 */
void ArtnetTransmitter::initializeSocket()
{
    if (mSocket) {
        return;
    }
    
    mSocket = new QUdpSocket(this);
    
    // 创建队列处理定时器
    mQueueTimer = new QTimer(this);
    connect(mQueueTimer, &QTimer::timeout, this, &ArtnetTransmitter::onQueueTimer);
    mQueueTimer->start(mSendInterval); // 默认20ms间隔
    
    // qDebug() << "Art-Net transmitter initialized with" << mSendInterval << "ms send interval";
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
 * @brief 添加Art-Net数据帧到发送队列
 */
void ArtnetTransmitter::enqueueFrame(const ArtnetFrame& frame)
{
    if (!frame.isValid()) {
        qWarning() << "Invalid ArtnetFrame, skipping";
        return;
    }
    
    QMutexLocker locker(&mMutex);
    mSendQueue.enqueue(frame);
    int queueSize = mSendQueue.size();
    locker.unlock();
    
    emit queueSizeChanged(queueSize);
    
    qDebug() << "Enqueued frame for universe" << frame.universe 
             << "to" << frame.host << "queue size:" << queueSize;
}

/**
 * @brief 立即发送Art-Net数据帧
 */
bool ArtnetTransmitter::sendFrame(const ArtnetFrame& frame)
{
    if (!frame.isValid()) {
        qWarning() << "Invalid ArtnetFrame";
        emit frameSendFailed(frame, "Invalid frame data");
        return false;
    }
    
    return sendFrameInternal(frame);
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
    
    qDebug() << "Enqueued" << frames.size() << "frames, queue size:" << queueSize;
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
 * @brief 处理发送队列
 */
void ArtnetTransmitter::processQueue()
{
    QMutexLocker locker(&mMutex);
    
    if (mSendQueue.isEmpty()) {
        return;
    }
    
    ArtnetFrame frame = mSendQueue.dequeue();
    int queueSize = mSendQueue.size();
    locker.unlock();
    
    // 发送数据帧
    bool success = sendFrameInternal(frame);
    
    if (success) {
        emit queueSizeChanged(queueSize);
    } else {
        // 发送失败，可以选择重新入队或丢弃
        qWarning() << "Failed to send frame, dropping";
    }
}

/**
 * @brief 定时处理队列
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


