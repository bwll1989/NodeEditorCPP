#include "TimeSyncServer.hpp"
#include <QDebug>
#include <QNetworkDatagram>

TimeSyncServer::TimeSyncServer(QObject *parent)
    : QObject(parent)
    , socket(new QUdpSocket(this))
    , broadcastTimer(new QTimer(this))
    , currentTime(0.0)
    , speed(1.0)
    , isPaused(true)
{
    // 设置定时广播，20ms间隔
    broadcastTimer->setInterval(20);
    connect(broadcastTimer, &QTimer::timeout, this, &TimeSyncServer::broadcastTime);
    
    // 启动时就开始广播
    broadcastTimer->start();
    
    qDebug() << "UDP时间服务器已启动，将在端口34456广播";
}

TimeSyncServer::~TimeSyncServer() {
    if (socket) {
        socket->close();
    }
}

void TimeSyncServer::start() {
    startTime = Clock::now();
    currentTime = 0.0;
    isPaused = false;
    broadcastTimer->start();
}

void TimeSyncServer::stop() {
    
    currentTime = 0.00;
    isPaused = true;
}

void TimeSyncServer::pause() {
    if (!isPaused) {
        // 记录暂停时的时间点
        currentTime = getTime();
        isPaused = true;
    }
}

void TimeSyncServer::resume() {
    if (isPaused) {
        // 恢复时重新设置起始时间点，确保从暂停点继续
        startTime = Clock::now();
        isPaused = false;
    }
}

/**
 * @brief 获取当前时间
 * @return 当前时间（秒）
 */
double TimeSyncServer::getTime() const {
    if (isPaused) {

        return currentTime;  // 暂停状态返回记录的时间点
    }
    auto now = Clock::now();
    std::chrono::duration<double> elapsed = now - startTime;
    return currentTime + elapsed.count() * speed;
}

/**
 * @brief 广播时间信息
 */
void TimeSyncServer::broadcastTime() {
    // 构建JSON对象
    QJsonObject jsonObj;
    double timeToSend;
    
    if (!broadcastTimer->isActive()) {
        // 停止状态
        timeToSend = 0.0;
        jsonObj["status"] = "pause";
    } else {
        // 获取当前时间（暂停时会返回暂停时的时间点）

        timeToSend = getTime();

        jsonObj["status"] = isPaused ? "pause" : "play";
    }
    
    jsonObj["time"] = timeToSend;
    
    // 转换为JSON文档并序列化为字节数组
    QJsonDocument doc(jsonObj);
    QByteArray datagram = doc.toJson(QJsonDocument::Compact);

    // 广播数据包
    qint64 written = socket->writeDatagram(
        datagram,
        QHostAddress::Broadcast,
        34456
    );

    if (written == -1) {
        qDebug() << "广播发送失败:" << socket->error();
        return;
    }
    emit timeUpdated(timeToSend);
}

void TimeSyncServer::setCurrentTime(double time) {

    currentTime = time;
}
void TimeSyncServer::setSpeed(double newSpeed) {
    speed = newSpeed;
}


