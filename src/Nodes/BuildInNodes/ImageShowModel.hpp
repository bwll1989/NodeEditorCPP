#pragma once

/**
 * @file ImageShowModel.hpp
 * @brief Image Display — 节点内嵌入式 GPU 纹理预览
 *
 * ## 职责
 * - 在 QtNodes 节点面板中显示上游 ImageData 的当前帧（零拷贝 bind textureId）
 * - 透传同一 ImageData 句柄到输出端口，供下游继续连接
 *
 * ## 数据流
 * @code
 * 上游 ImageData（共享 ImageTimestampRingQueue）
 *         │
 *         ├─ setInData：保存 m_inImage，绑定 newFrameWritten
 *         │
 *         ├─ tick / newFrameWritten → updateDisplayFromInput
 *         │       └─ getLatestFrame / getFrameByTimestamp → ImageTextureViewHost
 *         │
 *         └─ outData()：返回同一 m_inImage 指针（透传）
 * @endcode
 *
 * ## 刷新策略（与 WindowDisplayModel 对齐，并针对 QGraphicsProxyWidget 增强）
 * 1. **TimestampGenerator::frameCountUpdated**（QueuedConnection）— 每系统 tick 拉帧
 * 2. **ImageTimestampRingQueue::newFrameWritten** — GPU 算子 push 后立即排队刷新
 * 3. **setInData** — 换源/重连时立即尝试拉帧，减少首帧空白
 *
 * ## 与 setInData 的约定
 * - 连接建立/断开时**不在 setInData 内清屏**；清屏由 updateDisplayFromInput 在确认无有效帧后执行
 * - 换源时仅重置 m_lastDisplayedTimestamp / m_lastDisplayedTextureId 去重标记
 *
 * @see ImageShowModel.cpp  实现细节
 * @see WindowDisplayModel  全屏窗口显示（逻辑同源）
 * @see ImageTextureViewHost  QGraphicsView 内 GL 刷新桥接
 */

#include <QtNodes/NodeDelegateModelRegistry>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Elements/ImageTextureView/ImageTextureViewHost.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <memory>

#include <QMetaObject>

using namespace NodeDataTypes;

namespace Nodes
{

/**
 * @brief 嵌入式图像显示节点（Image Display）
 *
 * 节点 UI 内嵌 ImageTextureViewHost，在节点画布中预览 GPU 纹理。
 * 输入/输出均为 ImageData；输出与输入为同一 shared_ptr（透传，不复制像素）。
 */
class ImageShowModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    ImageShowModel();
    ~ImageShowModel() override = default;

    /** @brief 输入/输出端口均为 image 类型 */
    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    /**
     * @brief 返回透传的 ImageData 句柄
     * @note 与 m_inImage 相同；下游读到的是上游 ring buffer 的同一对象
     */
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

    /**
     * @brief 上游连接变化时更新输入引用
     *
     * - 保存 m_inImage，m_outData = m_inImage
     * - 重置显示去重标记
     * - bindInputBuffer() 订阅上游 newFrameWritten
     * - 立即 updateDisplayFromInput() 尝试显示首帧
     */
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

    /** @brief 节点内嵌预览控件（ImageTextureViewHost → ImageTextureView） */
    QWidget* embeddedWidget() override { return _viewHost; }

    /** @brief 允许用户在画布上拖拽调整节点预览区域大小 */
    bool resizable() const override { return true; }

private:
    /**
     * @brief 将 m_inImage 的 ring buffer 与 newFrameWritten 信号关联
     *
     * 换源前先 disconnect 旧连接，避免悬空订阅。
     * GPU 双输入算子（Add/Subtract/Cross 等）在 tick 内 push 后 emit newFrameWritten，
     * 本节点可早于下一 tick 完成刷新，避免与 Display tick 的竞态导致误清屏。
     */
    void bindInputBuffer();

    /**
     * @brief 从上游 buffer 取帧并更新内嵌预览
     * @param frameCount 当前系统帧号（用于 getFrameByTimestamp 回退查找）
     *
     * 决策树见 ImageShowModel.cpp 内注释。
     */
    void updateDisplayFromInput(qint64 frameCount);

    /** @brief TimestampGenerator 每 tick 的槽函数，委托 updateDisplayFromInput */
    void onSystemFrameTick(qint64 frameCount);

    /**
     * @brief 帧内容未变则跳过 setTexture，否则写入 ImageTextureViewHost
     * @return 成功绑定有效纹理返回 true
     *
     * 去重依据：timestamp + textureId（同一 tick 重复 push 或重复 tick 时避免多余 GL 刷新）
     */
    bool applyFrameIfChanged(const ImageFrame& frame);

    /**
     * @brief 仅当曾显示过有效帧时清屏一次
     *
     * m_lastDisplayedTimestamp < 0 表示从未显示或已清过，不再重复 clearTexture，
     * 避免无上游时每 tick 触发无效 GL 重绘。
     */
    void clearDisplayIfNeeded();

    /**
     * @brief 从 ring buffer 解析可用于显示的 ImageFrame
     * @param buffer 上游共享环形缓冲
     * @param frameCount 系统 tick 帧号（精确查找回退）
     * @param frame 输出：解析到的帧
     * @return 得到有效 GpuTextureHandle 时返回 true
     *
     * 查找顺序：
     * 1. getLatestFrame（显示节点主路径）
     * 2. getFrameByTimestamp(frameCount)
     *
     * 若仅有 CPU Mat 缓存，会尝试 ensureGpuTexture() lazy 上传。
     */
    bool resolveDisplayFrame(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                             qint64 frameCount,
                             ImageFrame& frame);

    ImageTextureViewHost* _viewHost = nullptr;              ///< 内嵌 GL 预览容器（节点 embeddedWidget）
    std::shared_ptr<ImageData> m_inImage;                   ///< 上游输入 ImageData 句柄
    std::shared_ptr<ImageData> m_outData;                   ///< 透传输出（与 m_inImage 相同）
    QMetaObject::Connection m_bufferFrameConnection;        ///< 上游 buffer::newFrameWritten 连接句柄
    qint64 m_lastDisplayedTimestamp = -1;                   ///< 上次显示的帧时间戳；-1 表示未显示
    unsigned int m_lastDisplayedTextureId = 0;              ///< 上次 bind 的 textureId，配合 timestamp 去重
};

} // namespace Nodes
