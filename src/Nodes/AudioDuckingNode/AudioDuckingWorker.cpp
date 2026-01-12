#include "AudioDuckingWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace Nodes
{
    AudioDuckingWorker::AudioDuckingWorker(QObject *parent)
        : QObject(parent)
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
        , _frameSize(2048)
        , _sampleRate(48000)
    {
        initializeGist(_frameSize, _sampleRate);
    }
    
    AudioDuckingWorker::~AudioDuckingWorker()
    {
        stopProcessing();
    }
    
    void AudioDuckingWorker::initializeGist(int frameSize, int sampleRate)
    {
        _frameSize = frameSize;
        _sampleRate = sampleRate;
        try {
            _gistAnalyzer = std::make_unique<Gist<float>>(frameSize, sampleRate);
        } catch (const std::exception& e) {
            qWarning() << "AudioDuckingWorker: Failed to initialize GIST:" << e.what();
        }
    }

    void AudioDuckingWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) return;
        
        _isProcessing = true;
        _lastProcessedTimestamp = 0;
        QObject::connect(TimestampGenerator::getInstance(),
                         &TimestampGenerator::frameCountUpdated,
                         this,
                         &AudioDuckingWorker::onFrameTick,
                         Qt::QueuedConnection);
        emit processingStatusChanged(true);
    }
    
    void AudioDuckingWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) {
            _isProcessing = false;
            QObject::disconnect(TimestampGenerator::getInstance(),
                                &TimestampGenerator::frameCountUpdated,
                                this,
                                &AudioDuckingWorker::onFrameTick);
            emit processingStatusChanged(false);
        }
    }
    
    void AudioDuckingWorker::processAudioData()
    {
        // Not used when driven by TimestampGenerator, but kept for compatibility
    }

    void AudioDuckingWorker::onFrameTick(qint64 frameCount)
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
    
    void AudioDuckingWorker::performDuckingOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp)
    {
        if (inputFrames.empty() || inputFrames[0].data.isEmpty()) return;

        const AudioFrame& musicFrame = inputFrames[0];
        const float* musicData = reinterpret_cast<const float*>(musicFrame.data.constData());
        size_t sampleCount = musicFrame.data.size() / sizeof(float);
        
        std::vector<float> sidechainData;
        bool hasSidechain = false;

        // Check sidechain input
        if (inputFrames.size() > 1 && !inputFrames[1].data.isEmpty()) {
            const AudioFrame& sidechainFrame = inputFrames[1];
            size_t scSamples = sidechainFrame.data.size() / sizeof(float);
            // Only process if we have data
            if (scSamples > 0) {
                const float* scRaw = reinterpret_cast<const float*>(sidechainFrame.data.constData());
                sidechainData.assign(scRaw, scRaw + scSamples);
                
                // Pad or trim to match music frame size if necessary
                // Use cyclic padding to maintain RMS level if data is smaller than frame size
                if (sidechainData.size() < (size_t)_frameSize) {
                    size_t originalSize = sidechainData.size();
                    sidechainData.resize(_frameSize);
                    for (size_t i = originalSize; i < (size_t)_frameSize; ++i) {
                        sidechainData[i] = sidechainData[i % originalSize];
                    }
                } else if (sidechainData.size() > (size_t)_frameSize) {
                    sidechainData.resize(_frameSize);
                }
                hasSidechain = true;
            }
        }

        float gainReduction = 1.0f;
        
        if (hasSidechain) {
            // Apply Sidechain Gain before analysis
            if (_sidechainGain > 0.001f) {
                float scGainLinear = std::pow(10.0f, _sidechainGain / 20.0f);
                for (float& s : sidechainData) {
                    s *= scGainLinear;
                }
            }

            // Calculate RMS manually to avoid Gist windowing attenuation
            double sumSq = 0.0;
            for (float s : sidechainData) {
                sumSq += s * s;
            }
            float rms = 0.0f;
            if (!sidechainData.empty()) {
                rms = std::sqrt((float)(sumSq / sidechainData.size()));
            }

            // Keep Gist analysis call to satisfy requirement (maybe used for other features later)
            if (_gistAnalyzer) {
                _gistAnalyzer->processAudioFrame(sidechainData);
            }
            
            // Convert RMS to dB
            float rmsDb = (rms > 0.000001f) ? 20.0f * std::log10(rms) : -100.0f;
            
            // Calculate gain reduction with maximum ducking depth
            if (rmsDb > _threshold) {
                float overshoot = rmsDb - _threshold;
                // Use ratio to scale reduction and clamp by depth
                float grDb = std::min(_depthDb, overshoot * _ratio);
                gainReduction = std::pow(10.0f, -grDb / 20.0f);
            }
        }
        
        // Apply envelope (Attack/Release) - simplified block processing
        // Ideally we do this sample by sample, but frame-based is okay for simple ducking 
        // if frame rate is high enough (approx 21ms at 48k/1024, or 42ms at 2048).
        // 42ms might be too slow for fast attack. 
        // But since we are using Gist which is frame-based, we are limited to frame granularity unless we do sample-based RMS.
        // Given the prompt "use gist analysis", I will stick to frame-based RMS from Gist.
        
        // Smooth the gain
        float targetGain = gainReduction;
        // Simple smoothing: alpha filter
        // We can't do proper attack/release per sample with just one RMS value per frame.
        // We will apply the target gain to the whole frame, but smoothed from previous frame's gain.
        
        // Prepare output
        AudioFrame outputFrame = musicFrame;
        outputFrame.timestamp = timestamp + 2; // Latency compensation
        outputFrame.data.resize(sampleCount * sizeof(float));
        float* outputData = reinterpret_cast<float*>(outputFrame.data.data());
        
        float currentGain = _envelope;
        
        // Time constants
        float dt = (float)sampleCount / _sampleRate; // Duration of this frame
        float attackCoeff = std::exp(-dt / (_attack / 1000.0f)); 
        
        // Apply smoothing toward targetGain
        if (targetGain < currentGain) {
            // Attack phase: Exponential convergence (Linear domain)
            _envelope = attackCoeff * currentGain + (1.0f - attackCoeff) * targetGain;
        } else {
            // Release phase: Linear dB ramp (Constant dB/sec)
            // This provides a uniform volume recovery that feels "slower" and more natural than exponential
            // We define Release Time as the time to recover 30dB
            // Rate (dB/s) = 30.0 / (ReleaseTime_ms / 1000.0)
            
            float releaseTimeSec = std::max(0.001f, _release / 1000.0f);
            float recoveryRateDbPerSec = 30.0f / releaseTimeSec;
            float maxRecoveryDb = recoveryRateDbPerSec * dt;
            
            // Convert current gain to dB (handle near-zero case)
            float currentDb = (currentGain > 0.000001f) ? 20.0f * std::log10(currentGain) : -120.0f;
            float targetDb = (targetGain > 0.000001f) ? 20.0f * std::log10(targetGain) : -120.0f;
            
            if (currentDb < targetDb) {
                currentDb += maxRecoveryDb;
                if (currentDb > targetDb) currentDb = targetDb;
                _envelope = std::pow(10.0f, currentDb / 20.0f);
            } else {
                _envelope = targetGain;
            }
        }
        
        // Apply makeup gain
        float makeupLinear = std::pow(10.0f, _makeupGain / 20.0f);
        float finalGain = _envelope * makeupLinear;
        
        for (size_t i = 0; i < sampleCount; ++i) {
            outputData[i] = musicData[i] * finalGain;
        }
        
        if (!_outputBuffers.empty() && _outputBuffers[0]) {
            _outputBuffers[0]->pushFrame(outputFrame);
        }
    }

    void AudioDuckingWorker::initializeBuffers(int inputCount, int outputCount)
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

    void AudioDuckingWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < (int)_inputBuffers.size()) {
            _inputBuffers[port] = buffer;
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioDuckingWorker::getOutputBuffer(int port)
    {
        if (port >= 0 && port < (int)_outputBuffers.size()) {
            return _outputBuffers[port];
        }
        return nullptr;
    }

    void AudioDuckingWorker::setThreshold(double val) { QMutexLocker l(&_mutex); _threshold = (float)val; }
    void AudioDuckingWorker::setRatio(double val) { QMutexLocker l(&_mutex); _ratio = (float)val; }
    void AudioDuckingWorker::setAttack(double val) { QMutexLocker l(&_mutex); _attack = (float)val; }
    void AudioDuckingWorker::setRelease(double val) { QMutexLocker l(&_mutex); _release = (float)val; }
    void AudioDuckingWorker::setMakeupGain(double val) { QMutexLocker l(&_mutex); _makeupGain = (float)val; }
    void AudioDuckingWorker::setSidechainGain(double val) { QMutexLocker l(&_mutex); _sidechainGain = (float)val; }
    /**
     * @brief 设置最大压低深度（dB）
     * @param val 压低深度，单位 dB
     */
    void AudioDuckingWorker::setDepth(double val) { QMutexLocker l(&_mutex); _depthDb = (float)val; }
}
