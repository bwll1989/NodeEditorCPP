//
// Created by WuBin on 2025/12/19.
//

#include "SpoutReceiver.h"

SpoutReceiver::SpoutReceiver(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
    m_worker = new SpoutReceiverWorker(this);
    
    // 使用 QueuedConnection 确保跨线程信号安全传输
    connect(m_worker, &SpoutReceiverWorker::frameReceived, this, &SpoutReceiver::frameReceived, Qt::QueuedConnection);
    connect(m_worker, &SpoutReceiverWorker::connectionStatusChanged, this, &SpoutReceiver::connectionStatusChanged, Qt::QueuedConnection);
}

SpoutReceiver::~SpoutReceiver()
{
    stop();
    // 确保线程安全退出
    if (m_worker->isRunning()) {
        m_worker->stop();
        m_worker->wait();
    }
}

void SpoutReceiver::start(const QString& senderName)
{
    setSenderName(senderName);

    if (!m_worker->isRunning()) {
        qDebug()<<"is not running";
        m_worker->start();
    }

}

void SpoutReceiver::stop()
{
    m_worker->stop();
}

void SpoutReceiver::setSenderName(const QString& senderName)
{
    m_worker->setSenderName(senderName);
}

QStringList SpoutReceiver::getSenderList()
{
    QStringList senders;
    // 创建临时的 Spout 实例来查询共享内存
    SPOUTHANDLE spout = GetSpout();
    if (spout) {
        int count = spout->GetSenderCount();
        for (int i = 0; i < count; i++) {
            char name[256];
            if (spout->GetSender(i, name, 256)) {
                senders.append(QString::fromLocal8Bit(name));
            }
        }
        spout->Release();
    }
    return senders;
}
