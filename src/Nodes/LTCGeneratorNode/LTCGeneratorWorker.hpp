#pragma once

#include <QObject>
#include <QMutex>
#include <QVector>
#include <thread>
#include <atomic>
#include "NodeDataList.hpp"
#include "TimeCodeDefines.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <ltc.h>
static const int SAMPLE_RATE = 48000;
// 源端固定超前 1 拍：同拍被 wake 的下游按「当前帧」取数时，能取到上一拍已写入的块。
// 不要用设置里的「音频输出延时」——那是处理节点（Router/VST 等）叠加用的。
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
    };
}
