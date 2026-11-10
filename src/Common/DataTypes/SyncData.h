//
// Created by WuBin on 2026/11/09.
// 节点间同步载体：以 TimestampGenerator 全局帧计数为唯一时间基准。
// SMPTE timecode 不进入图内——它只在边界节点做双向转换（TC→内部秒→SyncState→TC）。
//
#pragma once
#include "DataTypesExport.h"
#include "QtNodes/NodeData"
#include <QMutex>
#include <QString>
#include <memory>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

namespace NodeDataTypes
{
    /**
     * @brief 同步状态快照（值语义，线程安全读取的产物）
     * 由 SyncSharedState::read() 产生，跟随端基于它外推源此刻应处的位置。
     * 时间基准：TimestampGenerator 全局帧计数（Flow 图内唯一的同步时钟）。
     */
    struct DATATYPES_EXPORT SyncState {
        double positionSec = 0.0;     ///< 写入时刻的播放位置（秒，相对媒体/时间轴起点）
        qint64 anchorFrameCount = 0;  ///< 写入 positionSec 时对应的全局帧计数（锚点）
        bool playing = false;         ///< 时钟源是否处于播放态
        bool active = false;          ///< 时钟源是否有效（从未写入或已失效为 false）
        quint64 version = 0;          ///< 每次写入自增；跟随端用于心跳/失联检测
        QString sourceId;             ///< 时钟源标识（预留多源仲裁）

        /**
         * @brief 外推当前时刻源应处的位置
         * playing 且有效时按全局帧计数差外推；暂停/无效时返回定格位置。
         * @param nowFrameCount 当前全局帧计数（TimestampGenerator::getCurrentFrameCount）
         * @param frameIntervalSec 全局帧间隔（秒，即 1.0 / getFrameRate()）
         * @return 外推后的期望位置（秒）
         */
        double expectedSec(qint64 nowFrameCount, double frameIntervalSec) const;
    };

    /**
     * @brief 跨线程共享的同步状态（发布端写，N 个跟随端读）
     * 仿 AudioTimestampRingQueue 的共享句柄模式：端口连线传递 shared_ptr，
     * 状态数据不随端口推送流动，各端按自己的节奏轮询读取，天然支持一对多。
     */
    class DATATYPES_EXPORT SyncSharedState
    {
    public:
        SyncSharedState() = default;
        explicit SyncSharedState(const QString &sourceId);

        /**
         * @brief 发布端写入一次快照（version 自增，active 置 true）
         * @param positionSec 写入时刻的播放位置（秒）
         * @param anchorFrameCount 写入时刻对应的全局帧计数（TimestampGenerator::getCurrentFrameCount）
         * @param playing 是否处于播放态
         */
        void write(double positionSec, qint64 anchorFrameCount, bool playing);

        /**
         * @brief 源失效/恢复（不递增 version，仅影响 active）
         */
        void setActive(bool active);

        /**
         * @brief 读取状态快照（互斥保护下的值拷贝）
         */
        SyncState read() const;

        /**
         * @brief 便捷方法：读取快照并用全局时钟外推到"此刻"的期望位置
         * 等价于 read().expectedSec(getCurrentFrameCount(), 1.0/getFrameRate())
         * @return 外推后的期望位置（秒）；源无效或暂停时为定格位置
         */
        double expectedNowSec() const;

    private:
        mutable QMutex mutex;
        SyncState state;
    };

    /**
     * @brief 节点端口数据类型：Sync（节点间同步载体）
     * 发布端（AudioDecoder/VideoDecoder 主机、时间线等）持续写入共享状态；
     * 跟随端（从机）通过端口连线取得同一共享句柄，在自己的循环里读取+外推。
     */
    class DATATYPES_EXPORT SyncData : public NodeData
    {
    public:
        SyncData();

        explicit SyncData(std::shared_ptr<SyncSharedState> state);

        NodeDataType type() const override;

        /**
         * @brief 设置共享同步状态（用于连接建立时）
         */
        void setSharedState(std::shared_ptr<SyncSharedState> state);

        /**
         * @brief 获取共享同步状态指针
         * @return 共享状态指针，nullptr 表示未连接
         */
        std::shared_ptr<SyncSharedState> getSharedState() const;

        /**
         * @brief 检查是否已连接到共享状态
         */
        bool isConnectedToSharedState() const;

    private:
        std::shared_ptr<SyncSharedState> sharedState_;
    };
}
