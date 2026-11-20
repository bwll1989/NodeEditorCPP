#include "FaustAudioWorker.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

namespace Nodes
{
    namespace
    {
        bool fetchContentFrame(const std::shared_ptr<AudioTimestampRingQueue>& queue,
                               qint64 contentTs, AudioFrame& outFrame)
        {
            if (!queue) {
                return false;
            }
            return queue->getFrameByTimestamp(contentTs, outFrame)
                && outFrame.timestamp == contentTs
                && !outFrame.data.isEmpty();
        }
    }

    FaustAudioWorker::FaustAudioWorker(QObject* parent)
        : QObject(parent)
    {}

    FaustAudioWorker::~FaustAudioWorker()
    {
        stopProcessing();
    }

    std::shared_ptr<AudioTimestampRingQueue> FaustAudioWorker::getOutputBuffer(int port) const
    {
        QMutexLocker locker(&m_mutex);
        if (port < 0 || port >= static_cast<int>(m_outputs.size())) {
            return nullptr;
        }
        return m_outputs[static_cast<std::size_t>(port)];
    }

    void FaustAudioWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&m_mutex);
        if (port < 0 || port >= static_cast<int>(m_inputs.size())) {
            return;
        }
        auto& slot = m_inputs[static_cast<std::size_t>(port)];
        if (slot && m_isProcessing) {
            slot->unregisterFrameWaiter(&m_waiter);
        }
        slot = buffer;
        if (slot && m_isProcessing) {
            slot->registerFrameWaiter(&m_waiter);
        }
    }

    void FaustAudioWorker::configureChannels(int inputCount, int outputCount)
    {
        QMutexLocker locker(&m_mutex);
        for (auto& buf : m_inputs) {
            if (buf && m_isProcessing) {
                buf->unregisterFrameWaiter(&m_waiter);
            }
        }
        m_inputs.assign(static_cast<std::size_t>(qMax(0, inputCount)), nullptr);
        m_outputs.assign(static_cast<std::size_t>(qMax(0, outputCount)), nullptr);
        for (auto& buf : m_outputs) {
            buf = std::make_shared<AudioTimestampRingQueue>();
        }
        m_lastTs = 0;
        m_deferredTs = 0;
    }

    void FaustAudioWorker::setDsp(::dsp* dsp, int sampleRate)
    {
        QMutexLocker locker(&m_mutex);
        m_dsp = dsp;
        m_sampleRate = sampleRate > 0 ? sampleRate : 48000;
    }

    void FaustAudioWorker::startProcessing()
    {
        {
            QMutexLocker locker(&m_mutex);
            if (m_isProcessing) {
                return;
            }
            m_isProcessing = true;
            m_lastTs = 0;
            m_deferredTs = 0;
            for (auto& buf : m_inputs) {
                if (buf) {
                    buf->registerFrameWaiter(&m_waiter);
                }
            }
        }
        m_stop.store(false, std::memory_order_release);
        m_waiter.reset();
        m_thread = std::thread([this]() { audioLoop(); });
    }

    void FaustAudioWorker::stopProcessing()
    {
        m_stop.store(true, std::memory_order_release);
        {
            QMutexLocker locker(&m_mutex);
            for (auto& buf : m_inputs) {
                if (buf) {
                    buf->unregisterFrameWaiter(&m_waiter);
                }
            }
        }
        m_waiter.requestStop();
        if (m_thread.joinable()) {
            if (m_thread.get_id() != std::this_thread::get_id()) {
                m_thread.join();
            } else {
                m_thread.detach();
            }
        }
        QMutexLocker locker(&m_mutex);
        m_isProcessing = false;
    }

    void FaustAudioWorker::audioLoop()
    {
        AudioThreadRealtimeGuard realtimeGuard(L"Pro Audio");
        while (!m_stop.load(std::memory_order_acquire) && !m_waiter.isStopRequested()) {
            m_waiter.wait(20);
            if (m_stop.load(std::memory_order_acquire) || m_waiter.isStopRequested()) {
                break;
            }
            processFrame();
        }
    }

    void FaustAudioWorker::processFrame()
    {
        FrameSnapshot snap;
        {
            QMutexLocker locker(&m_mutex);
            if (!m_isProcessing || !m_dsp || m_outputs.empty()) {
                return;
            }
            snap.inputs = m_inputs;
            snap.outputs = m_outputs;
            snap.dsp = m_dsp;
            snap.sampleRate = m_sampleRate;
        }

        const int inCount = snap.dsp->getNumInputs();
        const int outCount = snap.dsp->getNumOutputs();
        if (inCount <= 0 && outCount <= 0) {
            return;
        }

        int connected = 0;
        bool haveTip = false;
        qint64 tipLatest = 0;
        const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
        const qint64 kStaleFrames = 16;
        for (const auto& q : snap.inputs) {
            if (!q || !q->isActive()) {
                continue;
            }
            ++connected;
            const qint64 latest = q->latestTimestamp();
            if (latest <= 0 || (clockTs > latest && clockTs - latest > kStaleFrames)) {
                continue;
            }
            if (!haveTip || latest < tipLatest) {
                tipLatest = latest;
            }
            haveTip = true;
        }

        qint64 cursor = m_lastTs;
        if (cursor <= 0 || cursor < clockTs - 64) {
            cursor = clockTs - 1;
        }
        int maxFrames = 2;
        if (clockTs > cursor + 2) {
            maxFrames = static_cast<int>(qMin<qint64>(8, clockTs - cursor));
        }
        if (connected > 1) {
            maxFrames = qMin(maxFrames, 2); // 多源对齐保守推进（VST3 惯例）
        }

        const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
        const int count = qMax(
            1, TimestampGenerator::getInstance()->getSamplesPerFrame(snap.sampleRate));

        std::vector<std::vector<FAUSTFLOAT>> inData(
            static_cast<std::size_t>(qMax(0, inCount)),
            std::vector<FAUSTFLOAT>(static_cast<std::size_t>(count), FAUSTFLOAT(0)));
        std::vector<std::vector<FAUSTFLOAT>> outData(
            static_cast<std::size_t>(qMax(0, outCount)),
            std::vector<FAUSTFLOAT>(static_cast<std::size_t>(count), FAUSTFLOAT(0)));
        std::vector<FAUSTFLOAT*> inPtrs(inData.size());
        std::vector<FAUSTFLOAT*> outPtrs(outData.size());
        for (std::size_t i = 0; i < inData.size(); ++i) {
            inPtrs[i] = inData[i].data();
        }
        for (std::size_t i = 0; i < outData.size(); ++i) {
            outPtrs[i] = outData[i].data();
        }

        int processed = 0;
        int holeSkips = 0;
        while (processed < maxFrames) {
            const qint64 nextTs = cursor + 1;

            if (connected == 0) {
                if (nextTs > clockTs) {
                    break;
                }
            } else {
                if (!haveTip || nextTs > tipLatest) {
                    break; // NotYet：上游还没写到这一戳
                }
                bool allPresent = true;
                for (const auto& q : snap.inputs) {
                    if (!q || !q->isActive()) {
                        continue;
                    }
                    AudioFrame probe;
                    if (!fetchContentFrame(q, nextTs, probe)) {
                        allPresent = false;
                        break;
                    }
                }
                if (!allPresent) {
                    if (m_deferredTs != nextTs) {
                        m_deferredTs = nextTs;
                        break; // 缺帧先 defer 一次，等下一轮
                    }
                    m_deferredTs = 0;
                    if (connected == 1) {
                        cursor = nextTs;
                        if (++holeSkips > 16) {
                            break;
                        }
                        continue; // 单源空洞跳过
                    }
                    // 多源二次缺：缺路输入保持静音，仍然 compute
                }
            }
            m_deferredTs = 0;

            // 取帧 → 填充 Faust 输入缓冲（单声道端口 ↔ Faust 输入通道一一对应）
            for (int ch = 0; ch < inCount && ch < static_cast<int>(snap.inputs.size()); ++ch) {
                std::fill(inData[static_cast<std::size_t>(ch)].begin(),
                          inData[static_cast<std::size_t>(ch)].end(), FAUSTFLOAT(0));
                const auto& queue = snap.inputs[static_cast<std::size_t>(ch)];
                if (!queue || !queue->isActive()) {
                    continue;
                }
                AudioFrame frame;
                if (!fetchContentFrame(queue, nextTs, frame)) {
                    continue;
                }
                const auto* src = reinterpret_cast<const float*>(frame.data.constData());
                const int n = qMin(static_cast<int>(frame.data.size() / sizeof(float)), count);
                for (int i = 0; i < n; ++i) {
                    inData[static_cast<std::size_t>(ch)][static_cast<std::size_t>(i)] =
                        static_cast<FAUSTFLOAT>(src[i]);
                }
            }

            // 【核心】一次 compute 处理所有通道（Faust 内部状态跨通道共享）
            snap.dsp->compute(count, inPtrs.empty() ? nullptr : inPtrs.data(),
                              outPtrs.empty() ? nullptr : outPtrs.data());

            const qint64 outputTs = nextTs + (connected > 0 ? delayFrames : 0);
            for (int ch = 0; ch < outCount && ch < static_cast<int>(snap.outputs.size()); ++ch) {
                const auto& queue = snap.outputs[static_cast<std::size_t>(ch)];
                if (!queue) {
                    continue;
                }
                QByteArray bytes(static_cast<qsizetype>(count) * static_cast<qsizetype>(sizeof(float)),
                                 Qt::Uninitialized);
                auto* dst = reinterpret_cast<float*>(bytes.data());
                for (int i = 0; i < count; ++i) {
                    dst[i] = qBound(-1.0f,
                                    static_cast<float>(outData[static_cast<std::size_t>(ch)][
                                        static_cast<std::size_t>(i)]),
                                    1.0f);
                }
                AudioFrame outFrame;
                outFrame.data = bytes;
                outFrame.sampleRate = snap.sampleRate;
                outFrame.channels = 1;
                outFrame.bitsPerSample = 32;
                outFrame.timestamp = outputTs;
                queue->pushFrame(outFrame);
            }

            cursor = nextTs;
            ++processed;
        }

        if (processed > 0) {
            m_lastTs = cursor;
        }
    }
}
