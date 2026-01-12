#include "AudioCrossFaderWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace Nodes
{
    AudioCrossFaderWorker::AudioCrossFaderWorker(QObject *parent)
        : QObject(parent)
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
        , _sampleRate(48000)
    {
        // nothing
    }
    
    AudioCrossFaderWorker::~AudioCrossFaderWorker()
    {
        stopProcessing();
    }
    
    void AudioCrossFaderWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) return;
        
        _isProcessing = true;
        _lastProcessedTimestamp = 0;
        QObject::connect(TimestampGenerator::getInstance(),
                         &TimestampGenerator::frameCountUpdated,
                         this,
                         &AudioCrossFaderWorker::onFrameTick,
                         Qt::QueuedConnection);
        emit processingStatusChanged(true);
    }
    
    void AudioCrossFaderWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) {
            _isProcessing = false;
            QObject::disconnect(TimestampGenerator::getInstance(),
                                &TimestampGenerator::frameCountUpdated,
                                this,
                                &AudioCrossFaderWorker::onFrameTick);
            emit processingStatusChanged(false);
        }
    }
    
    void AudioCrossFaderWorker::processAudioData()
    {
        // Not used when driven by TimestampGenerator, but kept for compatibility
    }

    void AudioCrossFaderWorker::onFrameTick(qint64 frameCount)
    {
        if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) return;
        if (frameCount == _lastProcessedTimestamp) return;
        
        // 自动淡入淡出：根据时间推进 mix
        if (_fadingActive) {
            if (frameCount >= _fadeEndFrame) {
                _mix = _fadeTargetMix;
                _fadingActive = false;
            } else if (frameCount >= _fadeStartFrame) {
                qint64 span = std::max<qint64>(1, _fadeEndFrame - _fadeStartFrame);
                double progress = double(frameCount - _fadeStartFrame) / double(span);
                float newMix = _fadeStartMix + (float)progress * (_fadeTargetMix - _fadeStartMix);
                _mix = std::clamp(newMix, 0.0f, 1.0f);
            }
        }
        
        // Expect two inputs for crossfade: A (0) and B (1)
        if (_inputBuffers.size() < 2) return;

        std::vector<AudioFrame> inputFrames(_inputBuffers.size());
        bool hasA = false;
        bool hasB = false;
        
        // Get A (Input 0)
        if (_inputBuffers[0] && _inputBuffers[0]->isActive()) {
            if (_inputBuffers[0]->getFrameByTimestamp(frameCount, inputFrames[0])) {
                hasA = true;
            }
        }
        
        // Get B (Input 1)
        if (_inputBuffers.size() > 1 && _inputBuffers[1] && _inputBuffers[1]->isActive()) {
            if (_inputBuffers[1]->getFrameByTimestamp(frameCount, inputFrames[1])) {
                hasB = true;
            }
        }
        
        if (!hasA && !hasB) return;

        performCrossFadeOperation(inputFrames, frameCount);
        _lastProcessedTimestamp = frameCount;
        emit audioProcessed(_outputBuffers);
    }
    
    void AudioCrossFaderWorker::performCrossFadeOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp)
    {

        bool hasA = !(inputFrames.size() < 1 || inputFrames[0].data.isEmpty());
        bool hasB = !(inputFrames.size() < 2 || inputFrames[1].data.isEmpty());
        if (!hasA && !hasB) return;

        const uint8_t* aRaw = hasA ? reinterpret_cast<const uint8_t*>(inputFrames[0].data.constData()) : nullptr;
        const uint8_t* bRaw = hasB ? reinterpret_cast<const uint8_t*>(inputFrames[1].data.constData()) : nullptr;
        int aBytesPerSample = hasA ? inputFrames[0].bitsPerSample / 8 : 0;
        int bBytesPerSample = hasB ? inputFrames[1].bitsPerSample / 8 : 0;
        size_t aCount = hasA ? inputFrames[0].data.size() / std::max(1, aBytesPerSample) : 0;
        size_t bCount = hasB ? inputFrames[1].data.size() / std::max(1, bBytesPerSample) : 0;
        size_t sampleCount = std::max(aCount, bCount);
        if (sampleCount == 0) return;

        // Compute weights
        float mix = _mix;
        // Always use equal-power curve
        float theta = mix * static_cast<float>(M_PI_2);
        float wA = std::cos(theta);
        float wB = std::sin(theta);
        // Passthrough fallback when only one input is present
        if (!hasA && hasB) { wA = 0.0f; wB = 1.0f; }
        if (hasA && !hasB) { wA = 1.0f; wB = 0.0f; }

        // Prepare output
        AudioFrame outputFrame;
        const AudioFrame& refIn = hasA ? inputFrames[0] : inputFrames[1];
        outputFrame.sampleRate = refIn.sampleRate > 0 ? refIn.sampleRate : _sampleRate;
        outputFrame.channels = 1;
        outputFrame.bitsPerSample = 32;
        outputFrame.timestamp = timestamp + 2; // small latency compensation
        outputFrame.data.resize(sampleCount * sizeof(float));
        float* out = reinterpret_cast<float*>(outputFrame.data.data());

        // Convert inputs to float32 if needed and mix
        for (size_t i = 0; i < sampleCount; ++i) {
            float a = 0.0f;
            float b = 0.0f;
            if (hasA && i < aCount) {
                if (inputFrames[0].bitsPerSample == 32) {
                    a = reinterpret_cast<const float*>(aRaw)[i];
                } else if (inputFrames[0].bitsPerSample == 16) {
                    a = static_cast<float>(reinterpret_cast<const int16_t*>(aRaw)[i]) / 32768.0f;
                }
            }
            if (hasB && i < bCount) {
                if (inputFrames[1].bitsPerSample == 32) {
                    b = reinterpret_cast<const float*>(bRaw)[i];
                } else if (inputFrames[1].bitsPerSample == 16) {
                    b = static_cast<float>(reinterpret_cast<const int16_t*>(bRaw)[i]) / 32768.0f;
                }
            }
            out[i] = a * wA + b * wB;
        }
        
        if (!_outputBuffers.empty() && _outputBuffers[0]) {  
            _outputBuffers[0]->pushFrame(outputFrame);
        }
    }

    void AudioCrossFaderWorker::initializeBuffers(int inputCount, int outputCount)
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

    void AudioCrossFaderWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < (int)_inputBuffers.size()) {
            _inputBuffers[port] = buffer;
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioCrossFaderWorker::getOutputBuffer(int port)
    {
        if (port >= 0 && port < (int)_outputBuffers.size()) {
            return _outputBuffers[port];
        }
        return nullptr;
    }

    void AudioCrossFaderWorker::setMix(double val) { QMutexLocker l(&_mutex); _mix = std::clamp((float)val, 0.0f, 1.0f); }
    /**
     * @brief 设置淡入淡出时长（毫秒）
     * @param ms 时长，单位毫秒
     */
    void AudioCrossFaderWorker::setFadeDuration(double ms) { QMutexLocker l(&_mutex); _fadeDurationMs = ms; }
    /**
     * @brief 启动 A->B 的淡入淡出
     */
    void AudioCrossFaderWorker::startFadeAToB() {
        QMutexLocker l(&_mutex);
        _fadeStartMix = _mix;
        _fadeTargetMix = 1.0f;
        qint64 startFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        double frames = _fadeDurationMs / TimestampGenerator::getInstance()->getFrameInterval();
        _fadeStartFrame = startFrame;
        _fadeEndFrame = startFrame + static_cast<qint64>(std::round(frames));
        _fadingActive = true;
        // fixed direction A->B
    }
    /**
     * @brief 启动 B->A 的淡入淡出
     */
    void AudioCrossFaderWorker::startFadeBToA() {
        QMutexLocker l(&_mutex);
        _fadeStartMix = _mix;
        _fadeTargetMix = 0.0f;
        qint64 startFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        double frames = _fadeDurationMs / TimestampGenerator::getInstance()->getFrameInterval();
        _fadeStartFrame = startFrame;
        _fadeEndFrame = startFrame + static_cast<qint64>(std::round(frames));
        _fadingActive = true;
        // fixed direction B->A
    }

}
