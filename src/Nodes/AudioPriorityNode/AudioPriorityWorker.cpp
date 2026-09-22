#include "AudioPriorityWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace Nodes
{
    AudioPriorityWorker::AudioPriorityWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioPriorityWorker::~AudioPriorityWorker()
    {
        stopProcessing();
    }

    void AudioPriorityWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedTimestamp = 0;
            _gainDb = 0.0f;
            _holdLeftSec = 0.0f;
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        TimestampGenerator::getInstance()->registerAudioTickWaiter(&_tickWaiter);
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioPriorityWorker::stopProcessing()
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

    void AudioPriorityWorker::audioLoop()
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

    void AudioPriorityWorker::processCurrentFrame()
    {
        const qint64 currentFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        Params params;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            if (currentFrame == _lastProcessedTimestamp) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            params = _params;
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

        performDucking(inputFrames, outputs, currentFrame, params);

        QMutexLocker locker(&_mutex);
        _lastProcessedTimestamp = currentFrame;
    }

    AudioPriorityWorker::ChannelView AudioPriorityWorker::viewOf(const AudioFrame &frame)
    {
        ChannelView view;
        if (frame.data.isEmpty() || frame.channels <= 0) {
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

    float AudioPriorityWorker::readSample(const ChannelView &view, int frameIndex, int channel, int outChannels)
    {
        if (!view.valid || frameIndex < 0 || frameIndex >= view.frames || outChannels <= 0) {
            return 0.0f;
        }
        if (view.channels == 1) {
            return view.data[frameIndex];
        }
        if (view.channels == outChannels) {
            return view.data[frameIndex * view.channels + channel];
        }
        if (outChannels == 1) {
            float sum = 0.0f;
            for (int c = 0; c < view.channels; ++c) {
                sum += view.data[frameIndex * view.channels + c];
            }
            return sum / static_cast<float>(view.channels);
        }
        if (channel >= 0 && channel < view.channels) {
            return view.data[frameIndex * view.channels + channel];
        }
        return 0.0f;
    }

    float AudioPriorityWorker::onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec)
    {
        if (tauSec <= 0.0001f) {
            return targetDb;
        }
        const float alpha = 1.0f - std::exp(-dtSec / tauSec);
        return currentDb + (targetDb - currentDb) * alpha;
    }

    void AudioPriorityWorker::performDucking(const std::vector<AudioFrame> &inputFrames,
                                             const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                             qint64 timestamp,
                                             const Params &params)
    {
        if (inputFrames.empty() || outputs.empty()) {
            return;
        }

        const int priorityIndex = static_cast<int>(inputFrames.size()) - 1;
        const ChannelView priority = viewOf(inputFrames[static_cast<size_t>(priorityIndex)]);

        float rmsDb = -100.0f;
        int envFrames = 0;
        int sampleRate = 48000;
        if (priority.valid) {
            double sumSq = 0.0;
            const int total = priority.frames * priority.channels;
            for (int i = 0; i < total; ++i) {
                const float s = priority.data[i];
                sumSq += static_cast<double>(s) * static_cast<double>(s);
            }
            const float rms = (total > 0) ? std::sqrt(static_cast<float>(sumSq / total)) : 0.0f;
            rmsDb = (rms > 1.0e-7f) ? 20.0f * std::log10(rms) : -100.0f;
            envFrames = priority.frames;
            sampleRate = priority.sampleRate;
        }

        if (envFrames <= 0) {
            for (int i = 0; i < priorityIndex; ++i) {
                const ChannelView program = viewOf(inputFrames[static_cast<size_t>(i)]);
                if (program.valid) {
                    envFrames = program.frames;
                    sampleRate = program.sampleRate;
                    break;
                }
            }
        }
        if (envFrames <= 0) {
            envFrames = qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate));
        }

        const float dt = static_cast<float>(envFrames) / static_cast<float>(qMax(1, sampleRate));
        const float gainStartDb = _gainDb;
        const bool above = priority.valid && rmsDb >= params.thresholdDb;
        if (above) {
            _holdLeftSec = params.holdSec;
            _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
        } else if (_holdLeftSec > 0.0f) {
            // 低于阈值后继续停在 Depth，避免语句停顿把节目顶回来
            _holdLeftSec = std::max(0.0f, _holdLeftSec - dt);
            _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
        } else {
            _gainDb = onePoleDb(_gainDb, 0.0f, dt, params.releaseSec);
        }
        const float gainEndDb = _gainDb;
        const float priorityLinear = std::pow(10.0f, params.priorityGainDb / 20.0f);

        const int outCount = static_cast<int>(outputs.size());
        for (int outIndex = 0; outIndex < outCount; ++outIndex) {
            if (!outputs[static_cast<size_t>(outIndex)]) {
                continue;
            }

            const bool priorityOnly = (outIndex >= priorityIndex);

            ChannelView program;
            if (!priorityOnly && outIndex < static_cast<int>(inputFrames.size())) {
                program = viewOf(inputFrames[static_cast<size_t>(outIndex)]);
            }

            const ChannelView &layout = program.valid ? program : priority;
            int channels = layout.valid ? layout.channels : 1;
            int frames = layout.valid ? layout.frames : envFrames;
            if (program.valid && priority.valid) {
                frames = std::max(program.frames, priority.frames);
            }
            if (channels <= 0) {
                channels = 1;
            }
            if (frames <= 0) {
                frames = envFrames;
            }

            std::vector<float> mixed(static_cast<size_t>(frames * channels), 0.0f);
            const int denom = qMax(1, frames - 1);
            for (int i = 0; i < frames; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(denom);
                const float gainDb = gainStartDb + (gainEndDb - gainStartDb) * t;
                const float duck = std::pow(10.0f, gainDb / 20.0f);
                for (int c = 0; c < channels; ++c) {
                    float sample = 0.0f;
                    if (!priorityOnly) {
                        sample += readSample(program, i, c, channels) * duck;
                        sample += readSample(priority, i, c, channels) * priorityLinear;
                    } else {
                        sample = readSample(priority, i, c, channels) * priorityLinear;
                    }
                    mixed[static_cast<size_t>(i * channels + c)] = sample;
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = layout.valid ? layout.sampleRate : sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = timestamp + TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(mixed.data()),
                                          static_cast<int>(mixed.size() * sizeof(float)));
            outputs[static_cast<size_t>(outIndex)]->pushFrame(outputFrame);
        }
    }

    void AudioPriorityWorker::initializeBuffers(int inputCount, int outputCount)
    {
        QMutexLocker locker(&_mutex);
        inputCount = qMax(1, inputCount);
        outputCount = qMax(1, outputCount);

        _inputBuffers.resize(static_cast<size_t>(inputCount));

        if (static_cast<int>(_outputBuffers.size()) == outputCount) {
            return;
        }

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(outputCount));
        for (int i = 0; i < outputCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }
    }

    void AudioPriorityWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_inputBuffers.size())) {
            _inputBuffers[static_cast<size_t>(port)] = buffer;
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioPriorityWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioPriorityWorker::setThreshold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.thresholdDb = static_cast<float>(val);
    }

    void AudioPriorityWorker::setDepth(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.depthDb = static_cast<float>(qMax(0.0, val));
    }

    void AudioPriorityWorker::setPriorityGain(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.priorityGainDb = static_cast<float>(val);
    }

    void AudioPriorityWorker::setAttack(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.attackSec = static_cast<float>(qMax(0.0005, val / 1000.0));
    }

    void AudioPriorityWorker::setHold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.holdSec = static_cast<float>(qMax(0.0, val / 1000.0));
    }

    void AudioPriorityWorker::setRelease(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.releaseSec = static_cast<float>(qMax(0.001, val / 1000.0));
    }
}
