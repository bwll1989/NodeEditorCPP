#pragma once

/**
 * @file MonochromeImageOperateModel.hpp
 * @brief Image Monochrome — 彩色转灰度（类似 TD Monochrome TOP）
 *
 * 通过 RGB / Alpha 两套通道选择分别计算灰度值，再用 mono 在彩色与灰度间混合：
 * - mono = 0：保留原色
 * - mono = 255：完全灰度（黑白色）
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include <algorithm>

struct GlobalEvent;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace MonochromeImageOperateGpu
{
static const char kFragMonochrome[] = R"(
uniform sampler2D uTexture;
uniform float uMono;
uniform int uRgbSource;
uniform int uAlphaSource;
varying vec2 vTexCoord;

float sampleChannel(vec4 c, int mode) {
    vec3 bgr = vec3(c.b, c.g, c.r);
    if (mode == 0) {
        return dot(bgr, vec3(0.114, 0.587, 0.299));
    }
    if (mode == 1) {
        return bgr.r;
    }
    if (mode == 2) {
        return bgr.g;
    }
    if (mode == 3) {
        return bgr.b;
    }
    if (mode == 4) {
        return c.a;
    }
    if (mode == 5) {
        return (bgr.r + bgr.g + bgr.b) / 3.0;
    }
    return (bgr.r + bgr.g + bgr.b + c.a) / 4.0;
}

void main() {
    vec4 c = texture2D(uTexture, vTexCoord);
    float grayRgb = sampleChannel(c, uRgbSource);
    float grayAlpha = sampleChannel(c, uAlphaSource);
    vec3 grayColor = vec3(grayRgb, grayRgb, grayRgb);
    vec3 outRgb = mix(c.rgb, grayColor, uMono);
    float outA = mix(c.a, grayAlpha, uMono);
    gl_FragColor = vec4(outRgb, outA);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& src, int mono, int rgbSource, int alphaSource)
{
    if (!src.valid()) {
        return {};
    }
    const float monoNorm = static_cast<float>(std::clamp(mono, 0, 255)) / 255.0f;
    const int rgb = std::clamp(rgbSource, 0, 6);
    const int alpha = std::clamp(alphaSource, 0, 6);
    return ImageGpuPass::instance().runFragmentPass(
        src.width,
        src.height,
        kFragMonochrome,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uMono", monoNorm);
            program.setUniformValue("uRgbSource", rgb);
            program.setUniformValue("uAlphaSource", alpha);
        },
        [&](QOpenGLFunctions* f) { ImageGpuPass::bindTexture(f, 0, src.textureId); });
}
} // namespace MonochromeImageOperateGpu

/** @brief 灰度化 — 单输入 GPU 算子 */
class MonochromeImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT

    /** 灰度取样通道，对齐 TD Monochrome TOP → RGB / Alpha 菜单 */
    enum class MonoChannelSource : int {
        Luminance = 0,
        Red = 1,
        Green = 2,
        Blue = 3,
        Alpha = 4,
        RgbAverage = 5,
        RgbaAverage = 6
    };
    Q_ENUM(MonoChannelSource)

    Q_PROPERTY(int mono READ mono WRITE setMono NOTIFY monoChanged)
    Q_PROPERTY(MonoChannelSource rgbSource READ rgbSource WRITE setRgbSource NOTIFY rgbSourceChanged)
    Q_PROPERTY(int rgbSourceIndex READ rgbSourceIndex WRITE setRgbSourceIndex NOTIFY rgbSourceIndexChanged DESIGNABLE false)
    Q_PROPERTY(MonoChannelSource alphaSource READ alphaSource WRITE setAlphaSource NOTIFY alphaSourceChanged)
    Q_PROPERTY(int alphaSourceIndex READ alphaSourceIndex WRITE setAlphaSourceIndex NOTIFY alphaSourceIndexChanged DESIGNABLE false)

public:
    MonochromeImageOperateModel()
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Monochrome";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "mono";
            AbstractDelegateModel::registerExternalBinding("/mono", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "rgbSourceIndex";
            AbstractDelegateModel::registerExternalBinding("/rgbSource", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "alphaSourceIndex";
            AbstractDelegateModel::registerExternalBinding("/alphaSource", this, b);
        }

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

    ~MonochromeImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    int mono() const { return m_mono; }
    MonoChannelSource rgbSource() const { return m_rgbSource; }
    int rgbSourceIndex() const { return static_cast<int>(m_rgbSource); }
    MonoChannelSource alphaSource() const { return m_alphaSource; }
    int alphaSourceIndex() const { return static_cast<int>(m_alphaSource); }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
        case PortType::In:
            return portIndex == 0 ? ImageData().type() : VariableData().type();
        case PortType::Out:
            return ImageData().type();
        default:
            return ImageData().type();
        }
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
        case PortType::In:
            switch (portIndex) {
            case 0: return "IMAGE";
            case 1: return "MONO";
            case 2: return "RGB";
            case 3: return "ALPHA";
            default: return QString();
            }
        case PortType::Out:
            return "IMAGE";
        default:
            return QString();
        }
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
                setMono(variable->asNumber());
            } else if (!data) {
                setMono(255);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setRgbSourceIndex(variable->asNumber());
            } else if (!data) {
                setRgbSourceIndex(static_cast<int>(MonoChannelSource::Luminance));
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setAlphaSourceIndex(variable->asNumber());
            } else if (!data) {
                setAlphaSourceIndex(static_cast<int>(MonoChannelSource::Luminance));
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["mono"] = m_mono;
        values["rgbSource"] = static_cast<int>(m_rgbSource);
        values["alphaSource"] = static_cast<int>(m_alphaSource);

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        const QJsonObject values = p["values"].toObject();
        setMono(values.value("mono").toInt(255));
        setRgbSourceIndex(values.value("rgbSource").toInt(static_cast<int>(MonoChannelSource::Luminance)));
        setAlphaSourceIndex(values.value("alphaSource").toInt(static_cast<int>(MonoChannelSource::Luminance)));
    }

public Q_SLOTS:
    void setMono(int v)
    {
        const int clamped = std::clamp(v, 0, 255);
        if (m_mono == clamped) {
            return;
        }
        m_mono = clamped;
        m_paramsDirty = true;
        Q_EMIT monoChanged(m_mono);
    }

    void setRgbSource(MonoChannelSource v)
    {
        if (m_rgbSource == v) {
            return;
        }
        m_rgbSource = v;
        m_paramsDirty = true;
        Q_EMIT rgbSourceChanged();
        Q_EMIT rgbSourceIndexChanged(rgbSourceIndex());
    }

    void setRgbSourceIndex(int v)
    {
        setRgbSource(static_cast<MonoChannelSource>(std::clamp(v, 0, 6)));
    }

    void setAlphaSource(MonoChannelSource v)
    {
        if (m_alphaSource == v) {
            return;
        }
        m_alphaSource = v;
        m_paramsDirty = true;
        Q_EMIT alphaSourceChanged();
        Q_EMIT alphaSourceIndexChanged(alphaSourceIndex());
    }

    void setAlphaSourceIndex(int v)
    {
        setAlphaSource(static_cast<MonoChannelSource>(std::clamp(v, 0, 6)));
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/mono")) {
            setMono(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/rgbSource")) {
            setRgbSourceIndex(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/alphaSource")) {
            setAlphaSourceIndex(ev.payload.toInt());
        }
    }

Q_SIGNALS:
    void monoChanged(int v);
    void rgbSourceChanged();
    void rgbSourceIndexChanged(int v);
    void alphaSourceChanged();
    void alphaSourceIndexChanged(int v);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/mono"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/rgbSource"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/alphaSource"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private:
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

        if (!ImageOperateHelpers::hasInputImage(m_inImage)) {
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

        GpuTextureHandle out = MonochromeImageOperateGpu::run(
            inputFrame.texture,
            m_mono,
            static_cast<int>(m_rgbSource),
            static_cast<int>(m_alphaSource));
        ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
        Q_EMIT dataUpdated(0);
    }

    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    qint64 m_lastPushedTimestamp = -1;
    bool m_paramsDirty = false;
    int m_mono = 255;
    MonoChannelSource m_rgbSource = MonoChannelSource::Luminance;
    MonoChannelSource m_alphaSource = MonoChannelSource::Luminance;
};

} // namespace Nodes
