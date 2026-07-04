#pragma once

#include <QtGlobal>
#include <atomic>
#include <cstdint>
#include <utility>
#include <vector>

namespace TimestampRingQueueDetail
{
/**
 * @brief 基于 seqlock 的环形槽位，单写者多读者下避免互斥锁
 */
template <typename FrameT>
struct LockFreeSlot {
    std::atomic<uint32_t> seq;
    FrameT frame;

    LockFreeSlot() : seq(0) {}

    LockFreeSlot(const LockFreeSlot&) = delete;
    LockFreeSlot& operator=(const LockFreeSlot&) = delete;

    LockFreeSlot(LockFreeSlot&& other) noexcept
        : seq(other.seq.load(std::memory_order_relaxed))
        , frame(std::move(other.frame))
    {
        other.seq.store(0, std::memory_order_relaxed);
    }

    LockFreeSlot& operator=(LockFreeSlot&& other) noexcept
    {
        if (this != &other) {
            seq.store(other.seq.load(std::memory_order_relaxed), std::memory_order_relaxed);
            frame = std::move(other.frame);
            other.seq.store(0, std::memory_order_relaxed);
        }
        return *this;
    }
};

template <typename FrameT>
inline bool readRingEntry(const LockFreeSlot<FrameT>& ringSlot, FrameT& out)
{
    for (int attempt = 0; attempt < 32; ++attempt) {
        const uint32_t seqBefore = ringSlot.seq.load(std::memory_order_acquire);
        if (seqBefore & 1u) {
            continue;
        }
        out = ringSlot.frame;
        const uint32_t seqAfter = ringSlot.seq.load(std::memory_order_acquire);
        if (seqBefore == seqAfter) {
            return true;
        }
    }
    return false;
}

template <typename FrameT>
inline void writeRingEntry(LockFreeSlot<FrameT>& ringSlot, FrameT in)
{
    const uint32_t seq = ringSlot.seq.load(std::memory_order_relaxed);
    ringSlot.seq.store(seq + 1, std::memory_order_release);
    ringSlot.frame = std::move(in);
    ringSlot.seq.store(seq + 2, std::memory_order_release);
}

template <typename FrameT>
inline bool lookupFrameByTimestamp(const std::vector<LockFreeSlot<FrameT>>& ringSlots,
                                   int maxSize,
                                   qint64 latestTimestamp,
                                   int latestIndex,
                                   qint64& lastHitTimestamp,
                                   int& lastHitIndex,
                                   qint64 targetFrameCount,
                                   FrameT& frame)
{
    if (maxSize <= 0 || ringSlots.empty()) {
        return false;
    }

    if (lastHitIndex >= 0 && lastHitIndex < maxSize) {
        FrameT cached;
        if (readRingEntry(ringSlots[static_cast<size_t>(lastHitIndex)], cached)) {
            if (cached.timestamp == targetFrameCount) {
                frame = std::move(cached);
                lastHitTimestamp = cached.timestamp;
                return true;
            }

            if (cached.timestamp + 1 == targetFrameCount) {
                const int nextIdx = (lastHitIndex + 1) % maxSize;
                FrameT nextFrame;
                if (readRingEntry(ringSlots[static_cast<size_t>(nextIdx)], nextFrame) &&
                    nextFrame.timestamp == targetFrameCount) {
                    frame = std::move(nextFrame);
                    lastHitIndex = nextIdx;
                    lastHitTimestamp = nextFrame.timestamp;
                    return true;
                }
            }
        }
    }

    if (latestTimestamp < 0 || latestIndex < 0) {
        return false;
    }

    const qint64 delta = latestTimestamp - targetFrameCount;
    if (delta < 0 || delta >= maxSize) {
        return false;
    }

    int idx = latestIndex - static_cast<int>(delta);
    idx %= maxSize;
    if (idx < 0) {
        idx += maxSize;
    }

    FrameT hit;
    if (readRingEntry(ringSlots[static_cast<size_t>(idx)], hit) && hit.timestamp == targetFrameCount) {
        frame = std::move(hit);
        lastHitTimestamp = hit.timestamp;
        lastHitIndex = idx;
        return true;
    }

    constexpr int scanRadius = 4;
    for (int offset = 1; offset <= scanRadius; ++offset) {
        const int idxR = (idx + offset) % maxSize;
        FrameT right;
        if (readRingEntry(ringSlots[static_cast<size_t>(idxR)], right) && right.timestamp == targetFrameCount) {
            frame = std::move(right);
            lastHitTimestamp = right.timestamp;
            lastHitIndex = idxR;
            return true;
        }

        int idxL = (idx - offset) % maxSize;
        if (idxL < 0) {
            idxL += maxSize;
        }
        FrameT left;
        if (readRingEntry(ringSlots[static_cast<size_t>(idxL)], left) && left.timestamp == targetFrameCount) {
            frame = std::move(left);
            lastHitTimestamp = left.timestamp;
            lastHitIndex = idxL;
            return true;
        }
    }

    return false;
}
} // namespace TimestampRingQueueDetail
