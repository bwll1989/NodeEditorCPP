#pragma once

#include <QObject>
#include <QMutex>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

namespace Nodes
{
    /**
     * @brief Variable 触发的多通道闪避
     * Duck 为真时各音频通道按 Depth 衰减，带 Attack / Hold / Release。
     * 由 TimestampGenerator 时钟线程直接 wake，不经 QueuedConnection
     */
    class AudioDuckingWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioDuckingWorker(QObject *parent = nullptr);
        ~AudioDuckingWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void startProcessing();
        void stopProcessing();

        void setDepth(double val);
        void setAttack(double val);
        void setHold(double val);
        void setRelease(double val);
        void setDuckActive(bool active);

        void initializeBuffers(int audioChannelCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);

    private:
        struct Params {
            float depthDb = 20.0f;
            float attackSec = 0.010f;
            float holdSec = 0.200f;
            float releaseSec = 1.0f;
        };

        struct ChannelView {
            const float *data = nullptr;
            int channels = 0;
            int frames = 0;
            int sampleRate = 0;
            bool valid = false;
        };

        void audioLoop();
        void processCurrentFrame();
        void performDucking(const std::vector<AudioFrame> &inputFrames,
                            const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                            qint64 timestamp,
                            const Params &params,
                            bool duckActive);
        static ChannelView viewOf(const AudioFrame &frame);
        static float onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        QMutex _mutex;
        bool _isProcessing = false;
        qint64 _lastProcessedTimestamp = 0;

        Params _params;
        bool _duckActive = false;
        float _gainDb = 0.0f;
        float _holdLeftSec = 0.0f;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
