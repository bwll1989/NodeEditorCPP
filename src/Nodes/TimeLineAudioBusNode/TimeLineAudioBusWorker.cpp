#include "TimeLineAudioBusWorker.hpp"

#include <cmath>
#include <vector>

#include "Common/DataTypes/TimelineAudioBusRegistry.h"
#include "Common/AppConfig/ConstantDefines.h"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

namespace Nodes
{

using NodeDataTypes::TimelineAudioBusRegistry;

TimeLineAudioBusWorker::TimeLineAudioBusWorker(QObject* parent)
    : QObject(parent)
{
}

TimeLineAudioBusWorker::~TimeLineAudioBusWorker()
{
    stopProcessing();
}

std::shared_ptr<AudioTimestampRingQueue> TimeLineAudioBusWorker::queueForPort(unsigned int portIndex)
{
    std::lock_guard<std::mutex> locker(queuesMutex_);
    const auto it = queues_.find(portIndex);
    if (it != queues_.end()) {
        return it->second;
    }
    auto queue = std::make_shared<AudioTimestampRingQueue>();
    queues_[portIndex] = queue;
    // 队列唯一出生点在节点：以端口名为 bus 名注册，runtime 按名取写
    TimelineAudioBusRegistry::instance()->registerBus(
        QStringLiteral("BUS %1").arg(portIndex), queue);
    return queue;
}

void TimeLineAudioBusWorker::reconcilePortCount(unsigned int outPortCount)
{
    // out 端口 0 = SYNC，audio 端口 = 1..outPortCount-1
    const unsigned int wanted = (outPortCount > 0) ? (outPortCount - 1) : 0;
    std::vector<unsigned int> removed;
    {
        std::lock_guard<std::mutex> locker(queuesMutex_);
        portCount_ = wanted;
        for (auto it = queues_.begin(); it != queues_.end();) {
            if (it->first == 0 || it->first > wanted) {
                removed.push_back(it->first);
                it = queues_.erase(it);
            } else {
                ++it;
            }
        }
    }
    auto* registry = TimelineAudioBusRegistry::instance();
    for (unsigned int idx : removed) {
        registry->unregisterBus(QStringLiteral("BUS %1").arg(idx));
    }
    // 【补建缺失（2026-11-13 修）】原实现只删不建——注释声称"补建缺失"但
    // 队列只有连线时 outData→queueForPort 才懒创建，节点出生/加载存档后
    // registry 里没有任何 bus，runtime（TimelineAudioBusHost）acquire 不到
    // 队列 → 混音输出全部丢弃（"没有音频输出"根因，实测 buses=0 实锤）。
    // 出生即注册：节点构造（reconcilePortCount(OutPortCount)）就补齐全部
    // 端口队列，host 每 tick 的 buses() 刷新立刻可见。
    for (unsigned int idx = 1; idx <= wanted; ++idx) {
        queueForPort(idx);
    }
}

std::shared_ptr<AudioTimestampRingQueue> TimeLineAudioBusWorker::peekQueue(unsigned int portIndex) const
{
    std::lock_guard<std::mutex> locker(queuesMutex_);
    const auto it = queues_.find(portIndex);
    return (it != queues_.end()) ? it->second : nullptr;
}

void TimeLineAudioBusWorker::setSyncPublisher(std::shared_ptr<NodeDataTypes::SyncSharedState> publisher)
{
    syncPublisher_ = std::move(publisher);
}

void TimeLineAudioBusWorker::startProcessing()
{
    if (thread_.joinable()) {
        return;
    }
    stopRequested_.store(false, std::memory_order_release);
    tickWaiter_.reset();
    TimestampGenerator::getInstance()->registerAudioTickWaiter(&tickWaiter_);
    thread_ = std::thread([this]() { loop(); });
    Q_EMIT processingStatusChanged(true);
}

void TimeLineAudioBusWorker::stopProcessing()
{
    stopRequested_.store(true, std::memory_order_release);
    tickWaiter_.requestStop();
    if (thread_.joinable()) {
        if (thread_.get_id() != std::this_thread::get_id()) {
            thread_.join();
        } else {
            thread_.detach();
        }
    }
    TimestampGenerator::getInstance()->unregisterAudioTickWaiter(&tickWaiter_);
    Q_EMIT processingStatusChanged(false);
}

void TimeLineAudioBusWorker::setDriveMode(const QString& mode)
{
    std::lock_guard<std::mutex> locker(propsMutex_);
    driveMode_ = mode;
}

void TimeLineAudioBusWorker::setFollowClock(bool follow)
{
    std::lock_guard<std::mutex> locker(propsMutex_);
    followClock_ = follow;
}

void TimeLineAudioBusWorker::setPositionSec(double sec)
{
    std::lock_guard<std::mutex> locker(propsMutex_);
    positionSec_ = sec;
}

void TimeLineAudioBusWorker::setPlaying(bool playing)
{
    std::lock_guard<std::mutex> locker(propsMutex_);
    playing_ = playing;
}

void TimeLineAudioBusWorker::loop()
{
    AudioThreadRealtimeGuard realtimeGuard(L"TimeLineAudioBus");
    auto* clock = TimestampGenerator::getInstance();
    while (!stopRequested_.load(std::memory_order_acquire)
           && !tickWaiter_.isStopRequested()) {
        // 时钟未运行/未通知时按帧间隔超时兜底，保持响应停止请求
        tickWaiter_.wait(20);
        if (stopRequested_.load(std::memory_order_acquire)
            || tickWaiter_.isStopRequested()) {
            break;
        }
        const qint64 clockFrameCount = clock->getCurrentFrameCount();
        writeTick(clockFrameCount);
        publishSync(clockFrameCount);
    }
}

void TimeLineAudioBusWorker::writeTick(qint64 clockFrameCount)
{
    QString mode;
    {
        std::lock_guard<std::mutex> locker(propsMutex_);
        mode = driveMode_;
    }
    if (mode == QLatin1String("external")) {
        // runtime（timeline 混音侧）是唯一写者，节点不碰队列
        return;
    }

    // 快照目标队列（锁外推送，避免持锁做实时工作）
    std::vector<std::shared_ptr<AudioTimestampRingQueue>> targets;
    {
        std::lock_guard<std::mutex> locker(queuesMutex_);
        for (auto& kv : queues_) {
            if (kv.first >= 1 && kv.first <= portCount_ && kv.second) {
                targets.push_back(kv.second);
            }
        }
    }
    if (targets.empty()) {
        return;
    }

    const int frames = TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate_);
    const bool isTest = (mode == QLatin1String("test"));

    // 立体声交错 f32 块（与全链 AudioFrame 约定一致）
    QByteArray data;
    data.resize(static_cast<int>(frames * 2 * sizeof(float)));
    float* samples = reinterpret_cast<float*>(data.data());
    const float kTestGain = 0.2f;
    const double kTestFreq = 440.0;
    const double phaseStep = 2.0 * M_PI * kTestFreq / static_cast<double>(sampleRate_);
    for (int i = 0; i < frames; ++i) {
        const float v = isTest
                            ? static_cast<float>(std::sin(testPhase_) * kTestGain)
                            : 0.0f;
        samples[static_cast<size_t>(i) * 2] = v;
        samples[static_cast<size_t>(i) * 2 + 1] = v;
        testPhase_ += phaseStep;
        if (testPhase_ >= 2.0 * M_PI) {
            testPhase_ -= 2.0 * M_PI;
        }
    }

    AudioFrame frame;
    frame.data = data;
    frame.sampleRate = sampleRate_;
    frame.channels = 2;
    frame.bitsPerSample = 32;
    // 打戳口径与 AudioDecoder/Libretro 一致：全局帧计数 + FIXED_DELAY_FRAMES
    frame.timestamp = clockFrameCount + AppConfigs::FIXED_DELAY_FRAMES;

    for (auto& queue : targets) {
        queue->pushFrame(frame);
    }
}

void TimeLineAudioBusWorker::publishSync(qint64 clockFrameCount)
{
    if (!syncPublisher_) {
        return;
    }
    // followClock：播放头 = 全局时钟换算秒（时间轴与全局时钟同锚的默认约定）；
    // runtime 接入后经属性写入真实时间轴位置（followClock=false）。
    double position = 0.0;
    bool playing = false;
    {
        std::lock_guard<std::mutex> locker(propsMutex_);
        if (followClock_) {
            const double frameInterval = TimestampGenerator::getInstance()->getFrameInterval();
            position = static_cast<double>(clockFrameCount) * frameInterval;
        } else {
            position = positionSec_;
        }
        playing = playing_;
    }
    syncPublisher_->write(position, clockFrameCount, playing);
}

} // namespace Nodes
