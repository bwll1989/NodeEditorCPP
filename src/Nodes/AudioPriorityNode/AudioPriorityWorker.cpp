#include "AudioPriorityWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace Nodes
{
    AudioPriorityWorker::AudioPriorityWorker(QObject *parent)
        : QObject(parent)
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
        , _frameSize(2048)
        , _sampleRate(48000)
    {
        initializeGist(_frameSize, _sampleRate);
    }
    
    AudioPriorityWorker::~AudioPriorityWorker()
    {
        stopProcessing();
    }
    
    void AudioPriorityWorker::initializeGist(int frameSize, int sampleRate)
    {
        _frameSize = frameSize;
        _sampleRate = sampleRate;
        try {
            _gistAnalyzer = std::make_unique<Gist<float>>(frameSize, sampleRate);
        } catch (const std::exception& e) {
            qWarning() << "AudioPriorityWorker: Failed to initialize GIST:" << e.what();
        }
    }

    void AudioPriorityWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) return;
        
        _isProcessing = true;
        _lastProcessedTimestamp = 0;
        QObject::connect(TimestampGenerator::getInstance(),
                         &TimestampGenerator::frameCountUpdated,
                         this,
                         &AudioPriorityWorker::onFrameTick,
                         Qt::QueuedConnection);
        emit processingStatusChanged(true);
    }
    
    void AudioPriorityWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) {
            _isProcessing = false;
            QObject::disconnect(TimestampGenerator::getInstance(),
                                &TimestampGenerator::frameCountUpdated,
                                this,
                                &AudioPriorityWorker::onFrameTick);
            emit processingStatusChanged(false);
        }
    }
    
    void AudioPriorityWorker::processAudioData()
    {
        // Not used when driven by TimestampGenerator, but kept for compatibility
    }

    void AudioPriorityWorker::onFrameTick(qint64 frameCount)
    {
        if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) return;
        if (frameCount == _lastProcessedTimestamp) return;
        
        // We expect at least Input 0 (Music) to process. Input 1 (Sidechain) is optional (if missing, no ducking).
        if (_inputBuffers.size() < 1) return;

        std::vector<AudioFrame> inputFrames(_inputBuffers.size());
        bool hasMusicInput = false;
        
        // Get Music Frame (Input 0)
        if (_inputBuffers[0] && _inputBuffers[0]->isActive()) {
            if (_inputBuffers[0]->getFrameByTimestamp(frameCount, inputFrames[0])) {
                hasMusicInput = true;
            }
        }
        
        // Get Sidechain Frame (Input 1)
        if (_inputBuffers.size() > 1 && _inputBuffers[1] && _inputBuffers[1]->isActive()) {
            _inputBuffers[1]->getFrameByTimestamp(frameCount, inputFrames[1]);
        }
        
        if (!hasMusicInput) return;

        performDuckingOperation(inputFrames, frameCount);
        _lastProcessedTimestamp = frameCount;
        emit audioProcessed(_outputBuffers);
    }
    
    /**
     * @brief 执行优先级选择：有高优先级则输出高优先级，否则输出低优先级
     * @param inputFrames 输入帧数组，0为低优先级，1为高优先级
     * @param timestamp 帧时间戳
     */
    void AudioPriorityWorker::performDuckingOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp)
    {
        if (inputFrames.empty() || inputFrames[0].data.isEmpty()) return;

        const AudioFrame& lowPriority = inputFrames[0];
        bool hasHigh = (inputFrames.size() > 1 && !inputFrames[1].data.isEmpty());
        const AudioFrame& highPriority = hasHigh ? inputFrames[1] : lowPriority;

        // --- 保留原检测原理与参数：对高优先级输入做侧链分析 ---
        // 构建侧链数据并应用侧链增益
        std::vector<float> sidechainData;
        if (hasHigh) {
            size_t scSamples = highPriority.data.size() / sizeof(float);
            if (scSamples > 0) {
                const float* scRaw = reinterpret_cast<const float*>(highPriority.data.constData());
                sidechainData.assign(scRaw, scRaw + scSamples);
                // 对齐到 _frameSize
                if (sidechainData.size() < (size_t)_frameSize) {
                    size_t originalSize = sidechainData.size();
                    sidechainData.resize(_frameSize);
                    for (size_t i = originalSize; i < (size_t)_frameSize; ++i) {
                        sidechainData[i] = sidechainData[i % originalSize];
                    }
                } else if (sidechainData.size() > (size_t)_frameSize) {
                    sidechainData.resize(_frameSize);
                }
                if (_sidechainGain > 0.001f) {
                    float scGainLinear = std::pow(10.0f, _sidechainGain / 20.0f);
                    for (float& s : sidechainData) s *= scGainLinear;
                }
            }
        }

        // 计算 RMS（保留 Gist 分析调用）
        float rmsDb = -100.0f;
        if (!sidechainData.empty()) {
            double sumSq = 0.0;
            for (float s : sidechainData) sumSq += s * s;
            float rms = std::sqrt((float)(sumSq / sidechainData.size()));
            rmsDb = (rms > 0.000001f) ? 20.0f * std::log10(rms) : -100.0f;
            if (_gistAnalyzer) {
                _gistAnalyzer->processAudioFrame(sidechainData);
            }
        }

        // 使用阈值、比率、最大压低深度来计算“激活强度”，仅用于优先级选择，不改变音量
        float presenceTarget = 0.0f;
        if (hasHigh && rmsDb > _threshold) {
            float overshoot = rmsDb - _threshold;
            float activationDb = std::min(_depthDb, overshoot * _ratio); // 保留 ratio/depth 的影响
            presenceTarget = activationDb / std::max(0.001f, _depthDb);  // 归一化到 [0,1]
        }

        // 用 Attack/Release 平滑激活强度（保留原包络思想）
        size_t sampleCount = lowPriority.data.size() / sizeof(float);
        float dt = (float)sampleCount / _sampleRate;
        float attackCoeff = std::exp(-dt / std::max(0.001f, (_attack / 1000.0f)));
        float releaseCoeff = std::exp(-dt / std::max(0.001f, (_release / 1000.0f)));
        float currentPresence = _envelope;
        if (presenceTarget > currentPresence) {
            _envelope = attackCoeff * currentPresence + (1.0f - attackCoeff) * presenceTarget;
        } else {
            _envelope = releaseCoeff * currentPresence + (1.0f - releaseCoeff) * presenceTarget;
        }

        // 计算压低增益（dB转线性），并进行混合输出
        const float duckDb = -_envelope * _depthDb;
        const float makeupDb = _makeupGain;
        const float duckLinear = std::pow(10.0f, (duckDb + makeupDb) / 20.0f);
        const float mixRatio = hasHigh ? std::clamp(_envelope, 0.0f, 1.0f) : 0.0f;

        const float* lowPtr = reinterpret_cast<const float*>(lowPriority.data.constData());
        size_t lowSamples = lowPriority.data.size() / sizeof(float);

        const float* highPtr = hasHigh ? reinterpret_cast<const float*>(highPriority.data.constData()) : nullptr;
        size_t highSamples = hasHigh ? (highPriority.data.size() / sizeof(float)) : 0;

        size_t outSamples = lowSamples;
        std::vector<float> mixed(outSamples, 0.0f);
        for (size_t i = 0; i < outSamples; ++i) {
            const float lowVal = lowPtr ? lowPtr[i] * duckLinear : 0.0f;
            const float highVal = (hasHigh && i < highSamples) ? highPtr[i] : 0.0f;
            mixed[i] = lowVal * (1.0f - mixRatio) + highVal * mixRatio;
        }

        AudioFrame outputFrame;
        outputFrame.sampleRate = lowPriority.sampleRate;
        outputFrame.channels = lowPriority.channels;
        outputFrame.bitsPerSample = lowPriority.bitsPerSample;
        outputFrame.timestamp = timestamp + 2;
        outputFrame.data = QByteArray(reinterpret_cast<const char*>(mixed.data()), static_cast<int>(mixed.size() * sizeof(float)));

        if (!_outputBuffers.empty() && _outputBuffers[0]) {
            _outputBuffers[0]->pushFrame(outputFrame);
        }
    }

    void AudioPriorityWorker::initializeBuffers(int inputCount, int outputCount)
    {
        if (_isProcessing) stopProcessing();
        
        _inputBuffers.clear();
        _inputBuffers.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _inputBuffers[i] = std::make_shared<AudioTimestampRingQueue>();
        }
        
        _outputBuffers.clear();
        _outputBuffers.resize(outputCount);
        for (int i = 0; i < outputCount; ++i) {
            _outputBuffers[i] = std::make_shared<AudioTimestampRingQueue>();
        }
    }

    void AudioPriorityWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < (int)_inputBuffers.size()) {
            _inputBuffers[port] = buffer;
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioPriorityWorker::getOutputBuffer(int port)
    {
        if (port >= 0 && port < (int)_outputBuffers.size()) {
            return _outputBuffers[port];
        }
        return nullptr;
    }

    void AudioPriorityWorker::setThreshold(double val) { QMutexLocker l(&_mutex); _threshold = (float)val; }
    void AudioPriorityWorker::setRatio(double val) { QMutexLocker l(&_mutex); _ratio = (float)val; }
    void AudioPriorityWorker::setAttack(double val) { QMutexLocker l(&_mutex); _attack = (float)val; }
    void AudioPriorityWorker::setRelease(double val) { QMutexLocker l(&_mutex); _release = (float)val; }
    void AudioPriorityWorker::setMakeupGain(double val) { QMutexLocker l(&_mutex); _makeupGain = (float)val; }
    void AudioPriorityWorker::setSidechainGain(double val) { QMutexLocker l(&_mutex); _sidechainGain = (float)val; }
    /**
     * @brief 设置最大压低深度（dB）
     * @param val 压低深度，单位 dB
     */
    void AudioPriorityWorker::setDepth(double val) { QMutexLocker l(&_mutex); _depthDb = (float)val; }
}
