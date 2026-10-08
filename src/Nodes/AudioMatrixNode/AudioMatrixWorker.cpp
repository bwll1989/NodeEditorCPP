#include "AudioMatrixWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <utility>

namespace Nodes
{
    namespace
    {
        constexpr double kMuteFloorLinear = 1e-3; // 10^(-60/20)
    }

    AudioMatrixWorker::AudioMatrixWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioMatrixWorker::~AudioMatrixWorker()
    {
        stopProcessing();
    }

    void AudioMatrixWorker::unregisterAllInputWaitersLocked()
    {
        for (auto &buf : _inputBuffers) {
            if (buf) {
                buf->unregisterFrameWaiter(&_tickWaiter);
            }
        }
    }

    void AudioMatrixWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                qDebug() << "AudioMatrixWorker: Already processing, ignoring duplicate call";
                return;
            }
            _isProcessing = true;
            _lastProcessedByOutput.assign(_outputBuffers.size(), 0);
            _deferredEmptyTsByOutput.assign(_outputBuffers.size(), 0);
            for (auto &buf : _inputBuffers) {
                if (buf) {
                    buf->registerFrameWaiter(&_tickWaiter);
                }
            }
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioMatrixWorker::stopProcessing()
    {
        _stopRequested.store(true, std::memory_order_release);
        {
            QMutexLocker locker(&_mutex);
            unregisterAllInputWaitersLocked();
        }
        _tickWaiter.requestStop();

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

    void AudioMatrixWorker::audioLoop()
    {
        AudioThreadRealtimeGuard realtimeGuard(L"Pro Audio");
        while (!_stopRequested.load(std::memory_order_acquire)
               && !_tickWaiter.isStopRequested()) {
            _tickWaiter.wait(20);
            if (_stopRequested.load(std::memory_order_acquire)
                || _tickWaiter.isStopRequested()) {
                break;
            }
            processCurrentFrame();
        }
    }

    void AudioMatrixWorker::updateMatrix(const Eigen::MatrixXd& matrix)
    {
        QMutexLocker locker(&_mutex);
        _matrix = matrix;
    }

    void AudioMatrixWorker::processAudioData()
    {
        processCurrentFrame();
    }

    void AudioMatrixWorker::processCurrentFrame()
    {
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        Eigen::MatrixXd matrix;
        std::vector<qint64> lastByOut;
        std::vector<qint64> deferredByOut;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            matrix = _matrix;
            if (_lastProcessedByOutput.size() != outputs.size()) {
                _lastProcessedByOutput.assign(outputs.size(), 0);
            }
            if (_deferredEmptyTsByOutput.size() != outputs.size()) {
                _deferredEmptyTsByOutput.assign(outputs.size(), 0);
            }
            lastByOut = _lastProcessedByOutput;
            deferredByOut = _deferredEmptyTsByOutput;
        }

        const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
        bool anyProgress = false;
        const int outCount = static_cast<int>(outputs.size());
        for (int out = 0; out < outCount; ++out) {
            int maxFrames = 2;
            if (clockTs > lastByOut[static_cast<size_t>(out)] + 2) {
                maxFrames = static_cast<int>(
                    qMin<qint64>(8, clockTs - lastByOut[static_cast<size_t>(out)]));
            }
            if (processOutputMix(out, inputs, outputs, matrix,
                                 lastByOut[static_cast<size_t>(out)],
                                 deferredByOut[static_cast<size_t>(out)],
                                 clockTs, maxFrames) > 0) {
                anyProgress = true;
            }
        }

        {
            QMutexLocker locker(&_mutex);
            _lastProcessedByOutput = std::move(lastByOut);
            _deferredEmptyTsByOutput = std::move(deferredByOut);
        }
        if (anyProgress) {
            emit audioProcessed(outputs);
        }
    }

    int AudioMatrixWorker::processOutputMix(int out,
                                            const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                            const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                            const Eigen::MatrixXd &matrix,
                                            qint64 &inoutLastTs,
                                            qint64 &inoutDeferredEmptyTs,
                                            qint64 clockTs,
                                            int maxFramesPerWake)
    {
        if (out < 0 || out >= static_cast<int>(outputs.size()) || !outputs[static_cast<size_t>(out)]) {
            return 0;
        }
        if (matrix.cols() <= out || matrix.rows() <= 0) {
            return 0;
        }

        std::vector<int> contributors;
        contributors.reserve(static_cast<size_t>(matrix.rows()));
        for (int in = 0; in < matrix.rows(); ++in) {
            if (std::abs(matrix(in, out)) <= kMuteFloorLinear) {
                continue;
            }
            if (in < 0 || in >= static_cast<int>(inputs.size())) {
                continue;
            }
            // Inactive / 断线：不参与齐套（等价 Stop→从混音集合移除）
            if (!inputs[static_cast<size_t>(in)] || !inputs[static_cast<size_t>(in)]->isActive()) {
                continue;
            }
            contributors.push_back(in);
        }
        if (contributors.empty()) {
            return 0;
        }

        constexpr int kSampleRate = 48000;
        const size_t frameSize = static_cast<size_t>(
            TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate));
        if (frameSize == 0) {
            return 0;
        }

        const bool multiSourceMix = contributors.size() > 1;
        // 找帧与透传一致：按 nextTs 前进；tip 未到只等；
        // 单源二次缺→跳戳；多源二次缺→带戳出帧（缺路=0）。写出 nextTs+D。
        constexpr qint64 kTipStaleFrames = 16;
        const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
        qint64 tipLatest = 0;
        bool haveTip = false;
        for (int in : contributors) {
            const qint64 L = inputs[static_cast<size_t>(in)]->latestTimestamp();
            if (L <= 0) {
                continue;
            }
            // tip 冻住过久的源不拖整路 NotYet，缺帧时按填 0 处理
            if (clockTs > L && (clockTs - L) > kTipStaleFrames) {
                continue;
            }
            if (!haveTip || L < tipLatest) {
                tipLatest = L;
            }
            haveTip = true;
        }
        qint64 cursor = inoutLastTs;
        if (cursor <= 0 || cursor < clockTs - 64) {
            cursor = clockTs - 1;
        }

        const int framesThisWake = multiSourceMix ? 2 : maxFramesPerWake;

        int processedCount = 0;
        int holeSkips = 0;
        while (processedCount < framesThisWake) {
            const qint64 nextTs = cursor + 1;
            if (!haveTip || nextTs > tipLatest) {
                break; // NotYet：绝不填 0
            }

            std::vector<float> mixed(frameSize, 0.0f);
            int sampleRate = kSampleRate;
            bool allPresent = true;
            for (int in : contributors) {
                const auto &queue = inputs[static_cast<size_t>(in)];
                if (!queue || !queue->isActive()) {
                    allPresent = false;
                    continue;
                }
                AudioFrame inFrame;
                const bool got = queue->getFrameByTimestamp(nextTs, inFrame)
                    && inFrame.timestamp == nextTs
                    && !inFrame.data.isEmpty();
                if (!got) {
                    allPresent = false;
                    continue; // 该路保持 0
                }
                if (inFrame.sampleRate > 0) {
                    sampleRate = inFrame.sampleRate;
                }
                const float gain = static_cast<float>(matrix(in, out));
                const float *src = reinterpret_cast<const float *>(inFrame.data.constData());
                const size_t available = inFrame.data.size() / sizeof(float);
                const size_t n = qMin(frameSize, available);
                for (size_t s = 0; s < n; ++s) {
                    mixed[s] += src[s] * gain;
                }
            }

            if (!allPresent) {
                if (inoutDeferredEmptyTs != nextTs) {
                    inoutDeferredEmptyTs = nextTs;
                    break; // 第一次缺：等
                }
                inoutDeferredEmptyTs = 0;
                if (!multiSourceMix) {
                    cursor = nextTs; // 单源二次缺：跳戳不 push
                    if (++holeSkips > 16) {
                        break;
                    }
                    continue;
                }
                // 多源二次缺：下面 push，缺路已是 0
            } else {
                inoutDeferredEmptyTs = 0;
            }

            AudioFrame outputFrame;
            outputFrame.timestamp = nextTs + delayFrames;
            outputFrame.sampleRate = sampleRate;
            outputFrame.channels = 1;
            outputFrame.bitsPerSample = 32;
            outputFrame.data.resize(static_cast<int>(frameSize * sizeof(float)));
            float *dst = reinterpret_cast<float *>(outputFrame.data.data());
            for (size_t s = 0; s < frameSize; ++s) {
                float v = mixed[s];
                if (v > 1.0f) {
                    v = 1.0f;
                } else if (v < -1.0f) {
                    v = -1.0f;
                }
                dst[s] = v;
            }

            outputs[static_cast<size_t>(out)]->pushFrame(outputFrame);
            cursor = nextTs;
            ++processedCount;
            inoutDeferredEmptyTs = 0;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    void AudioMatrixWorker::initializeBuffers(int inputCount, int outputCount, const Eigen::MatrixXd& matrix)
    {
        QMutexLocker locker(&_mutex);

        inputCount = qMax(1, inputCount);
        outputCount = qMax(1, outputCount);

        unregisterAllInputWaitersLocked();
        _inputBuffers.resize(static_cast<size_t>(inputCount));

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(outputCount));
        for (int i = 0; i < outputCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }

        _matrix = matrix;
        _lastProcessedByOutput.assign(static_cast<size_t>(outputCount), 0);
        _deferredEmptyTsByOutput.assign(static_cast<size_t>(outputCount), 0);
    }

    void AudioMatrixWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port < 0 || port >= static_cast<int>(_inputBuffers.size())) {
            qWarning() << "AudioMatrixWorker: Invalid input port index:" << port
                       << "(valid range: 0 -" << (_inputBuffers.size() - 1) << ")";
            return;
        }
        auto &slot = _inputBuffers[static_cast<size_t>(port)];
        if (slot) {
            slot->unregisterFrameWaiter(&_tickWaiter);
        }
        slot = buffer;
        if (slot && _isProcessing) {
            slot->registerFrameWaiter(&_tickWaiter);
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioMatrixWorker::getOutputBuffer(int port)
    {
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[port];
        }
        return nullptr;
    }
}
