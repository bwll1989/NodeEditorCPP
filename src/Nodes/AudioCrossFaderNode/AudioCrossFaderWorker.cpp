#include "AudioCrossFaderWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

namespace Nodes
{
    AudioCrossFaderWorker::AudioCrossFaderWorker(QObject *parent)
        : QObject(parent)
    {
    }

    AudioCrossFaderWorker::~AudioCrossFaderWorker()
    {
        stopProcessing();
    }

    void AudioCrossFaderWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedTimestamp = 0;
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        TimestampGenerator::getInstance()->registerAudioTickWaiter(&_tickWaiter);
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioCrossFaderWorker::stopProcessing()
    {
        _stopRequested.store(true, std::memory_order_release);
        _tickWaiter.requestStop();
        TimestampGenerator::getInstance()->unregisterAudioTickWaiter(&_tickWaiter);

        if (_audioThread.joinable()) {
            if (_audioThread.get_id() != std::this_thread::get_id()) {
                _audioThread.join();
            } else {
                _audioThread.detach();
            }
        }

        QMutexLocker locker(&_mutex);
        if (!_isProcessing) {
            return;
        }
        _isProcessing = false;
        emit processingStatusChanged(false);
    }

    void AudioCrossFaderWorker::audioLoop()
    {
        while (!_stopRequested.load(std::memory_order_acquire)
               && !_tickWaiter.isStopRequested()) {
            if (!_tickWaiter.wait(50)) {
                continue;
            }
            if (_stopRequested.load(std::memory_order_acquire)
                || _tickWaiter.isStopRequested()) {
                break;
            }
            processCurrentFrame();
        }
    }

    void AudioCrossFaderWorker::processAudioData()
    {
        // Not used when driven by TimestampGenerator, kept for compatibility
    }

    void AudioCrossFaderWorker::processCurrentFrame()
    {
        const qint64 currentFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        int channelCount = 1;
        float mix = 0.5f;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            if (currentFrame == _lastProcessedTimestamp) {
                return;
            }

            // 自动淡入淡出：根据时间推进 mix
            if (_fadingActive) {
                if (currentFrame >= _fadeEndFrame) {
                    _mix = _fadeTargetMix;
                    _fadingActive = false;
                } else if (currentFrame >= _fadeStartFrame) {
                    const qint64 span = std::max<qint64>(1, _fadeEndFrame - _fadeStartFrame);
                    const double progress = double(currentFrame - _fadeStartFrame) / double(span);
                    const float newMix = _fadeStartMix
                        + static_cast<float>(progress) * (_fadeTargetMix - _fadeStartMix);
                    _mix = std::clamp(newMix, 0.0f, 1.0f);
                }
            }

            inputs = _inputBuffers;
            outputs = _outputBuffers;
            channelCount = _channelCount;
            mix = _mix;
        }

        if (channelCount <= 0 || static_cast<int>(inputs.size()) < channelCount * 2) {
            return;
        }

        std::vector<AudioFrame> inputFrames(inputs.size());
        bool hasAny = false;
        for (size_t i = 0; i < inputs.size(); ++i) {
            if (inputs[i] && inputs[i]->isActive()
                && inputs[i]->getFrameByTimestamp(currentFrame, inputFrames[i])) {
                hasAny = true;
            }
        }
        if (!hasAny) {
            return;
        }

        performCrossFadeOperation(inputFrames, outputs, currentFrame, channelCount, mix);

        {
            QMutexLocker locker(&_mutex);
            _lastProcessedTimestamp = currentFrame;
        }
    }

    AudioCrossFaderWorker::ChannelView AudioCrossFaderWorker::viewOf(const AudioFrame &frame)
    {
        ChannelView view;
        if (frame.data.isEmpty() || frame.channels <= 0) {
            return view;
        }
        if (frame.bitsPerSample != 32) {
            return view;
        }
        const int total = frame.data.size() / static_cast<int>(sizeof(float));
        const int frames = total / frame.channels;
        if (frames <= 0) {
            return view;
        }
        view.data = reinterpret_cast<const float *>(frame.data.constData());
        view.channels = frame.channels;
        view.frames = frames;
        view.sampleRate = frame.sampleRate > 0 ? frame.sampleRate : 48000;
        view.valid = true;
        return view;
    }

    void AudioCrossFaderWorker::performCrossFadeOperation(
        const std::vector<AudioFrame> &inputFrames,
        const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
        qint64 timestamp,
        int channelCount,
        float mix)
    {
        if (channelCount <= 0 || outputs.empty()) {
            return;
        }

        // equal-power 曲线
        const float theta = mix * static_cast<float>(M_PI_2);
        const float wABase = std::cos(theta);
        const float wBBase = std::sin(theta);

        const int outCount = std::min(channelCount, static_cast<int>(outputs.size()));
        for (int ch = 0; ch < outCount; ++ch) {
            if (!outputs[static_cast<size_t>(ch)]) {
                continue;
            }

            const int aIndex = ch;
            const int bIndex = channelCount + ch;
            ChannelView viewA;
            ChannelView viewB;
            if (aIndex < static_cast<int>(inputFrames.size())) {
                viewA = viewOf(inputFrames[static_cast<size_t>(aIndex)]);
            }
            if (bIndex < static_cast<int>(inputFrames.size())) {
                viewB = viewOf(inputFrames[static_cast<size_t>(bIndex)]);
            }

            if (!viewA.valid && !viewB.valid) {
                continue;
            }

            float wA = wABase;
            float wB = wBBase;
            if (!viewA.valid && viewB.valid) {
                wA = 0.0f;
                wB = 1.0f;
            } else if (viewA.valid && !viewB.valid) {
                wA = 1.0f;
                wB = 0.0f;
            }

            int channels = 1;
            int frames = 0;
            int sampleRate = _sampleRate;
            if (viewA.valid) {
                channels = viewA.channels;
                frames = viewA.frames;
                sampleRate = viewA.sampleRate;
            }
            if (viewB.valid) {
                channels = std::max(channels, viewB.channels);
                frames = std::max(frames, viewB.frames);
                if (!viewA.valid) {
                    sampleRate = viewB.sampleRate;
                }
            }
            if (channels <= 0) {
                channels = 1;
            }
            if (frames <= 0) {
                continue;
            }

            std::vector<float> mixed(static_cast<size_t>(frames * channels), 0.0f);
            for (int i = 0; i < frames; ++i) {
                for (int c = 0; c < channels; ++c) {
                    float a = 0.0f;
                    float b = 0.0f;
                    if (viewA.valid && c < viewA.channels && i < viewA.frames) {
                        a = viewA.data[i * viewA.channels + c];
                    }
                    if (viewB.valid && c < viewB.channels && i < viewB.frames) {
                        b = viewB.data[i * viewB.channels + c];
                    }
                    mixed[static_cast<size_t>(i * channels + c)] = a * wA + b * wB;
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = timestamp + TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(mixed.data()),
                                          static_cast<int>(mixed.size() * sizeof(float)));
            outputs[static_cast<size_t>(ch)]->pushFrame(outputFrame);
        }
    }

    void AudioCrossFaderWorker::initializeBuffers(int channelCount)
    {
        QMutexLocker locker(&_mutex);
        channelCount = qMax(1, channelCount);
        _channelCount = channelCount;

        const int inputCount = channelCount * 2;
        _inputBuffers.resize(static_cast<size_t>(inputCount));

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(channelCount));
        for (int i = 0; i < channelCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }
    }

    void AudioCrossFaderWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_inputBuffers.size())) {
            _inputBuffers[static_cast<size_t>(port)] = buffer;
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioCrossFaderWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioCrossFaderWorker::setMix(double val)
    {
        QMutexLocker locker(&_mutex);
        _mix = std::clamp(static_cast<float>(val), 0.0f, 1.0f);
        _fadingActive = false;
    }

    void AudioCrossFaderWorker::setFadeDuration(double ms)
    {
        QMutexLocker locker(&_mutex);
        _fadeDurationMs = ms;
    }

    void AudioCrossFaderWorker::startFadeAToB()
    {
        QMutexLocker locker(&_mutex);
        _fadeStartMix = _mix;
        _fadeTargetMix = 1.0f;
        const qint64 startFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        const double frames = _fadeDurationMs / TimestampGenerator::getInstance()->getFrameInterval();
        _fadeStartFrame = startFrame;
        _fadeEndFrame = startFrame + static_cast<qint64>(std::round(frames));
        _fadingActive = true;
    }

    void AudioCrossFaderWorker::startFadeBToA()
    {
        QMutexLocker locker(&_mutex);
        _fadeStartMix = _mix;
        _fadeTargetMix = 0.0f;
        const qint64 startFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        const double frames = _fadeDurationMs / TimestampGenerator::getInstance()->getFrameInterval();
        _fadeStartFrame = startFrame;
        _fadeEndFrame = startFrame + static_cast<qint64>(std::round(frames));
        _fadingActive = true;
    }
}
