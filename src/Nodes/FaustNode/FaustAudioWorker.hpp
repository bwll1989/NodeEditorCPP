#pragma once

#include <QObject>
#include <QMutex>
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include "faust/dsp/dsp.h"

namespace Nodes
{
    /**
     * @brief Faust DSP 的音频工作线程。
     *
     * 与项目现有音频中间节点（VST3 / AudioGate）同构：
     * - QtNodes 图只在接线时交换 ring queue 句柄，不传播样本；
     * - 上游 pushFrame 经 AudioTickWaiter 直接唤醒本线程（20ms 兜底）；
     * - 按全局时间戳追帧，多输入对齐取帧，**一次 compute() 处理全部通道**
     *   （Faust 的延迟线/反馈状态跨通道共享，不能像 AudioGate 那样逐通道独立处理）；
     * - 输出帧时间戳 = 输入戳 + getAudioOutputDelayFrames()（多源对齐惯例）。
     *
     * 生命周期：重载/销毁必须先 stopProcessing() join 线程，再拆 DSP 实例，
     * 避免 compute() 与 delete m_dsp 并发（VST3 同款顺序）。
     */
    class FaustAudioWorker : public QObject
    {
        Q_OBJECT
    public:
        explicit FaustAudioWorker(QObject* parent = nullptr);
        ~FaustAudioWorker() override;

        /** GUI 线程调用：取第 port 路输出 ring queue（句柄交换用） */
        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port) const;

        /** GUI 线程调用：绑定/解绑第 port 路输入 ring queue（nullptr = 断连） */
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    public slots:
        void startProcessing();
        void stopProcessing();

        /** 重载 DSP 时调用；必须在 stop 之后、start 之前 */
        void setDsp(::dsp* dsp, int sampleRate);
        /** 按 .dsp 的 getNumInputs/Outputs 重建 buffer 数组 */
        void configureChannels(int inputCount, int outputCount);

    private:
        void audioLoop();
        void processFrame();

        struct FrameSnapshot
        {
            std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
            std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
            ::dsp* dsp = nullptr;
            int sampleRate = 48000;
        };

        mutable QMutex m_mutex;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> m_inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> m_outputs;
        ::dsp* m_dsp = nullptr;
        int m_sampleRate = 48000;
        bool m_isProcessing = false;

        qint64 m_lastTs = 0;
        qint64 m_deferredTs = 0;

        AudioTickWaiter m_waiter;
        std::atomic<bool> m_stop{false};
        std::thread m_thread;
    };
}
