#pragma once

/**
 * @file FlipImageOperateModel.hpp
 * @brief Image Flip — 水平/垂直翻转（GPU resample）
 *
 * 本文件是 ImageOperates GPU 算子的**参考模板**，新增算子请对齐以下约定：
 * - setInData：只更新连接与 m_paramsDirty，不在此跑 GPU / clearOutput
 * - 参数 setter：m_paramsDirty = true + m_cook->markDirty()
 * - DirtyTickCook（队列唤醒）→ requestProcess → clearOutput 或 pushGpuResult
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
/** éè¿ resample UV åºé´å®ç°éåï¼horizontal/vertical æ§å¶æ¯å¦ç¿»è½¬å¯¹åºè½´ */
inline GpuTextureHandle run(const GpuTextureHandle& src, bool horizontal, bool vertical)
{
    if (!src.valid()) {
        return {};
    }
    return ImageGpuPass::instance().resample(
        src, src.width, src.height, 0.f, 0.f, 1.f, 1.f, horizontal, vertical);
}
} // namespace FlipImageOperateGpu

/** @brief å¾åç¿»è½¬èç¹ â è¡ä¸ºçº¦å®è§æä»¶å¤´ä¸ Doc.md Â§3 */
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

        m_cook = new ImageOperateHelpers::DirtyTickCook(
            this,
            [this]() { requestProcess(); },
            &m_paramsDirty);
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
            // å¾åå£ï¼ä»ä¿å­ä¸æ¸¸å¥æå¹¶éç½®è¾å¥å»éï¼æ¸ç©º/è®¡ç®çç» tick
            m_inImage = std::dynamic_pointer_cast<ImageData>(data);
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            m_lastProcessedInputTimestamp = -1;
            m_paramsDirty = true;
            if (m_cook) {
                m_cook->setWatchedInputs({m_inImage});
            }
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
        if (m_cook) {
            m_cook->markDirty();
        }
        Q_EMIT horizontalChanged(value);
    }

    void setVertical(bool value)
    {
        if (m_vertical == value) {
            return;
        }
        m_vertical = value;
        m_paramsDirty = true;
        if (m_cook) {
            m_cook->markDirty();
        }
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

    /** æ¸ç©ºè¾åº ring buffer å¹¶éç½®å»éç¶æï¼Display å¨ä¸ä¸ tick éè¿ isEmpty() æ¸å± */
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
     * @brief åææ°è¾å¥å¸§ãæ§è¡ GPU ç¿»è½¬ï¼å¹¶å°ç»æåå¥è¾åº ring buffer
     */
    void requestProcess()
    {
        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
        if (!m_inImage || imageDataIsEmpty(m_inImage)) {
            clearOutput();
            return;
        }

        ImageFrame inputFrame;
        if (!ImageOperateHelpers::resolveInputGpuFrameLatest(m_inImage, inputFrame)) {
            if (!ImageOperateHelpers::hasInputImage(m_inImage)) {
                clearOutput();
            }
            return;
        }

        if (!m_paramsDirty && inputFrame.timestamp == m_lastProcessedInputTimestamp) {
            return;
        }

        GpuTextureHandle out = FlipImageOperateGpu::run(
            inputFrame.texture, m_horizontal, m_vertical);
        ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp,
            inputFrame.timestamp);

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
    }

    std::shared_ptr<ImageData> m_inImage;                       ///< ä¸æ¸¸ ImageData å¥æï¼éåç´ å¯æ¬ï¼
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;       ///< æ¬èç¹è¾åº ring buffer
    std::shared_ptr<ImageData> m_outImageData;                  ///< ç¨³å®è¾åºå¥æï¼outData() å§ç»è¿ååä¸å¯¹è±¡

    ImageOperateHelpers::DirtyTickCook* m_cook = nullptr;
    qint64 m_lastProcessedInputTimestamp = -1;
    qint64 m_lastPushedTimestamp = -1;
    bool m_paramsDirty = false;
    bool m_horizontal = true;
    bool m_vertical = false;
};
} // namespace Nodes
