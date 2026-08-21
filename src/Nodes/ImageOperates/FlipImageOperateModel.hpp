#pragma once

/**
 * @file FlipImageOperateModel.hpp
 * @brief Image Flip — 水平/垂直翻转（GPU resample）
 *
 * 本文件是 ImageOperates GPU 算子的**参考模板**，新增算子请对齐以下约定：
 * - setInData：只更新连接与 m_paramsDirty，不在此跑 GPU / clearOutput
 * - 参数 setter：只 m_paramsDirty = true
 * - tick (frameCountUpdated) → requestProcess → clearOutput 或 pushGpuResult
 *
 * @see Doc.md §3  ImageOperateCommon.hpp
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace FlipImageOperateGpu
{
/** 通过 resample UV 区间实现镜像；horizontal/vertical 控制是否翻转对应轴 */
inline GpuTextureHandle run(const GpuTextureHandle& src, bool horizontal, bool vertical)
{
    if (!src.valid()) {
        return {};
    }
    return ImageGpuPass::instance().resample(
        src, src.width, src.height, 0.f, 0.f, 1.f, 1.f, horizontal, vertical);
}
} // namespace FlipImageOperateGpu

/** @brief 图像翻转节点 — 行为约定见文件头与 Doc.md §3 */
class FlipImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(bool horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged)
    Q_PROPERTY(bool vertical READ vertical WRITE setVertical NOTIFY verticalChanged)

public:
    FlipImageOperateModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Flip";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "horizontal";
            AbstractDelegateModel::registerExternalBinding("/horizontal", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "vertical";
            AbstractDelegateModel::registerExternalBinding("/vertical", this, binding);
        }

        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

        // tick 驱动：同帧且参数未脏则跳过；否则 requestProcess
        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64 frameCount) {
                    if (m_lastRequestedFrame == frameCount && !m_paramsDirty) {
                        return;
                    }
                    m_lastRequestedFrame = frameCount;
                    requestProcess(frameCount);
                },
                Qt::QueuedConnection);
    }

    ~FlipImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    bool horizontal() const { return m_horizontal; }
    bool vertical() const { return m_vertical; }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            return portIndex == 0 ? ImageData().type() : VariableData().type();
        }
        return ImageData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case 0: return "IMAGE";
            case 1: return "H";
            case 2: return "V";
            default: return QString();
            }
        }
        return "IMAGE";
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return m_outImageData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        switch (portIndex) {
        case 0:
            // 图像口：仅保存上游句柄并重置输入去重；清空/计算留给 tick
            m_inImage = std::dynamic_pointer_cast<ImageData>(data);
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            m_lastProcessedInputTimestamp = -1;
            m_paramsDirty = true;
            break;
        case 1:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHorizontal(variable->asBool());
            } else if (!data) {
                setHorizontal(false);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setVertical(variable->asBool());
            } else if (!data) {
                setVertical(false);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["horizontal"] = m_horizontal;
        values["vertical"] = m_vertical;
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setHorizontal(values.value("horizontal").toBool());
        setVertical(values.value("vertical").toBool());
    }

public slots:
    void setHorizontal(bool value)
    {
        if (m_horizontal == value) {
            return;
        }
        m_horizontal = value;
        m_paramsDirty = true;
        Q_EMIT horizontalChanged(value);
    }

    void setVertical(bool value)
    {
        if (m_vertical == value) {
            return;
        }
        m_vertical = value;
        m_paramsDirty = true;
        Q_EMIT verticalChanged(value);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/horizontal")) {
            setHorizontal(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/vertical")) {
            setVertical(ev.payload.toBool());
        }
    }

signals:
    void horizontalChanged(bool value);
    void verticalChanged(bool value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/horizontal"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/vertical"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    /** 清空输出 ring buffer 并重置去重状态；Display 在下一 tick 通过 isEmpty() 清屏 */
    void clearOutput()
    {
        if (m_outBuffer) {
            m_outBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
        m_lastProcessedInputTimestamp = -1;
        m_paramsDirty = false;
    }

    /**
     * @brief 按目标帧号取输入、执行 GPU 翻转，并将结果写入输出 ring buffer
     * @param targetTimestamp 上游对齐用的查找帧号；传 -1 时使用当前系统帧号
     */
    void requestProcess(qint64 targetTimestamp = -1)
    {
        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

        const qint64 lookupTimestamp = ImageOperateHelpers::normalizeTargetTimestamp(targetTimestamp);

        if (!m_inImage || imageDataIsEmpty(m_inImage)) {
            clearOutput();
            return;
        }

        ImageFrame inputFrame;
        if (!ImageOperateHelpers::resolveInputGpuFrame(m_inImage, lookupTimestamp, inputFrame)) {
            // 上游尚无可用帧：保留现有输出，避免重连瞬间误清空
            if (!ImageOperateHelpers::hasInputImage(m_inImage)) {
                clearOutput();
            }
            return;
        }

        // 输入帧与参数均未变则跳过 GPU（静态图不重复算）
        if (!m_paramsDirty && inputFrame.timestamp == m_lastProcessedInputTimestamp) {
            return;
        }

        GpuTextureHandle out = FlipImageOperateGpu::run(
            inputFrame.texture, m_horizontal, m_vertical);
        ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
    }

    std::shared_ptr<ImageData> m_inImage;                       ///< 上游 ImageData 句柄（非像素副本）
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;       ///< 本节点输出 ring buffer
    std::shared_ptr<ImageData> m_outImageData;                  ///< 稳定输出句柄，outData() 始终返回同一对象

    qint64 m_lastRequestedFrame = -1;              ///< tick 同帧去重
    qint64 m_lastProcessedInputTimestamp = -1;     ///< 输入帧未变 + 参数未脏 → 跳过 GPU
    qint64 m_lastPushedTimestamp = -1;             ///< pushGpuResult 辅助
    bool m_paramsDirty = false;                    ///< 输入/参数变化后由 tick 消费
    bool m_horizontal = true;
    bool m_vertical = false;
};
} // namespace Nodes
