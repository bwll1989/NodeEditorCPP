#pragma once

#include <QObject>
#include <QMutex>
#include <QVector>
#include <thread>
#include <atomic>
#include "NodeDataList.hpp"
#include "Common/AppConfig/TimeCodeDefines.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <ltc.h>
static const int SAMPLE_RATE = 48000;
// 源端固定超前 1 拍：同拍被 wake 的下游按「当前帧」取数时，能取到上一拍已写入的块。
static constexpr int kLtcSourceLeadFrames = 1;

namespace Nodes
{
    /**
     * @brief LTC生成工作线程类
     * 在独立线程中生成 LTC（Linear Timecode）波形，并写入共享音频环形缓冲区供下游消费
     */
    class LTCGeneratorWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit LTCGeneratorWorker(QObject *parent = nullptr);
        ~LTCGeneratorWorker();

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer() const;

    public slots:
        void startProcessing();
        void stopProcessing();
        void resetTimecode();
        void setEnabled(bool enabled);
        void setTimeCodeType(TimeCodeType type);
        void setVolume(float db);

    public:
        /**
         * @brief 绑定同步源（SYNC→LTC 跟随模式）
         * 生成循环每轮读取源快照：源播放中按全局时钟外推期望位置，
         * 帧差超过死区（1帧）时重置编码器时间码；源暂停/失效时输出静音定格。
         * 未绑定则保持原有自由生成行为。
         */
        void setSyncSource(std::shared_ptr<NodeDataTypes::SyncSharedState> src);

        /**
         * @brief 设置 SYNC 秒 → LTC 时间码的映射偏移（秒，可负）
         */
        void setSyncOffsetSec(double sec);

    signals:
        void timeCodeFrameGenerated(TimeCodeFrame frame);
        void processingStatusChanged(bool isProcessing);

    private:
        void generationLoop();
        void audioLoop();
        void processCurrentFrame();
        void ensureEncoderLocked();
        int encodeOneLtcFrameLocked();
        void takeSamplesLocked(int samplesNeeded, QVector<float>& out);
        TimeCodeFrame toTimeCodeFrameLocked(const SMPTETimecode& t) const;
        void advancePlaybackFramesLocked(int consumedSamples);
        int getBufferSize() const;
        void syncEncoderFromTimecodeLocked();
        void applyDropFrameFlagLocked();
        void followSyncLocked();
        void secToSmpteLocked(double sec, SMPTETimecode& out) const;
        qint64 smpteToTotalFramesLocked(const SMPTETimecode& t) const;

        std::shared_ptr<AudioTimestampRingQueue> _outputBuffer;
        mutable QMutex _mutex;
        std::atomic<bool> _isProcessing{false};
        std::atomic<bool> _running{false};
        std::unique_ptr<std::thread> _generationThread;
        bool _enabled = true;
        float _volumeDb = -25.0f;

        LTCEncoder* _encoder = nullptr;
        SMPTETimecode _timecode {};
        double _fps = 25.0;
        double _configuredFps = 0.0;
        LTC_TV_STANDARD _configuredStandard = LTC_TV_FILM_24;
        TimeCodeType _forcedType = TimeCodeType::PAL;

        QVector<float> _pendingSamples;
        int _pendingReadOffset = 0;

        QVector<int> _frameSampleCounts;
        QVector<SMPTETimecode> _frameTimecodes;
        int _frameQueueHead = 0;
        int _frameHeadConsumed = 0;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopTickRequested{false};
        std::thread _tickThread;
        qint64 _lastOutputTimestamp = 0;

        // ---- SYNC→LTC 跟随（SyncData 体系）----
        std::shared_ptr<NodeDataTypes::SyncSharedState> _syncSource; ///< 外部同步源（未绑定=自由生成）
        double _syncOffsetSec = 0.0;      ///< SYNC 秒 → LTC 时间码偏移
        bool _syncHold = false;           ///< 源暂停/失效：输出静音、时间码定格（仅生成线程访问）
    };
}
