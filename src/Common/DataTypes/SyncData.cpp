//
// Created by WuBin on 2026/11/09.
//
#include "SyncData.h"

#include "TimestampGenerator/TimestampGenerator.hpp"

using namespace NodeDataTypes;

double SyncState::expectedSec(qint64 nowFrameCount, double frameIntervalSec) const
{
    // 暂停/无效/帧间隔非法时不外推，返回定格位置
    if (!active || !playing || frameIntervalSec <= 0.0) {
        return positionSec;
    }
    return positionSec + double(nowFrameCount - anchorFrameCount) * frameIntervalSec;
}

SyncSharedState::SyncSharedState(const QString &src)
{
    state.sourceId = src;
}

void SyncSharedState::write(double positionSec, qint64 anchorFrameCount, bool playing)
{
    QMutexLocker locker(&mutex);
    state.positionSec = positionSec;
    state.anchorFrameCount = anchorFrameCount;
    state.playing = playing;
    state.active = true;
    ++state.version;
}

void SyncSharedState::setActive(bool active)
{
    QMutexLocker locker(&mutex);
    state.active = active;
}

SyncState SyncSharedState::read() const
{
    QMutexLocker locker(&mutex);
    return state;
}

double SyncSharedState::expectedNowSec() const
{
    const SyncState snap = read();
    auto *gen = TimestampGenerator::getInstance();
    const qint64 nowFrame = gen ? gen->getCurrentFrameCount() : snap.anchorFrameCount;
    const double intervalSec = gen ? (1.0 / gen->getFrameRate()) : 0.0;
    return snap.expectedSec(nowFrame, intervalSec);
}

SyncData::SyncData() = default;

SyncData::SyncData(std::shared_ptr<SyncSharedState> state)
    : sharedState_(std::move(state))
{
}

NodeDataType SyncData::type() const
{
    return NodeDataType{ QStringLiteral("sync"), QStringLiteral("Sync") };
}

void SyncData::setSharedState(std::shared_ptr<SyncSharedState> state)
{
    sharedState_ = std::move(state);
}

std::shared_ptr<SyncSharedState> SyncData::getSharedState() const
{
    return sharedState_;
}

bool SyncData::isConnectedToSharedState() const
{
    return sharedState_ != nullptr;
}
