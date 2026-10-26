#pragma once

/**
 * @file ImageOperateCookDriver.hpp
 * @brief 图像算子：环形队列 push 唤醒处理线程（对齐 AudioGateWorker）
 *
 * 与音频对称：
 * - ImageTimestampRingQueue::pushFrame → notifyFrameWaiters → AudioTickWaiter
 * - 专用 cook 线程 wait；多路输入注册同一 waiter，pending_ 合并多次 notify
 * - GPU 须在 GUI 线程：唤醒后 QueuedConnection 调回 GUI 执行 process
 *
 * 参数变更：markDirty()（置位 + notify，不经全局 tick）
 *
 * 注意：不再从 cook 线程读取模型侧 bool* paramsDirty（非原子 + 析构时悬空）。
 * 参数路径一律走 markDirty()；模型本地 m_paramsDirty 仅供 requestProcess 去重。
 */

#include "NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QMetaObject>
#include <QObject>
#include <QPointer>

#include <atomic>
#include <functional>
#include <initializer_list>
#include <memory>
#include <thread>
#include <vector>

namespace Nodes
{
namespace ImageOperateHelpers
{

/**
 * @brief push 唤醒的图像 cook 驱动（对外仍称 DirtyTickCook，少改各 Model）
 */
class DirtyTickCook final : public QObject
{
    Q_OBJECT

public:
    using ProcessFn = std::function<void()>;

    /**
     * @param paramsDirtyFlag 保留兼容；cook 线程不再解引用（见文件头说明）
     */
    DirtyTickCook(QObject* parent, ProcessFn process, bool* /*paramsDirtyFlag*/ = nullptr)
        : QObject(parent)
        , m_process(std::move(process))
        , m_guiTarget(parent)
    {
        startCookThread();
    }

    ~DirtyTickCook() override
    {
        stopCookThread();
        clearWaiterRegistrations();
    }

    /** @brief 参数/换线：置 dirty 并唤醒 cook 线程 */
    void markDirty()
    {
        m_inputDirty.store(true, std::memory_order_release);
        m_waiter.notifyFromClock();
    }

    /**
     * @brief 重绑监视的输入 ImageData（换线时调用）
     * 在各输入 ring 上 registerFrameWaiter；任意一路 push 即唤醒
     */
    void setWatchedInputs(std::initializer_list<std::shared_ptr<NodeDataTypes::ImageData>> inputs)
    {
        clearWaiterRegistrations();

        for (const auto& imageData : inputs) {
            if (!imageData || !imageData->isConnectedToSharedBuffer()) {
                continue;
            }
            const auto buffer = imageData->getSharedImageBuffer();
            if (!buffer) {
                continue;
            }
            buffer->registerFrameWaiter(&m_waiter);
            m_watchedBuffers.push_back(buffer);
        }
        markDirty();
    }

private:
    void startCookThread()
    {
        m_stopRequested.store(false, std::memory_order_release);
        m_waiter.reset();
        m_cookThread = std::thread([this]() { cookLoop(); });
    }

    void stopCookThread()
    {
        m_stopRequested.store(true, std::memory_order_release);
        m_waiter.requestStop();
        if (m_cookThread.joinable()) {
            if (m_cookThread.get_id() != std::this_thread::get_id()) {
                m_cookThread.join();
            } else {
                m_cookThread.detach();
            }
        }
    }

    void clearWaiterRegistrations()
    {
        for (const auto& buffer : m_watchedBuffers) {
            if (buffer) {
                buffer->unregisterFrameWaiter(&m_waiter);
            }
        }
        m_watchedBuffers.clear();
    }

    void cookLoop()
    {
        while (!m_stopRequested.load(std::memory_order_acquire)
               && !m_waiter.isStopRequested()) {
            // true = push / markDirty 唤醒；false = 纯超时
            const bool woken = m_waiter.wait(20);
            if (m_stopRequested.load(std::memory_order_acquire)
                || m_waiter.isStopRequested()) {
                break;
            }

            // push 只 notify，不写 dirty —— 唤醒本身即有活
            if (woken) {
                m_inputDirty.store(true, std::memory_order_release);
            }

            if (!m_inputDirty.exchange(false, std::memory_order_acq_rel)) {
                continue;
            }
            scheduleGuiProcess();
        }
    }

    void scheduleGuiProcess()
    {
        if (m_guiScheduled.exchange(true, std::memory_order_acq_rel)) {
            // 已有 GUI 任务在排队/执行：留下 dirty，结束后再跑
            m_inputDirty.store(true, std::memory_order_release);
            return;
        }
        QObject* target = m_guiTarget.data();
        if (!target) {
            m_guiScheduled.store(false, std::memory_order_release);
            return;
        }

        // QPointer：析构后事件队列里残留的回调直接跳过，避免 UAF
        QPointer<DirtyTickCook> self(this);
        QMetaObject::invokeMethod(
            target,
            [self]() {
                if (!self) {
                    return;
                }
                if (self->m_stopRequested.load(std::memory_order_acquire)) {
                    self->m_guiScheduled.store(false, std::memory_order_release);
                    return;
                }
                self->m_inputDirty.store(false, std::memory_order_release);
                if (self->m_process) {
                    self->m_process();
                }
                // 先跑完再清 scheduled，避免 process 期间重复入队两份
                self->m_guiScheduled.store(false, std::memory_order_release);
                if (self->m_inputDirty.load(std::memory_order_acquire)) {
                    self->scheduleGuiProcess();
                }
            },
            Qt::QueuedConnection);
    }

    ProcessFn m_process;
    QPointer<QObject> m_guiTarget;

    AudioTickWaiter m_waiter;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_inputDirty{false};
    std::atomic<bool> m_guiScheduled{false};
    std::thread m_cookThread;
    std::vector<std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>> m_watchedBuffers;
};

} // namespace ImageOperateHelpers
} // namespace Nodes
