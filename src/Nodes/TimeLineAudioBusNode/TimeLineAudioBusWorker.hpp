#pragma once

#include <atomic>
#include <memory>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <QByteArray>
#include <QString>

#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "Common/DataTypes/SyncData.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

namespace Nodes
{

/**
 * @brief TimeLineAudioBus 写侧线程（AudioTickWaiter 驱动，无 Qt 事件循环依赖）
 *
 * 职责（2026-11-13 实施方案 T2）：
 *  1. 队列簿记：对齐 OutPortCount（懒创建缺失、注销多余），创建即以端口名
 *     "BUS <i>" 注册进 TimelineAudioBusRegistry（runtime 按名取写）；
 *  2. 驱动模式 driveMode：
 *     - external（默认）：节点不写队列——runtime（timeline 混音侧）是唯一写者；
 *     - test：节点写 440Hz 正弦（链路联通冒烟，接入 runtime 前用）；
 *     - silence：节点写静音块（保持时间戳链不断）。
 *     单写者铁律：任一时刻每个队列只有一个写者（节点或 runtime 二选一）。
 *  3. 同步发布：每拍把播放头快照写入 SyncSharedState（0 号口 SyncData 载体）。
 *
 * 时间戳口径（全系统统一）：块 timestamp = 全局帧计数 + FIXED_DELAY_FRAMES，
 * 与 AudioDecoder/Libretro 等源节点同口径；块长 = getSamplesPerFrame(48000)
 * （50fps → 960 恰整除；非整除 fps 的 Bresenham 改造归 runtime 写入侧统一做）。
 */
class TimeLineAudioBusWorker : public QObject
{
    Q_OBJECT

public:
    explicit TimeLineAudioBusWorker(QObject* parent = nullptr);
    ~TimeLineAudioBusWorker() override;

    /// 端口 i (>=1) 的输出队列（懒创建并注册 registry；GUI 线程调用）
    std::shared_ptr<AudioTimestampRingQueue> queueForPort(unsigned int portIndex);

    /// 对齐端口数（GUI 线程在端口增删后调用）：补建缺失、注销多余
    void reconcilePortCount(unsigned int outPortCount);

    /// 队列快照（GUI 线程 outData 用；不创建）
    std::shared_ptr<AudioTimestampRingQueue> peekQueue(unsigned int portIndex) const;

    /// 同步发布端（0 号口 SyncData 载体，节点构造时创建、worker 持引用写入）
    void setSyncPublisher(std::shared_ptr<NodeDataTypes::SyncSharedState> publisher);

public slots:
    void startProcessing();
    void stopProcessing();

    void setDriveMode(const QString& mode);   ///< external / test / silence
    void setFollowClock(bool follow);
    void setPositionSec(double sec);
    void setPlaying(bool playing);

signals:
    void processingStatusChanged(bool running);

private:
    void loop();
    /// 本拍写一个块进全部 bus 队列（仅 test/silence 模式）
    void writeTick(qint64 clockFrameCount);
    /// 同步发布：播放头快照（线程安全）
    void publishSync(qint64 clockFrameCount);

    unsigned int portCount_ = 0;               ///< 期望 audio 端口数（= OutPortCount-1）
    std::map<unsigned int, std::shared_ptr<AudioTimestampRingQueue>> queues_; ///< portIndex → queue
    mutable std::mutex queuesMutex_;

    std::shared_ptr<NodeDataTypes::SyncSharedState> syncPublisher_;

    QString driveMode_ = QStringLiteral("external");
    bool    followClock_ = true;
    double  positionSec_ = 0.0;
    bool    playing_ = false;
    std::mutex propsMutex_;

    int      sampleRate_ = 48000;
    double   testPhase_ = 0.0;          ///< test 模式正弦相位（跨拍连续）

    AudioTickWaiter tickWaiter_;
    std::atomic<bool> stopRequested_{false};
    std::thread thread_;
};

} // namespace Nodes
