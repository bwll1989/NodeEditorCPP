#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimeCodeDefines.h"
#include "Gist.h"
#include <memory>

namespace Nodes
{
    /**
     * @brief 音频闪避处理工作线程类
     */
    class AudioDuckingWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioDuckingWorker(QObject *parent = nullptr);
        ~AudioDuckingWorker();

    public slots:
        void startProcessing();
        void stopProcessing();
        void processAudioData();
        void onFrameTick(qint64 frameCount);

        void setThreshold(double val);
        void setRatio(double val);
        void setAttack(double val);
        void setRelease(double val);
        void setMakeupGain(double val);
        void setSidechainGain(double val);
        /**
         * @brief 设置最大压低深度（dB）
         * @param val 压低深度，单位 dB
         */
        void setDepth(double val);

        void initializeBuffers(int inputCount, int outputCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);
        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    signals:
        void processingStatusChanged(bool isProcessing);
        void audioProcessed(const std::vector<std::shared_ptr<AudioTimestampRingQueue>>& outputBuffers);

    private:
        void performDuckingOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp);
        void initializeGist(int frameSize, int sampleRate);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        QMutex _mutex;
        bool _isProcessing;
        qint64 _lastProcessedTimestamp;

        // Gist analyzer for sidechain
        std::unique_ptr<Gist<float>> _gistAnalyzer;
        int _frameSize = 2048;
        int _sampleRate = 48000;

        // Ducking parameters
        float _threshold = -20.0f; // dB
        float _ratio = 4.0f;
        float _attack = 10.0f;     // ms
        float _release = 100.0f;   // ms
        float _makeupGain = 0.0f;  // dB
        float _sidechainGain = 0.0f; // dB
        float _depthDb = 24.0f;    // 最大压低深度 dB

        // Envelope follower state
        float _envelope = 1.0f;
    };
}
