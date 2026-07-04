#include "ImageShowModel.hpp"

#include <QtNodes/NodeDelegateModelRegistry>

using namespace Nodes;
using namespace NodeDataTypes;

// =============================================================================
// ImageShowModel — 嵌入式图像显示节点（Image Display）
// =============================================================================
//
// 【架构】
//   上游 ImageData ──shared_ptr──► m_inImage / m_outData（透传）
//                                      │
//                                      ▼
//                            ImageTimestampRingQueue
//                                      │
//                    getLatestFrame / getFrameByTimestamp
//                                      │
//                                      ▼
//                            ImageTextureViewHost::setTexture
//                                      │
//                                      ▼
//                            ImageTextureView::paintGL
//
// 【刷新触发源】
//   A. TimestampGenerator::frameCountUpdated  → onSystemFrameTick
//   B. ImageTimestampRingQueue::newFrameWritten（bindInputBuffer 订阅）
//   C. setInData 末尾立即 updateDisplayFromInput
//
// 【为何需要 B + C】
//   GPU 算子（如 Image Subtract）在同一 tick 内可能晚于 Display 的 Queued tick 才 push。
//   若 Display 先跑且 buffer 仍空，旧逻辑会 clearTexture；算子 push 后又要等下一 tick。
//   订阅 newFrameWritten + setInData 立即拉帧，可消除该竞态。
//
// 【清屏策略】
//   - 真正无上游 / 无 shared buffer / buffer.isEmpty() → 清纹理
//   - 有连接、buffer 非空、但本 tick 暂无可绑 GPU 纹理且从未显示过 → clearDisplayIfNeeded
//   - 有连接、buffer 非空、暂取不到纹理但曾显示过 → 保留上一帧（不闪黑）
// =============================================================================

ImageShowModel::ImageShowModel()
    : _viewHost(new ImageTextureViewHost())
{
    InPortCount = 1;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = "Image Display";
    WidgetEmbeddable = false;
    Resizable = false;

    // QueuedConnection：与 GPU 算子 tick 同队列顺序执行，避免跨线程 GL 访问
    connect(TimestampGenerator::getInstance(),
            &TimestampGenerator::frameCountUpdated,
            this,
            &ImageShowModel::onSystemFrameTick,
            Qt::QueuedConnection);
}

QtNodes::NodeDataType ImageShowModel::dataType(QtNodes::PortType const, QtNodes::PortIndex const) const
{
    return ImageData().type();
}

std::shared_ptr<QtNodes::NodeData> ImageShowModel::outData(QtNodes::PortIndex)
{
    // 透传：下游与上游共享同一 ImageData 与 ring buffer
    return m_outData;
}

void ImageShowModel::bindInputBuffer()
{
    // 换源或断开时先解除旧 buffer 订阅，防止析构后仍触发 lambda
    QObject::disconnect(m_bufferFrameConnection);
    m_bufferFrameConnection = {};

    if (!m_inImage || !m_inImage->isConnectedToSharedBuffer()) {
        return;
    }

    const auto buffer = m_inImage->getSharedImageBuffer();
    if (!buffer) {
        return;
    }

    // GPU 算子 pushFrame 后会 emit newFrameWritten；Queued 排队到主线程刷新预览
    m_bufferFrameConnection = connect(
        buffer.get(),
        &ImageTimestampRingQueue::newFrameWritten,
        this,
        [this](const ImageFrame&) {
            updateDisplayFromInput(TimestampGenerator::getInstance()->getCurrentFrameCount());
        },
        Qt::QueuedConnection);
}

void ImageShowModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const)
{
    m_inImage = std::dynamic_pointer_cast<ImageData>(nodeData);
    m_outData = m_inImage;

    if (m_inImage) {
        // 换源：重置去重标记，强制下一帧 applyFrameIfChanged 执行 setTexture
        // 注意：此处 intentionally 不调用 clearTexture，避免重连瞬间闪黑
        m_lastDisplayedTimestamp = -1;
        m_lastDisplayedTextureId = 0;
    }

    bindInputBuffer();

    // 连接建立后立即拉帧（静态图 / 已有 buffer 内容时可免等下一 tick）
    updateDisplayFromInput(TimestampGenerator::getInstance()->getCurrentFrameCount());

    // 通知下游端口数据句柄可能已变化（透传指针变更）
    Q_EMIT dataUpdated(0);
}

void ImageShowModel::clearDisplayIfNeeded()
{
    // 从未显示过有效帧时无需清 GL，减少 QGraphicsProxyWidget 内无效重绘
    if (!_viewHost || m_lastDisplayedTimestamp < 0) {
        return;
    }
    m_lastDisplayedTimestamp = -1;
    m_lastDisplayedTextureId = 0;
    _viewHost->clearTexture();
}

bool ImageShowModel::applyFrameIfChanged(const ImageFrame& frame)
{
    if (!frame.texture.valid() || !_viewHost) {
        return false;
    }

    // 同 timestamp + 同 textureId：跳过 setTexture（静态场景 / 重复 tick）
    if (m_lastDisplayedTimestamp >= 0
        && frame.timestamp == m_lastDisplayedTimestamp
        && frame.texture.textureId == m_lastDisplayedTextureId) {
        return true;
    }

    _viewHost->setTexture(frame.texture);
    m_lastDisplayedTimestamp = frame.timestamp;
    m_lastDisplayedTextureId = frame.texture.textureId;
    return true;
}

bool ImageShowModel::resolveDisplayFrame(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                         qint64 frameCount,
                                         ImageFrame& frame)
{
    if (!buffer) {
        return false;
    }

    // 主路径：取 ring buffer 中最新有效帧（O(1)）
    if (buffer->getLatestFrame(frame)) {
        // legacy 节点可能只写 CPU Mat；显示需要 GPU 纹理，尝试 lazy 上传
        if (!frame.texture.valid() && frame.hasCpuCache()) {
            ImageFrame uploadFrame = frame;
            if (uploadFrame.ensureGpuTexture()) {
                frame = std::move(uploadFrame);
            }
        }
        return frame.texture.valid();
    }

    // 回退：按当前系统 tick 帧号精确查找（tick 对齐的上游帧）
    if (buffer->getFrameByTimestamp(frameCount, frame)) {
        if (!frame.texture.valid() && frame.hasCpuCache()) {
            ImageFrame uploadFrame = frame;
            if (uploadFrame.ensureGpuTexture()) {
                frame = std::move(uploadFrame);
            }
        }
        return frame.texture.valid();
    }

    return false;
}

void ImageShowModel::updateDisplayFromInput(qint64 frameCount)
{
    if (!_viewHost) {
        return;
    }

    // ── 1. 无上游连接 ──
    if (!m_inImage) {
        clearDisplayIfNeeded();
        return;
    }

    // ── 2. 上游未挂载 ring buffer（非共享 ImageData 路径）──
    if (!m_inImage->isConnectedToSharedBuffer()) {
        clearDisplayIfNeeded();
        return;
    }

    const auto buffer = m_inImage->getSharedImageBuffer();
    if (!buffer) {
        clearDisplayIfNeeded();
        return;
    }

    // ── 3. buffer 已清空（上游算子 clearOutput / 断开输入后）──
    if (m_inImage->isEmpty()) {
        _viewHost->clearTexture();
        m_lastDisplayedTimestamp = -1;
        m_lastDisplayedTextureId = 0;
        return;
    }

    // ── 4. 尝试解析可显示的 GPU 帧 ──
    ImageFrame frame;
    if (resolveDisplayFrame(buffer, frameCount, frame)) {
        applyFrameIfChanged(frame);
        return;
    }

    // ── 5. 竞态保护：buffer 非空但本 tick 尚未 resolve 到纹理 ──
    // 典型场景：Display tick 先于 Subtract/Add 等双输入算子 push。
    // 若曾成功显示过，保留上一帧画面，等待 newFrameWritten 或下一 tick。
    if (m_lastDisplayedTimestamp >= 0) {
        return;
    }

    // ── 6. 从未显示且确实取不到帧 → 清屏一次 ──
    clearDisplayIfNeeded();
}

void ImageShowModel::onSystemFrameTick(qint64 frameCount)
{
    updateDisplayFromInput(frameCount);
}
