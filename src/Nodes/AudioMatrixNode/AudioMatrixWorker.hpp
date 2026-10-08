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
     * 各输出口仅跟随对本口有增益的输入追帧
     * 由输入环 pushFrame 级联唤醒；缺帧同戳 defer 一次再 Missing
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
        void unregisterAllInputWaitersLocked();
        int processOutputMix(int out,
                             const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                             const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                             const Eigen::MatrixXd &matrix,
                             qint64 &inoutLastTs,
                             qint64 &inoutDeferredEmptyTs,
                             qint64 clockTs,
                             int maxFramesPerWake);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        Eigen::MatrixXd _matrix;
        std::vector<qint64> _lastProcessedByOutput;
        /** 缺帧 defer：同 nextTs 连续两次失败 → Missing */
        std::vector<qint64> _deferredEmptyTsByOutput;
        QMutex _mutex;
        bool _isProcessing = false;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
