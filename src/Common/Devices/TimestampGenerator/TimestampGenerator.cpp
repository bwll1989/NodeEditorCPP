#include "TimestampGenerator.hpp"
#include <QDebug>
#include <QCoreApplication>

// 静态成员初始化
TimestampGenerator* TimestampGenerator::instance_ = nullptr;
QMutex TimestampGenerator::instanceMutex_;
const double TimestampGenerator::FRAME_INTERVAL_MS = 1000.0 / FRAME_RATE;
const std::chrono::nanoseconds TimestampGenerator::FRAME_INTERVAL_NS = 
    std::chrono::nanoseconds(static_cast<long long>(1000000000.0 / FRAME_RATE));

/**
 * @brief 导出函数：获取帧计数器生成器实例
 */
TIMESTAMPGENERATOR TimestampGenerator* getTimestampGeneratorInstance()
{
    return TimestampGenerator::getInstance();
}

/**
 * @brief 获取全局帧计数器生成器实例（线程安全的单例模式）
 * 实例化时自动启动帧计数生成
 */
TimestampGenerator* TimestampGenerator::getInstance()
{
    // 双重检查锁定模式
    if (instance_ == nullptr) {
        QMutexLocker locker(&instanceMutex_);
        if (instance_ == nullptr) {
            instance_ = new TimestampGenerator();
            // 实例化后自动启动
            instance_->start();
        }
    }
    return instance_;
}

/**
 * @brief 私有构造函数
 * 构造时初始化但不启动，等待getInstance()调用start()
 */
TimestampGenerator::TimestampGenerator(QObject *parent)
    : QObject(parent)
    , frameCounter_(0)
    , isRunning_(false)
    , shouldStop_(false)
    , timerThread_(nullptr)
    , baseAbsoluteTime_(0)
{
    // qDebug() << "FrameCounter created with frequency:" << FRAME_RATE << "fps, interval:" << FRAME_INTERVAL_MS << "ms";
}

/**
 * @brief 析构函数
 */
TimestampGenerator::~TimestampGenerator()
{
    stop();
    // qDebug() << "FrameCounter destroyed";
}

/**
 * @brief 启动帧计数生成
 */
void TimestampGenerator::start()
{
    QMutexLocker locker(&frameCountMutex_);
    
    if (!isRunning_.load()) {
        // 重置帧计数器和标志
        frameCounter_ = 0;
        shouldStop_ = false;
        isRunning_ = true;
        
        // 记录开始时间
        startTime_ = std::chrono::high_resolution_clock::now();
        baseAbsoluteTime_ = QDateTime::currentMSecsSinceEpoch();
        
        // 启动高精度计时线程
        timerThread_ = std::make_unique<std::thread>(&TimestampGenerator::timerThreadFunction, this);
        
        // qDebug() << "FrameCounter started with high precision timer at:" << baseAbsoluteTime_;
        
        // 发送初始帧信息
        FrameInfo initialFrame = createFrameInfo(0);
        emit frameInfoUpdated(initialFrame);
        emit frameCountUpdated(0);
        emit started();
    }
}

/**
 * @brief 停止帧计数生成
 */
void TimestampGenerator::stop()
{
    if (isRunning_.load()) {
        // 设置停止标志
        shouldStop_ = true;
        isRunning_ = false;
        
        // 等待线程结束
        if (timerThread_ && timerThread_->joinable()) {
            timerThread_->join();
        }
        timerThread_.reset();
        
        // qDebug() << "FrameCounter stopped";
        emit stopped();
    }
}

/**
 * @brief 重新启动帧计数生成
 */
void TimestampGenerator::restart()
{
    stop();
    start();
}

/**
 * @brief 获取当前帧信息
 * @return 当前帧信息（包含帧计数和绝对时间）
 */
FrameInfo TimestampGenerator::getCurrentFrameInfo() const
{
    qint64 currentFrame = frameCounter_.loadAcquire();
    return createFrameInfo(currentFrame);
}

/**
 * @brief 获取当前全局帧计数
 * @return 当前帧计数（从0开始的int64）
 */
qint64 TimestampGenerator::getCurrentFrameCount() const
{
    return frameCounter_.loadAcquire();
}

/**
 * @brief 获取下一个帧计数
 * @return 下一个帧计数
 */
qint64 TimestampGenerator::getNextFrameCount()
{
    return frameCounter_.fetchAndAddAcquire(1) + 1;
}

/**
 * @brief 获取下一个帧信息
 * @return 下一个帧信息
 */
FrameInfo TimestampGenerator::getNextFrameInfo()
{
    qint64 nextFrame = frameCounter_.fetchAndAddAcquire(1) + 1;
    return createFrameInfo(nextFrame);
}

/**
 * @brief 重置帧计数器
 */
void TimestampGenerator::resetFrameCount()
{
    QMutexLocker locker(&frameCountMutex_);
    frameCounter_ = 0;
    startTime_ = std::chrono::high_resolution_clock::now();
    baseAbsoluteTime_ = QDateTime::currentMSecsSinceEpoch();
    
    FrameInfo resetFrame = createFrameInfo(0);
    emit frameInfoUpdated(resetFrame);
    emit frameCountUpdated(0);
    // qDebug() << "FrameCounter reset to 0 at:" << baseAbsoluteTime_;
}

/**
 * @brief 检查帧计数器是否正在运行
 * @return 运行状态
 */
bool TimestampGenerator::isRunning() const
{
    return isRunning_.load();
}

/**
 * @brief 创建当前帧信息
 * @param frameCount 帧计数
 * @return 帧信息
 */
FrameInfo TimestampGenerator::createFrameInfo(qint64 frameCount) const
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    qint64 absoluteTime = baseAbsoluteTime_ + 
        std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime_).count();
    
    return FrameInfo(frameCount, absoluteTime, currentTime);
}

/**
 * @brief 高精度计时线程函数
 */
void TimestampGenerator::timerThreadFunction()
{
    auto nextFrameTime = startTime_;
    
    while (!shouldStop_.load()) {
        // 计算下一帧的时间点
        nextFrameTime += FRAME_INTERVAL_NS;
        
        // 高精度睡眠到下一帧时间
        std::this_thread::sleep_until(nextFrameTime);
        
        // 检查是否需要停止
        if (shouldStop_.load()) {
            break;
        }
        
        // 生成帧计数
        generateFrameCount();
    }
}

/**
 * @brief 生成帧计数
 */
void TimestampGenerator::generateFrameCount()
{
    // 递增帧计数器
    qint64 currentFrame = frameCounter_.fetchAndAddAcquire(1);
    
    // 创建帧信息
    FrameInfo frameInfo = createFrameInfo(currentFrame);
    
    // 发送信号
    emit frameInfoUpdated(frameInfo);
    emit frameCountUpdated(currentFrame);
    
    // // 每1000帧输出一次调试信息
    // if (currentFrame % 1000 == 0) {
    //     double actualFps = currentFrame * 1000.0 / frameInfo.getRelativeTimeMs(startTime_);
    //     double theoreticalTime = frameInfo.getTheoreticalTimeMs(FRAME_RATE);
    //     double actualTime = frameInfo.getRelativeTimeMs(startTime_);
    //     double drift = actualTime - theoreticalTime;
    //
    //     qDebug() << "FrameCounter frame:" << currentFrame
    //              << "absolute time:" << frameInfo.absoluteTimeMs
    //              << "relative time:" << actualTime << "ms"
    //              << "theoretical time:" << theoreticalTime << "ms"
    //              << "drift:" << drift << "ms"
    //              << "actual fps:" << actualFps
    //     << QDateTime::currentMSecsSinceEpoch();
    // }
}

/**
 * @brief 根据时间差计算帧计数
 * @param timeDeltaMs 时间差（毫秒），可以为正数（向前）或负数（向后）
 * @return 计算得到的帧计数
 */
qint64 TimestampGenerator::calculateFrameCountByTimeDelta(double timeDeltaMs) const
{
    // 获取当前帧计数
    qint64 currentFrame = frameCounter_.loadAcquire();
    
    // 计算时间差对应的帧数变化
    double frameDelta = timeDeltaMs / FRAME_INTERVAL_MS;
    
    // 计算新的帧计数（四舍五入到最近的整数）
    qint64 newFrameCount = currentFrame + static_cast<qint64>(std::round(frameDelta));
    
    // 确保帧计数不为负数
    return std::max(static_cast<qint64>(0), newFrameCount);
}

/**
 * @brief 根据时间差计算帧信息
 * @param timeDeltaMs 时间差（毫秒），可以为正数（向前）或负数（向后）
 * @return 计算得到的帧信息
 */
FrameInfo TimestampGenerator::calculateFrameInfoByTimeDelta(double timeDeltaMs) const
{
    // 计算新的帧计数
    qint64 newFrameCount = calculateFrameCountByTimeDelta(timeDeltaMs);
    
    // 计算对应的时间点
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::milliseconds(static_cast<long long>(timeDeltaMs));
    auto newTime = currentTime + deltaTime;
    
    // 计算新的绝对时间
    qint64 newAbsoluteTime = baseAbsoluteTime_ + 
        std::chrono::duration_cast<std::chrono::milliseconds>(newTime - startTime_).count();
    
    return FrameInfo(newFrameCount, newAbsoluteTime, newTime);
}

/**
 * @brief 根据绝对时间计算帧计数
 * @param absoluteTimeMs 绝对时间（毫秒）
 * @return 计算得到的帧计数，如果时间早于基准时间则返回-1
 */
qint64 TimestampGenerator::calculateFrameCountByAbsoluteTime(qint64 absoluteTimeMs) const
{
    // 检查时间是否早于基准时间
    if (absoluteTimeMs < baseAbsoluteTime_) {
        return -1; // 表示无效时间
    }
    
    // 计算相对于基准时间的偏移
    double relativeTimeMs = static_cast<double>(absoluteTimeMs - baseAbsoluteTime_);
    
    // 根据相对时间计算帧计数
    return calculateFrameCountByRelativeTime(relativeTimeMs);
}

/**
 * @brief 根据相对时间计算帧计数
 * @param relativeTimeMs 相对于开始时间的时间（毫秒）
 * @return 计算得到的帧计数
 */
qint64 TimestampGenerator::calculateFrameCountByRelativeTime(double relativeTimeMs) const
{
    // 确保时间不为负数
    if (relativeTimeMs < 0) {
        return 0;
    }
    
    // 根据帧率计算帧计数
    double frameCount = relativeTimeMs / FRAME_INTERVAL_MS;
    
    // 四舍五入到最近的整数
    return static_cast<qint64>(std::round(frameCount));
} 
