#include "AudioGateWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace Nodes
{
    AudioGateWorker::AudioGateWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioGateWorker::~AudioGateWorker()
    {
        stopProcessing();
    }

    void AudioGateWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedTimestamp = 0;
            for (auto &s : _states) {
                s.gainDb = -_params.depthDb;
                s.holdLeftSec = 0.0f;
            }
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        TimestampGenerator::getInstance()->registerAudioTickWaiter(&_tickWaiter);
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioGateWorker::stopProcessing()
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

    void AudioGateWorker::audioLoop()
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

    void AudioGateWorker::processCurrentFrame()
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

        performGate(inputFrames, outputs, currentFrame, params);

        QMutexLocker locker(&_mutex);
        _lastProcessedTimestamp = currentFrame;
    }

    AudioGateWorker::ChannelView AudioGateWorker::viewOf(const AudioFrame &frame)
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

    float AudioGateWorker::rmsDbOf(const ChannelView &view)
    {
        if (!view.valid) {
            return -100.0f;
        }
        double sumSq = 0.0;
        const int total = view.frames * view.channels;
        for (int i = 0; i < total; ++i) {
            const float s = view.data[i];
            sumSq += static_cast<double>(s) * static_cast<double>(s);
        }
        const float rms = (total > 0) ? std::sqrt(static_cast<float>(sumSq / total)) : 0.0f;
        return (rms > 1.0e-7f) ? 20.0f * std::log10(rms) : -100.0f;
    }

    float AudioGateWorker::onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec)
    {
        if (tauSec <= 0.0001f) {
            return targetDb;
        }
        const float alpha = 1.0f - std::exp(-dtSec / tauSec);
        return currentDb + (targetDb - currentDb) * alpha;
    }

    void AudioGateWorker::performGate(const std::vector<AudioFrame> &inputFrames,
                                      const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                      qint64 timestamp,
                                      const Params &params)
    {
        if (inputFrames.empty() || outputs.empty()) {
            return;
        }

        const int count = static_cast<int>(std::min(inputFrames.size(), outputs.size()));
        if (static_cast<int>(_states.size()) < count) {
            const size_t old = _states.size();
            _states.resize(static_cast<size_t>(count));
            for (size_t i = old; i < _states.size(); ++i) {
                _states[i].gainDb = -params.depthDb;
                _states[i].holdLeftSec = 0.0f;
            }
        }

        for (int ch = 0; ch < count; ++ch) {
            if (!outputs[static_cast<size_t>(ch)]) {
                continue;
            }

            const ChannelView view = viewOf(inputFrames[static_cast<size_t>(ch)]);
            int sampleRate = view.valid ? view.sampleRate : 48000;
            int frames = view.valid ? view.frames
                                    : qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate));
            int channels = view.valid ? view.channels : 1;
            if (channels <= 0) {
                channels = 1;
            }
            if (frames <= 0) {
                frames = 1;
            }

            const float dt = static_cast<float>(frames) / static_cast<float>(qMax(1, sampleRate));
            const float rmsDb = rmsDbOf(view);
            const bool above = view.valid && rmsDb >= params.thresholdDb;

            ChannelState &state = _states[static_cast<size_t>(ch)];
            const float gainStartDb = state.gainDb;

            // 开闸：目标 0 dB（无损）；关闸：目标 -Depth
            if (above) {
                state.holdLeftSec = params.holdSec;
                state.gainDb = onePoleDb(state.gainDb, 0.0f, dt, params.attackSec);
            } else if (state.holdLeftSec > 0.0f) {
                state.holdLeftSec = std::max(0.0f, state.holdLeftSec - dt);
                state.gainDb = onePoleDb(state.gainDb, 0.0f, dt, params.attackSec);
            } else {
                state.gainDb = onePoleDb(state.gainDb, -params.depthDb, dt, params.releaseSec);
            }
            const float gainEndDb = state.gainDb;

            std::vector<float> out(static_cast<size_t>(frames * channels), 0.0f);
            if (view.valid) {
                const int denom = qMax(1, frames - 1);
                const int total = frames * channels;
                const int srcTotal = view.frames * view.channels;
                for (int i = 0; i < frames; ++i) {
                    const float t = static_cast<float>(i) / static_cast<float>(denom);
                    const float gainDb = gainStartDb + (gainEndDb - gainStartDb) * t;
                    const float gain = std::pow(10.0f, gainDb / 20.0f);
                    for (int c = 0; c < channels; ++c) {
                        const int idx = i * channels + c;
                        if (idx < total && idx < srcTotal) {
                            out[static_cast<size_t>(idx)] = view.data[idx] * gain;
                        }
                    }
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = timestamp + TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(out.data()),
                                          static_cast<int>(out.size() * sizeof(float)));
            outputs[static_cast<size_t>(ch)]->pushFrame(outputFrame);
        }
    }

    void AudioGateWorker::initializeBuffers(int channelCount)
    {
        QMutexLocker locker(&_mutex);
        channelCount = qMax(1, channelCount);

        _inputBuffers.resize(static_cast<size_t>(channelCount));

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(channelCount));
        for (int i = 0; i < channelCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }

        const size_t oldStateCount = _states.size();
        _states.resize(static_cast<size_t>(channelCount));
        for (size_t i = oldStateCount; i < _states.size(); ++i) {
            _states[i].gainDb = -_params.depthDb;
            _states[i].holdLeftSec = 0.0f;
        }
    }

    void AudioGateWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_inputBuffers.size())) {
            _inputBuffers[static_cast<size_t>(port)] = buffer;
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioGateWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioGateWorker::setThreshold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.thresholdDb = static_cast<float>(val);
    }

    void AudioGateWorker::setDepth(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.depthDb = static_cast<float>(qBound(0.0, val, 60.0));
    }

    void AudioGateWorker::setAttack(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.attackSec = static_cast<float>(qMax(0.0001, val / 1000.0));
    }

    void AudioGateWorker::setHold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.holdSec = static_cast<float>(qMax(0.0, val / 1000.0));
    }

    void AudioGateWorker::setRelease(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.releaseSec = static_cast<float>(qMax(0.001, val / 1000.0));
    }
}
