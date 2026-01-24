#include "NodeTimeSync.hpp"
#include <QDebug>

NodeTimeSync::NodeTimeSync(QObject *parent)
    : QObject(parent)
    , broadcastTimer(new QTimer(this))
    , speed(1.0)
    , isPaused(true)
    , tickIntervalSec(0.04)
    , m_errorAccumulator(0.0)
{
    // 使用单次触发模式，以便动态调整间隔
    broadcastTimer->setSingleShot(true);
    connect(broadcastTimer, &QTimer::timeout, this, &NodeTimeSync::broadcastTime);
    
    // 构造时处于暂停状态，不启动定时器
}

NodeTimeSync::~NodeTimeSync() {
    if (broadcastTimer) {
        broadcastTimer->stop();
    }
}

void NodeTimeSync::start() {
    isPaused = false;
    m_errorAccumulator = 0.0;
    m_lastTickTime = Clock::now();
    // 立即启动或按间隔启动？通常 start 意味着开始运行，第一个 tick 可以稍后
    // 为了防止立即触发导致的不均匀，按标准间隔启动
    int intervalMs = static_cast<int>(tickIntervalSec * 1000.0);
    broadcastTimer->start(intervalMs);
}

void NodeTimeSync::stop() {
    isPaused = true;
    broadcastTimer->stop();
    m_errorAccumulator = 0.0;
}

void NodeTimeSync::pause() {
    if (!isPaused) {
        isPaused = true;
        broadcastTimer->stop();
    }
}

void NodeTimeSync::resume() {
    if (isPaused) {
        isPaused = false;
        // 恢复时重置上一次时间为当前，避免将暂停时间计入误差
        m_lastTickTime = Clock::now();
        // 保持之前的累积误差？通常暂停意味着时间中断，误差也应该重置，或者为了平滑可以保留
        // 这里选择重置误差，避免暂停期间的系统调度影响
        m_errorAccumulator = 0.0; 
        
        int intervalMs = static_cast<int>(tickIntervalSec * 1000.0);
        broadcastTimer->start(intervalMs);
    }
}

/**
 * @brief 设置时钟的 tick 间隔时间
 * @param intervalSec tick 间隔时间（秒）
 */
void NodeTimeSync::setTickInterval(double intervalSec)
{
    if (intervalSec <= 0.0) {
        return;
    }
    tickIntervalSec = intervalSec;
    // 如果正在运行，下一次 broadcastTime 会自动使用新的间隔计算
}

/**
 * @brief 广播时间信息
 */
void NodeTimeSync::broadcastTime() {
    if (isPaused) {
        return;
    }

    auto now = Clock::now();
    double targetIntervalMs = tickIntervalSec * 1000.0;
    
    // 计算实际经过的时间（毫秒）
    std::chrono::duration<double, std::milli> elapsed = now - m_lastTickTime;
    double actualIntervalMs = elapsed.count();
    
    // 计算本次误差：实际时间 - 目标时间
    // 正值表示 晚了（实际用了更多时间），需要减少下一次等待
    // 负值表示 早了（实际用的时间少），需要增加下一次等待
    double error = actualIntervalMs - targetIntervalMs;
    
    // 累积误差
    m_errorAccumulator += error;
    
    // 限制累积误差范围，防止极端情况导致的爆发或长时间停滞
    // 例如限制在 ±1秒范围内
    if (m_errorAccumulator > 1000.0) m_errorAccumulator = 1000.0;
    if (m_errorAccumulator < -1000.0) m_errorAccumulator = -1000.0;
    
    // 计算下一次等待时间
    int nextInterval = static_cast<int>(targetIntervalMs - m_errorAccumulator);
    
    // 确保间隔非负
    if (nextInterval < 0) {
        nextInterval = 0;
    }
    
    // 更新上一次时间
    m_lastTickTime = now;
    
    // 启动下一次定时器
    broadcastTimer->start(nextInterval);

    // 发送时间更新信号
    // 注意：这里发送的是逻辑上的 delta，即 tickIntervalSec * speed
    // TimeLineNodeClock 会根据这个 delta（或者仅仅是收到信号）来推进帧
    double delta = tickIntervalSec * speed;
    emit timeUpdated(delta);
}


void NodeTimeSync::setSpeed(double newSpeed) {  
    speed = newSpeed;
}
