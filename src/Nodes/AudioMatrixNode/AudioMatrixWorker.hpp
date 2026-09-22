#pragma once

#include <QObject>
#include <QMutex>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "Eigen/Core"

namespace Nodes
{
    /**
     * @brief 音频矩阵混音工作线程
     * 由 TimestampGenerator 时钟线程直接 wake，不经 QueuedConnection
     */
    class AudioMatrixWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioMatrixWorker(QObject *parent = nullptr);
        ~AudioMatrixWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void updateMatrix(const Eigen::MatrixXd& matrix);
        void startProcessing();
        void stopProcessing();
        void processAudioData();

        void initializeBuffers(int inputCount, int outputCount, const Eigen::MatrixXd& matrix);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);
        void audioProcessed(const std::vector<std::shared_ptr<AudioTimestampRingQueue>>& outputBuffers);

    private:
        void audioLoop();
        void processCurrentFrame();
        void performMatrixOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        Eigen::MatrixXd _matrix;
        QMutex _mutex;
        bool _isProcessing = false;
        qint64 _lastProcessedTimestamp = 0;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
