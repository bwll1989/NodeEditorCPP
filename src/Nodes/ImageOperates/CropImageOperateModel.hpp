#pragma once

/**
 * @file CropImageOperateModel.hpp
 * @brief Image Crop — 按四边百分比裁剪（GPU resample 子矩形）
 *
 * 百分比基于输入宽高；输出尺寸 = 原尺寸 − 四边像素。无效区域返回空纹理。
 * 末口 CROP 与 ROI 一致：0–1 的 [x, y, w, h]。
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>

#include <algorithm>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace CropImageOperateGpu
{
inline GpuTextureHandle run(const GpuTextureHandle& src,
                            double leftPercent,
                            double rightPercent,
                            double topPercent,
                            double bottomPercent)
{
    if (!src.valid()) {
        return {};
    }

    const int left = static_cast<int>(std::round(src.width * std::clamp(leftPercent, 0.0, 100.0) / 100.0));
    const int right = static_cast<int>(std::round(src.width * std::clamp(rightPercent, 0.0, 100.0) / 100.0));
    const int top = static_cast<int>(std::round(src.height * std::clamp(topPercent, 0.0, 100.0) / 100.0));
    const int bottom = static_cast<int>(std::round(src.height * std::clamp(bottomPercent, 0.0, 100.0) / 100.0));

    const int outW = src.width - left - right;
    const int outH = src.height - top - bottom;
    if (outW <= 0 || outH <= 0) {
        return {};
    }

    const float u0 = static_cast<float>(left) / static_cast<float>(src.width);
    const float v0 = static_cast<float>(top) / static_cast<float>(src.height);
    const float u1 = static_cast<float>(src.width - right) / static_cast<float>(src.width);
    const float v1 = static_cast<float>(src.height - bottom) / static_cast<float>(src.height);

    return ImageGpuPass::instance().resample(src, outW, outH, u0, v0, u1, v1);
}
} // namespace CropImageOperateGpu

/** @brief 百分比裁剪 — 单输入 GPU 算子 */
class CropImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double leftPercent READ leftPercent WRITE setLeftPercent NOTIFY leftPercentChanged)
    Q_PROPERTY(double rightPercent READ rightPercent WRITE setRightPercent NOTIFY rightPercentChanged)
    Q_PROPERTY(double topPercent READ topPercent WRITE setTopPercent NOTIFY topPercentChanged)
    Q_PROPERTY(double bottomPercent READ bottomPercent WRITE setBottomPercent NOTIFY bottomPercentChanged)

public:
    CropImageOperateModel()
    {
        InPortCount = 6;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Crop";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "leftPercent";
            AbstractDelegateModel::registerExternalBinding("/left", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "rightPercent";
            AbstractDelegateModel::registerExternalBinding("/right", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "topPercent";
            AbstractDelegateModel::registerExternalBinding("/top", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "bottomPercent";
            AbstractDelegateModel::registerExternalBinding("/bottom", this, binding);
        }

        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

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

    ~CropImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    double leftPercent() const { return m_leftPercent; }
    double rightPercent() const { return m_rightPercent; }
    double topPercent() const { return m_topPercent; }
    double bottomPercent() const { return m_bottomPercent; }

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
            case 1: return "LEFT %";
            case 2: return "RIGHT %";
            case 3: return "TOP %";
            case 4: return "BOTTOM %";
            case 5: return "CROP";
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
            m_inImage = std::dynamic_pointer_cast<ImageData>(data);
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            m_lastProcessedInputTimestamp = -1;
            m_paramsDirty = true;
            Q_EMIT dataUpdated(0);
            break;
        case 1:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setLeftPercent(variable->asNumber());
            } else if (!data) {
                setLeftPercent(0.0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setRightPercent(variable->asNumber());
            } else if (!data) {
                setRightPercent(0.0);
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setTopPercent(variable->asNumber());
            } else if (!data) {
                setTopPercent(0.0);
            }
            break;
        case 4:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setBottomPercent(variable->asNumber());
            } else if (!data) {
                setBottomPercent(0.0);
            }
            break;
        case 5:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                const QVector<float> crop = variable->asFloats(4);
                setCropNorm(double(crop[0]), double(crop[1]), double(crop[2]), double(crop[3]));
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["leftPercent"] = m_leftPercent;
        values["rightPercent"] = m_rightPercent;
        values["topPercent"] = m_topPercent;
        values["bottomPercent"] = m_bottomPercent;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setLeftPercent(values.value("leftPercent").toDouble());
        setRightPercent(values.value("rightPercent").toDouble());
        setTopPercent(values.value("topPercent").toDouble());
        setBottomPercent(values.value("bottomPercent").toDouble());
    }

public slots:
    void setLeftPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_leftPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_leftPercent = clamped;
        m_paramsDirty = true;
        Q_EMIT leftPercentChanged(m_leftPercent);
    }

    void setRightPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_rightPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_rightPercent = clamped;
        m_paramsDirty = true;
        Q_EMIT rightPercentChanged(m_rightPercent);
    }

    void setTopPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_topPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_topPercent = clamped;
        m_paramsDirty = true;
        Q_EMIT topPercentChanged(m_topPercent);
    }

    void setBottomPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_bottomPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_bottomPercent = clamped;
        m_paramsDirty = true;
        Q_EMIT bottomPercentChanged(m_bottomPercent);
    }

    void setCropNorm(double x, double y, double w, double h)
    {
        x = std::clamp(x, 0.0, 1.0);
        y = std::clamp(y, 0.0, 1.0);
        w = std::clamp(w, 0.0, 1.0);
        h = std::clamp(h, 0.0, 1.0);
        w = std::min(w, 1.0 - x);
        h = std::min(h, 1.0 - y);
        if (w <= 0.0 || h <= 0.0) {
            return;
        }
        setLeftPercent(x * 100.0);
        setTopPercent(y * 100.0);
        setRightPercent((1.0 - x - w) * 100.0);
        setBottomPercent((1.0 - y - h) * 100.0);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        if (ev.address == makeFullOscAddress("/left")) {
            setLeftPercent(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/right")) {
            setRightPercent(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/top")) {
            setTopPercent(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/bottom")) {
            setBottomPercent(ev.payload.toDouble());
        }
    }

signals:
    void leftPercentChanged(double value);
    void rightPercentChanged(double value);
    void topPercentChanged(double value);
    void bottomPercentChanged(double value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/left"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/right"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/top"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/bottom"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    static double clampPercent(double value)
    {
        return std::clamp(value, 0.0, 100.0);
    }

    void clearOutput()
    {
        if (m_outBuffer) {
            m_outBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
        m_lastProcessedInputTimestamp = -1;
        m_paramsDirty = false;
    }

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
            if (!ImageOperateHelpers::hasInputImage(m_inImage)) {
                clearOutput();
            }
            return;
        }

        if (!m_paramsDirty && inputFrame.timestamp == m_lastProcessedInputTimestamp) {
            return;
        }

        GpuTextureHandle out = CropImageOperateGpu::run(
            inputFrame.texture, m_leftPercent, m_rightPercent, m_topPercent, m_bottomPercent);
        ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
    }

    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    qint64 m_lastPushedTimestamp = -1;
    bool m_paramsDirty = false;
    double m_leftPercent = 0.0;
    double m_rightPercent = 0.0;
    double m_topPercent = 0.0;
    double m_bottomPercent = 0.0;
};
} // namespace Nodes
