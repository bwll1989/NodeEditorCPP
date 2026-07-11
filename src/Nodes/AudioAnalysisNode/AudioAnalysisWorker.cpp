#include "AudioAnalysisWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace Nodes
{
    AudioAnalysisWorker::AudioAnalysisWorker(QObject *parent)
        : QObject(parent)
    {
        initializeGist(_params.frameSize, _sampleRate);
    }

    AudioAnalysisWorker::~AudioAnalysisWorker()
    {
        stopProcessing();
    }

    void AudioAnalysisWorker::initializeGist(int frameSize, int sampleRate)
    {
        try {
            _gistAnalyzer = std::make_unique<Gist<float>>(frameSize, sampleRate);
            qDebug() << "GIST analyzer initialized with frame size:" << frameSize
                     << "sample rate:" << sampleRate;
        } catch (const std::exception &e) {
            qWarning() << "Failed to initialize GIST analyzer:" << e.what();
        }
    }

    void AudioAnalysisWorker::setAnalysisParams(float lowMinHz, float lowMaxHz,
                                                float midMinHz, float midMaxHz,
                                                float highMinHz, float highMaxHz,
                                                int frameSize, int beatIntervalMs,
                                                float bandAttackMs, float bandReleaseMs)
    {
        QMutexLocker locker(&_mutex);

        const int normalizedFrameSize = std::max(256, frameSize);
        const bool frameSizeChanged = normalizedFrameSize != _params.frameSize;

        _params.lowMinHz = std::min(lowMinHz, lowMaxHz);
        _params.lowMaxHz = std::max(lowMinHz, lowMaxHz);
        _params.midMinHz = std::min(midMinHz, midMaxHz);
        _params.midMaxHz = std::max(midMinHz, midMaxHz);
        _params.highMinHz = std::min(highMinHz, highMaxHz);
        _params.highMaxHz = std::max(highMinHz, highMaxHz);
        _params.frameSize = normalizedFrameSize;
        _params.beatIntervalMs = std::max(50, beatIntervalMs);
        _params.bandAttackMs = std::max(1.0f, bandAttackMs);
        _params.bandReleaseMs = std::max(1.0f, bandReleaseMs);

        if (frameSizeChanged) {
            initializeGist(_params.frameSize, _sampleRate);
            _sampleAccumulator.clear();
            _lowSmoothed = 0.0f;
            _midSmoothed = 0.0f;
            _highSmoothed = 0.0f;
            _fluxAverage = 0.0f;
            _lastBeatTimestamp = 0;
        }
    }

    void AudioAnalysisWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (_isProcessing) {
            return;
        }

        _isProcessing = true;
        _lastProcessedTimestamp = 0;
        _sampleAccumulator.clear();
        _lowSmoothed = 0.0f;
        _midSmoothed = 0.0f;
        _highSmoothed = 0.0f;
        _fluxAverage = 0.0f;
        _lastBeatTimestamp = 0;

        QObject::connect(TimestampGenerator::getInstance(),
                         &TimestampGenerator::frameCountUpdated,
                         this,
                         &AudioAnalysisWorker::onFrameTick,
                         Qt::QueuedConnection);
        emit processingStatusChanged(true);
    }

    void AudioAnalysisWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        if (!_isProcessing) {
            return;
        }

        _isProcessing = false;
        QObject::disconnect(TimestampGenerator::getInstance(),
                            &TimestampGenerator::frameCountUpdated,
                            this,
                            &AudioAnalysisWorker::onFrameTick);
        emit processingStatusChanged(false);
    }

    void AudioAnalysisWorker::onFrameTick(qint64 frameCount)
    {
        if (!_isProcessing || !_inputBuffers) {
            return;
        }
        if (frameCount == _lastProcessedTimestamp) {
            return;
        }

        AudioFrame inputFrame;
        if (!_inputBuffers->isActive()
            || !_inputBuffers->getFrameByTimestamp(frameCount, inputFrame)
            || inputFrame.data.isEmpty()) {
            return;
        }

        performAnalysisOperation(inputFrame);
        _lastProcessedTimestamp = frameCount;
    }

    std::vector<float> AudioAnalysisWorker::extractMonoSamples(const AudioFrame &frame) const
    {
        std::vector<float> audioBuffer;
        const float *samples = reinterpret_cast<const float *>(frame.data.constData());
        const int totalSamples = frame.data.size() / static_cast<int>(sizeof(float));
        if (totalSamples <= 0 || frame.channels <= 0) {
            return audioBuffer;
        }

        const int samplesPerChannel = totalSamples / frame.channels;
        audioBuffer.reserve(static_cast<size_t>(samplesPerChannel));

        if (frame.channels == 1) {
            audioBuffer.assign(samples, samples + samplesPerChannel);
        } else if (frame.channels == 2) {
            for (int i = 0; i < samplesPerChannel; ++i) {
                audioBuffer.push_back((samples[i * 2] + samples[i * 2 + 1]) * 0.5f);
            }
        } else {
            for (int i = 0; i < samplesPerChannel; ++i) {
                audioBuffer.push_back(samples[i * frame.channels]);
            }
        }

        return audioBuffer;
    }

    void AudioAnalysisWorker::appendSamples(const std::vector<float> &samples)
    {
        _sampleAccumulator.insert(_sampleAccumulator.end(), samples.begin(), samples.end());
        const size_t maxBufferedSamples = static_cast<size_t>(_params.frameSize) * 4;
        if (_sampleAccumulator.size() > maxBufferedSamples) {
            _sampleAccumulator.erase(
                _sampleAccumulator.begin(),
                _sampleAccumulator.begin() + static_cast<std::ptrdiff_t>(_sampleAccumulator.size() - maxBufferedSamples));
        }
    }

    bool AudioAnalysisWorker::consumeAnalysisFrame(std::vector<float> &frameOut, int frameSize)
    {
        if (_sampleAccumulator.size() < static_cast<size_t>(frameSize)) {
            return false;
        }

        frameOut.assign(_sampleAccumulator.begin(), _sampleAccumulator.begin() + frameSize);
        _sampleAccumulator.erase(_sampleAccumulator.begin(), _sampleAccumulator.begin() + frameSize);
        return true;
    }

    float AudioAnalysisWorker::computeBandEnergy(const std::vector<float> &magnitudeSpectrum,
                                                 float lowHz, float highHz, int sampleRate) const
    {
        if (magnitudeSpectrum.empty() || sampleRate <= 0) {
            return 0.0f;
        }

        const float nyquistFreq = static_cast<float>(sampleRate) * 0.5f;
        const int spectrumSize = static_cast<int>(magnitudeSpectrum.size());
        const float freqResolution = nyquistFreq / std::max(1, spectrumSize - 1);

        const int startIndex = std::clamp(
            static_cast<int>(std::floor(lowHz / freqResolution)), 0, spectrumSize - 1);
        const int endIndex = std::clamp(
            static_cast<int>(std::ceil(highHz / freqResolution)), startIndex, spectrumSize - 1);

        double energy = 0.0;
        for (int i = startIndex; i <= endIndex; ++i) {
            const float magnitude = magnitudeSpectrum[static_cast<size_t>(i)];
            energy += static_cast<double>(magnitude) * static_cast<double>(magnitude);
        }
        return static_cast<float>(energy);
    }

    float AudioAnalysisWorker::smoothBand(float target, float current, float dtSeconds,
                                          float attackMs, float releaseMs) const
    {
        const float attackSec = std::max(0.001f, attackMs / 1000.0f);
        const float releaseSec = std::max(0.001f, releaseMs / 1000.0f);
        const float coeff = (target > current)
            ? std::exp(-dtSeconds / attackSec)
            : std::exp(-dtSeconds / releaseSec);

        return std::clamp(coeff * current + (1.0f - coeff) * target, 0.0f, 1.0f);
    }

    bool AudioAnalysisWorker::detectBeat(float spectralFlux, qint64 timestampMs,
                                         const AudioAnalysisParams &params)
    {
        _fluxAverage = params.beatFluxSmoothing * _fluxAverage
            + (1.0f - params.beatFluxSmoothing) * spectralFlux;
        const float threshold = std::max(0.0005f, _fluxAverage * params.beatFluxMultiplier);

        if (spectralFlux <= threshold) {
            return false;
        }
        if (_lastBeatTimestamp > 0 && (timestampMs - _lastBeatTimestamp) < params.beatIntervalMs) {
            return false;
        }

        _lastBeatTimestamp = timestampMs;
        return true;
    }

    void AudioAnalysisWorker::performAnalysisOperation(const AudioFrame &inputFrame)
    {
        AudioAnalysisParams params;
        {
            QMutexLocker locker(&_mutex);
            if (!_gistAnalyzer || inputFrame.data.isEmpty()) {
                return;
            }
            params = _params;
        }

        if (inputFrame.sampleRate > 0 && inputFrame.sampleRate != _sampleRate) {
            QMutexLocker locker(&_mutex);
            _sampleRate = inputFrame.sampleRate;
            initializeGist(_params.frameSize, _sampleRate);
            _sampleAccumulator.clear();
        }

        const std::vector<float> monoSamples = extractMonoSamples(inputFrame);
        if (monoSamples.empty()) {
            return;
        }

        appendSamples(monoSamples);

        std::vector<float> analysisFrame;
        if (!consumeAnalysisFrame(analysisFrame, params.frameSize)) {
            return;
        }

        const float dtSeconds = static_cast<float>(params.frameSize) / static_cast<float>(_sampleRate);

        try {
            _gistAnalyzer->processAudioFrame(analysisFrame);

            const float rmsValue = _gistAnalyzer->rootMeanSquare();
            const float spectralFlux = _gistAnalyzer->spectralDifference();
            const std::vector<float> &magnitudeSpectrum = _gistAnalyzer->getMagnitudeSpectrum();

            const float lowEnergy = computeBandEnergy(
                magnitudeSpectrum, params.lowMinHz, params.lowMaxHz, _sampleRate);
            const float midEnergy = computeBandEnergy(
                magnitudeSpectrum, params.midMinHz, params.midMaxHz, _sampleRate);
            const float highEnergy = computeBandEnergy(
                magnitudeSpectrum, params.highMinHz, params.highMaxHz, _sampleRate);
            const float totalEnergy = lowEnergy + midEnergy + highEnergy;

            float low = 0.0f;
            float mid = 0.0f;
            float high = 0.0f;
            if (totalEnergy > 0.0f) {
                low = lowEnergy / totalEnergy;
                mid = midEnergy / totalEnergy;
                high = highEnergy / totalEnergy;
            }

            _lowSmoothed = smoothBand(low, _lowSmoothed, dtSeconds, params.bandAttackMs, params.bandReleaseMs);
            _midSmoothed = smoothBand(mid, _midSmoothed, dtSeconds, params.bandAttackMs, params.bandReleaseMs);
            _highSmoothed = smoothBand(high, _highSmoothed, dtSeconds, params.bandAttackMs, params.bandReleaseMs);

            const float level = std::clamp(rmsValue, 0.0f, 1.0f);
            const qint64 nowMs = TimestampGenerator::getInstance()->getCurrentFrameInfo().absoluteTimeMs;
            const bool beat = detectBeat(spectralFlux, nowMs, params);

            emit analysisOutputsChanged(static_cast<double>(_lowSmoothed),
                                         static_cast<double>(_midSmoothed),
                                         static_cast<double>(_highSmoothed),
                                         static_cast<double>(level),
                                         beat);
        } catch (const std::exception &e) {
            qWarning() << "GIST analysis error:" << e.what();
        }
    }

    void AudioAnalysisWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);

        if (port >= 0) {
            _inputBuffers = buffer;
        } else {
            qWarning() << "AudioAnalysisWorker: Invalid input port index:" << port;
        }
    }
}
