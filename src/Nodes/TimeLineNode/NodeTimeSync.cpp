#include "NodeTimeSync.hpp"
#include <QDebug>

NodeTimeSync::NodeTimeSync(QObject *parent)
    : QObject(parent)
    , broadcastTimer(new QTimer(this))
    , currentTime(0.0)
    , speed(1.0)
    , isPaused(true)
{
    // 设置定时广播，20ms间隔
    broadcastTimer->setInterval(40);
    connect(broadcastTimer, &QTimer::timeout, this, &NodeTimeSync::broadcastTime);
    // 启动时就开始广播
    broadcastTimer->start();
}

NodeTimeSync::~NodeTimeSync() {

}

void NodeTimeSync::start() {
    startTime = Clock::now();
    currentTime = 0.0;
    isPaused = false;
    broadcastTimer->start();
}

void NodeTimeSync::stop() {
    
    currentTime = 0.00;
    isPaused = true;
}

void NodeTimeSync::pause() {
    if (!isPaused) {
        // 记录暂停时的时间点
        currentTime = getTime();
        isPaused = true;
    }
}

void NodeTimeSync::resume() {
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
double NodeTimeSync::getTime() const {
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
void NodeTimeSync::broadcastTime() {
    // 构建JSON对象
    double timeToSend;
    
    if (!broadcastTimer->isActive()) {
        // 停止状态
        timeToSend = 0.0;
    } else {
        // 获取当前时间（暂停时会返回暂停时的时间点）
        timeToSend = getTime();
    }
    emit timeUpdated(timeToSend);
}

void NodeTimeSync::setCurrentTime(double time) {
    currentTime = time;
}
void NodeTimeSync::setSpeed(double newSpeed) {
    speed = newSpeed;
}
