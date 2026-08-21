#pragma once

/**
 * @file BlurImageOperateModel.hpp
 * @brief Image Blur — 可分离高斯模糊（GLSL 1.x，双 pass）
 *
 * BlurImageOperateGpu::run 先水平后垂直；半径 clamp 0–32。
 * 节点 tick / setInData 约定同 FlipImageOperateModel（见 Doc.md §3）。
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include <algorithm>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace BlurImageOperateGpu
{
/** 单方向高斯卷积；uHorizontal 区分水平/垂直 pass */
static const char kFragBlur[] = R"(
uniform sampler2D uTexture;
uniform vec2 uTexelSize;
uniform int uRadius;
uniform bool uHorizontal;
varying vec2 vTexCoord;
void main() {
    const int MAX_R = 32;
    vec4 sum = vec4(0.0);
    float wSum = 0.0;
    float sigma = max(float(uRadius), 1.0) * 0.5;
    float invTwoSigma2 = 1.0 / (2.0 * sigma * sigma);
    for (int i = -MAX_R; i <= MAX_R; ++i) {
        if (i < -uRadius || i > uRadius) continue;
        float w = exp(-float(i * i) * invTwoSigma2);
        vec2 offset = uHorizontal ? vec2(float(i) * uTexelSize.x, 0.0)
                                  : vec2(0.0, float(i) * uTexelSize.y);
        sum += texture2D(uTexture, vTexCoord + offset) * w;
        wSum += w;
    }
    gl_FragColor = sum / max(wSum, 0.0001);
}
)";

inline GpuTextureHandle blurPass(const GpuTextureHandle& src, int radius, bool horizontal)
{
    if (radius <= 0) {
        return src;
    }
    const float invW = 1.f / static_cast<float>(src.width);
    const float invH = 1.f / static_cast<float>(src.height);
    return ImageGpuPass::instance().runFragmentPass(
        src.width,
        src.height,
        kFragBlur,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uTexelSize", invW, invH);
            program.setUniformValue("uRadius", radius);
            program.setUniformValue("uHorizontal", horizontal);
        },
        [&](QOpenGLFunctions* f) { ImageGpuPass::bindTexture(f, 0, src.textureId); });
}

inline GpuTextureHandle run(const GpuTextureHandle& src, int radiusX, int radiusY)
{
    if (!src.valid()) {
        return {};
    }
    const int rx = std::clamp(radiusX, 0, 32);
    const int ry = std::clamp(radiusY, 0, 32);
    if (rx == 0 && ry == 0) {
        return ImageGpuPass::instance().resize(src, src.width, src.height);
    }
    GpuTextureHandle current = src;
    if (rx > 0) {
        current = blurPass(current, rx, true);
    }
    if (!current.valid()) {
        return {};
    }
    if (ry > 0) {
        current = blurPass(current, ry, false);
    }
    return current;
}
} // namespace BlurImageOperateGpu

/** @brief 高斯模糊 — GPU 双 pass，半径由 RADIUS X/Y 控制 */
class BlurImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int radiusX READ radiusX WRITE setRadiusX NOTIFY radiusXChanged)
    Q_PROPERTY(int radiusY READ radiusY WRITE setRadiusY NOTIFY radiusYChanged)

public:
    BlurImageOperateModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Blur";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "radiusX";
            AbstractDelegateModel::registerExternalBinding("/radiusX", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "radiusY";
            AbstractDelegateModel::registerExternalBinding("/radiusY", this, binding);
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

    ~BlurImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    int radiusX() const { return m_radiusX; }
    int radiusY() const { return m_radiusY; }

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
            case 1: return "RADIUS X";
            case 2: return "RADIUS Y";
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
                setRadiusX(variable->asNumber());
            } else if (!data) {
                setRadiusX(0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setRadiusY(variable->asNumber());
            } else if (!data) {
                setRadiusY(0);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["radiusX"] = m_radiusX;
        values["radiusY"] = m_radiusY;
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setRadiusX(values.value("radiusX").toInt());
        setRadiusY(values.value("radiusY").toInt());
    }

public slots:
    void setRadiusX(int value)
    {
        const int clamped = std::clamp(value, 0, 32);
        if (m_radiusX == clamped) {
            return;
        }
        m_radiusX = clamped;
        m_paramsDirty = true;
        Q_EMIT radiusXChanged(m_radiusX);
    }

    void setRadiusY(int value)
    {
        const int clamped = std::clamp(value, 0, 32);
        if (m_radiusY == clamped) {
            return;
        }
        m_radiusY = clamped;
        m_paramsDirty = true;
        Q_EMIT radiusYChanged(m_radiusY);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/radiusX")) {
            setRadiusX(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/radiusY")) {
            setRadiusY(ev.payload.toInt());
        }
    }

signals:
    void radiusXChanged(int value);
    void radiusYChanged(int value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/radiusX"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/radiusY"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        GpuTextureHandle out = BlurImageOperateGpu::run(
            inputFrame.texture, m_radiusX, m_radiusY);
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
    int m_radiusX = 8;
    int m_radiusY = 8;
};
} // namespace Nodes
