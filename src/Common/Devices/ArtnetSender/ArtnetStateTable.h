//
// Created by WuBin on 2026/11/8.
// Art-Net 输出状态表：缓存所有输出端点 × universe 的最新 DMX 状态，
// 支持多写入者（owner）分桶与 HTP/LTP 合成；由 ArtnetService 持有并定期取快照发送。
//

#pragma once

#include <QHash>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QtGlobal>
#include <algorithm>
#include "ArtnetFrame.h"
#include "Common/AppConfig/ConstantDefines.h"

/**
 * @brief Art-Net 输出端点标识（目标地址即端点，广播与单播地址地位相同）
 *
 * 不同 host:port 之间互不合并；同一端点内按 universe 分槽。
 */
struct ArtnetEndPoint
{
    QString host;              ///< 目标主机地址（如 "192.168.0.255" 广播 / "192.168.1.50" 单播）
    quint16 port = 6454;       ///< 目标端口，Art-Net 默认 6454

    bool operator==(const ArtnetEndPoint& o) const { return port == o.port && host == o.host; }
};

/// QHash 对端点标识的哈希支持
inline size_t qHash(const ArtnetEndPoint& key, size_t seed = 0)
{
    return qHash(key.host, seed) ^ (qHash(key.port) << 1);
}

/**
 * @brief Art-Net 输出状态表（纯数据逻辑类，无 QObject、无线程）
 *
 * 三层结构与需求维度一一对应：
 *   host:port  —— 发给谁（广播/单播天然隔离，互不合并）
 *   universe   —— 发什么（一个 ArtDmx 包 = 一个 universe，协议决定）
 *   owner 桶   —— 谁在写（多源合并的最小必要状态）
 *
 * 合并语义：
 *   HTP（默认）：逐通道取所有桶的最大值，两路同写不闪烁（强度合并标准做法）
 *   LTP       ：最近更新的桶整包胜出（适合"同一时刻只有一个源活跃"的接管场景）
 *
 * 线程模型：内部互斥锁自保护；snapshot() 在锁内完成拷贝与合成，
 * 返回的快照在锁外使用，天然保证"锁内拷贝、锁外发送"的纪律。
 */
class ArtnetStateTable
{
public:
    /// 单个写入者的数据桶：该源最后一次写入的 512B 数据 + 写入时间戳
    struct InputBucket
    {
        QByteArray dmx;           ///< 该源最后一次写入的 512 字节 DMX 数据
        qint64 updateMs = 0;      ///< 最后写入时间戳（毫秒，LTP 合成依据）
    };

    /// 单个 universe 的状态槽：每个写入者一个桶 + 发送序号
    struct UniverseSlot
    {
        QHash<void*, InputBucket> inputs; ///< 写入者(this指针，仅作身份标签) → 数据桶
        quint8 sequence = 0;              ///< 发送序号，由 snapshot() 每周期自增（255 回绕到 1，0 保留）
    };

    /// 快照条目：一次发送周期需要发出的一个 ArtDmx 包的全部信息
    struct SnapshotEntry
    {
        ArtnetEndPoint endpoint;  ///< 目标端点
        quint16 universe = 0;     ///< universe 编号（15 位端口地址）
        QByteArray dmx;           ///< 按当前策略合成后的 512B 数据
        quint8 sequence = 0;      ///< 本槽发送序号
    };

    ArtnetStateTable() = default;

    /**
     * @brief 写入/覆盖：生产者数据变化时调用
     *
     * 只做桶覆盖（最后写入者赢），O(1)，无队列。同一写入者反复更新只覆盖自己的桶；
     * 多个写入者各占一个桶，由 snapshot() 按策略合成。
     * @param f     待写入的帧（host/universe/dmx 有效字段被使用）
     * @param owner 写入者身份标签，惯例传生产者节点的 this 指针；nullptr 表示匿名写入者
     */
    void updateFrame(const ArtnetFrame& f, void* owner)
    {
        // 无效帧（如 host 填错、universe 越界）静默丢弃：
        // 固定重发模式下生产者会以 50Hz 持续写入同一帧，若在此打日志会刷屏，
        // 排查配置问题时请观察节点 isReady 状态（发送失败信号）而非此处
        if (!f.isValid()) {
            return;
        }
        QMutexLocker locker(&mMutex);
        InputBucket& bucket = mTable[{f.host, f.port}][f.universe].inputs[owner];
        bucket.dmx = f.getFormattedDmxData();
        bucket.updateMs = QDateTime::currentMSecsSinceEpoch();
    }

    /**
     * @brief 释放某属主参与的所有桶（节点析构/配置变化时调用）
     *
     * 桶空 → 删槽；端点的 universe 全空 → 删端点，实现确定性的状态回收。
     * @param owner     要释放的属主身份标签
     * @param zeroFirst true 时，对因释放而清空的槽补发一包全零（"删节点归零"策略）；
     *                  补发通过下一次 snapshot() 完成，不阻塞调用者
     */
    void releaseOwner(void* owner, bool zeroFirst = false)
    {
        QMutexLocker locker(&mMutex);
        for (auto endpointIt = mTable.begin(); endpointIt != mTable.end(); /* 手动推进 */) {
            auto& universes = endpointIt.value();
            for (auto slotIt = universes.begin(); slotIt != universes.end(); /* 手动推进 */) {
                slotIt->inputs.remove(owner);
                if (slotIt->inputs.isEmpty()) {
                    // 槽已无任何写入者：按需补发全零后删除
                    if (zeroFirst) {
                        PendingZero pz;
                        pz.endpoint = endpointIt.key();
                        pz.universe = slotIt.key();
                        mPendingZeros.append(pz);
                    }
                    slotIt = universes.erase(slotIt);
                } else {
                    ++slotIt;
                }
            }
            // 端点下已无任何 universe：删除整个端点
            if (universes.isEmpty()) {
                endpointIt = mTable.erase(endpointIt);
            } else {
                ++endpointIt;
            }
        }
    }

    /**
     * @brief 取快照：按当前合并策略合成所有活跃槽，并推进各槽发送序号
     *
     * 每个发送周期只应被发送线程调用一次（序号自增的依据）。
     * @param mode 合并策略（HTP/LTP）
     * @return 本次周期需要发送的全部数据包描述，锁外使用
     */
    QList<SnapshotEntry> snapshot(ArtnetMergeMode mode)
    {
        QMutexLocker locker(&mMutex);
        QList<SnapshotEntry> out;

        // 1. 冲刷待补发的全零包（属主释放归零策略）
        for (const PendingZero& pz : mPendingZeros) {
            SnapshotEntry entry;
            entry.endpoint = pz.endpoint;
            entry.universe = pz.universe;
            entry.dmx = QByteArray(512, 0x00);
            entry.sequence = 0; // 槽已删除，序号不可考，Art-Net 规定 0 = 接收端不校验
            out.append(entry);
        }
        mPendingZeros.clear();

        // 2. 遍历所有端点 × universe，按策略合成（非 const 迭代：需要推进槽内发送序号）
        for (auto endpointIt = mTable.begin(); endpointIt != mTable.end(); ++endpointIt) {
            for (auto slotIt = endpointIt->begin(); slotIt != endpointIt->end(); ++slotIt) {
                SnapshotEntry entry;
                entry.endpoint = endpointIt.key();
                entry.universe = slotIt.key();
                entry.dmx = mergeInputs(slotIt->inputs, mode);
                entry.sequence = nextSequence(slotIt->sequence);
                slotIt->sequence = entry.sequence; // 序号在槽内持续推进
                out.append(entry);
            }
        }
        return out;
    }

    /**
     * @brief 查询当前有数据的输出 universe 列表（去重、升序，最多 maxCount 个）
     *
     * 供 ArtPollReply 上报端口信息：让 MA/QLC+ 等控制台
     * 在设备发现时看到 Flow 正在输出的 universe。
     * @param maxCount 最多返回的数量（ArtPollReply 每包最多 4 个端口）
     */
    QList<quint16> activeOutputUniverses(int maxCount = 4)
    {
        QMutexLocker locker(&mMutex);
        QSet<quint16> seen;
        QList<quint16> out;
        for (auto endpointIt = mTable.constBegin(); endpointIt != mTable.constEnd() && out.size() < maxCount; ++endpointIt) {
            for (auto slotIt = endpointIt->constBegin(); slotIt != endpointIt->constEnd() && out.size() < maxCount; ++slotIt) {
                if (!slotIt->inputs.isEmpty() && !seen.contains(slotIt.key())) {
                    seen.insert(slotIt.key());
                    out.append(slotIt.key());
                }
            }
        }
        std::sort(out.begin(), out.end());
        return out;
    }

private:
    /// 待补发全零包的描述（属主释放后槽已删除，先记账、下个周期发送）
    struct PendingZero
    {
        ArtnetEndPoint endpoint;
        quint16 universe = 0;
    };

    /// 发送序号推进：0 起步跳到 1，255 回绕到 1（0 按 Art-Net 规范保留给"不校验"）
    static quint8 nextSequence(quint8 seq)
    {
        quint8 next = static_cast<quint8>(seq + 1);
        if (next == 0) next = 1;
        return next;
    }

    /**
     * @brief 按策略合成一个槽内所有写入者的数据
     * @param inputs 写入者 → 数据桶
     * @param mode   合并策略
     * @return 合成后的 512B 数据；单写入者时为直通拷贝，无额外开销
     */
    static QByteArray mergeInputs(const QHash<void*, InputBucket>& inputs, ArtnetMergeMode mode)
    {
        if (inputs.isEmpty()) {
            return QByteArray(512, 0x00);
        }
        if (inputs.size() == 1) {
            return inputs.constBegin()->dmx; // 单源直通
        }

        QByteArray result(512, 0x00);
        if (mode == ArtnetMergeMode::HTP) {
            // HTP：逐通道取所有桶的最大值（强度合并标准做法，多源同写不闪烁）
            for (auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
                const QByteArray& dmx = it->dmx;
                const int n = qMin(512, dmx.size());
                for (int i = 0; i < n; ++i) {
                    const quint8 v = static_cast<quint8>(dmx.at(i));
                    if (v > static_cast<quint8>(result.at(i))) {
                        result[i] = static_cast<char>(v);
                    }
                }
            }
        } else {
            // LTP：最近更新的桶整包胜出（槽级 LTP，适合单源活跃的接管场景）
            const InputBucket* latest = nullptr;
            for (auto it = inputs.constBegin(); it != inputs.constEnd(); ++it) {
                if (!latest || it->updateMs > latest->updateMs) {
                    latest = &it.value();
                }
            }
            if (latest) {
                result = latest->dmx.leftJustified(512, 0x00);
            }
        }
        return result;
    }

    QHash<ArtnetEndPoint, QHash<quint16, UniverseSlot>> mTable; ///< 端点 → universe → 槽
    QList<PendingZero> mPendingZeros;                           ///< 待补发的全零包队列
    mutable QMutex mMutex;                                      ///< 保护全部状态的互斥锁
};
