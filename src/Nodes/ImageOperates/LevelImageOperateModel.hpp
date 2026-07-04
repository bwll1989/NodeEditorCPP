#pragma once

/**
 * @file LevelImageOperateModel.hpp
 * @brief Image Level — 亮度/伽马/黑电平/量化/不透明度（类似 TD Luma Level TOP）
 *
 * 在选定源通道（默认 Luminance）上构建 lookup 曲线，再按比例缩放 RGB 以尽量保持色相：
 *   out.rgb = in.rgb * lookup(src) / src
 *   out.a   = in.a * opacity
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
namespace LevelImageOperateGpu
{
static const char kFragLevel[] = R"(
uniform sampler2D uTexture;
uniform float uBlackLevel;
uniform float uBrightness;
uniform float uGamma;
uniform float uStepSize;
uniform float uOpacity;
uniform int uSource;
varying vec2 vTexCoord;

float sampleSource(vec4 c, int mode) {
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

float levelLookup(float v) {
    if (v <= uBlackLevel) {
        v = 0.0;
    } else if (uBlackLevel < 1.0) {
        v = (v - uBlackLevel) / (1.0 - uBlackLevel);
    }
    v = v + uBrightness;
    v = clamp(v, 0.0, 1.0);
    if (uGamma > 0.0001) {
        v = pow(max(v, 0.0), 1.0 / uGamma);
    }
    if (uStepSize > 0.0001) {
        float bands = 1.0 / uStepSize;
        v = floor(v * bands) / bands;
    }
    return clamp(v, 0.0, 1.0);
}

void main() {
    vec4 c = texture2D(uTexture, vTexCoord);
    float src = sampleSource(c, uSource);
    float mapped = levelLookup(src);
    float scale = (src > 0.0001) ? (mapped / src) : mapped;
    vec3 outRgb = clamp(c.rgb * scale, 0.0, 1.0);
    gl_FragColor = vec4(outRgb, c.a * uOpacity);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& src,
                            int blackLevel,
                            int brightness,
                            int gamma100,
                            int stepSize,
                            int opacity,
                            int source)
{
    if (!src.valid()) {
        return {};
    }
    const float black = static_cast<float>(std::clamp(blackLevel, 0, 255)) / 255.0f;
    const float bright = static_cast<float>(std::clamp(brightness, -255, 255)) / 255.0f;
    const float gamma = static_cast<float>(std::clamp(gamma100, 10, 400)) / 100.0f;
    const float step = stepSize <= 0 ? 0.0f : static_cast<float>(std::clamp(stepSize, 1, 255)) / 255.0f;
    const float opa = static_cast<float>(std::clamp(opacity, 0, 255)) / 255.0f;
    const int srcMode = std::clamp(source, 0, 6);

    return ImageGpuPass::instance().runFragmentPass(
        src.width,
        src.height,
        kFragLevel,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uBlackLevel", black);
            program.setUniformValue("uBrightness", bright);
            program.setUniformValue("uGamma", gamma);
            program.setUniformValue("uStepSize", step);
            program.setUniformValue("uOpacity", opa);
            program.setUniformValue("uSource", srcMode);
        },
        [&](QOpenGLFunctions* f) { ImageGpuPass::bindTexture(f, 0, src.textureId); });
}
} // namespace LevelImageOperateGpu

/** @brief 色阶/亮度调整 — 单输入 GPU 算子 */
class LevelImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT

    enum class LevelSource : int {
        Luminance = 0,
        Red = 1,
        Green = 2,
        Blue = 3,
        Alpha = 4,
        RgbAverage = 5,
        RgbaAverage = 6
    };
    Q_ENUM(LevelSource)

    Q_PROPERTY(int blackLevel READ blackLevel WRITE setBlackLevel NOTIFY blackLevelChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int gamma READ gamma WRITE setGamma NOTIFY gammaChanged)
    Q_PROPERTY(int stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged)
    Q_PROPERTY(int opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
    Q_PROPERTY(LevelSource source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int sourceIndex READ sourceIndex WRITE setSourceIndex NOTIFY sourceIndexChanged DESIGNABLE false)

public:
    LevelImageOperateModel()
    {
        InPortCount = 6;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Level";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "blackLevel";
            AbstractDelegateModel::registerExternalBinding("/blackLevel", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "brightness";
            AbstractDelegateModel::registerExternalBinding("/brightness", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "gamma";
            AbstractDelegateModel::registerExternalBinding("/gamma", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "stepSize";
            AbstractDelegateModel::registerExternalBinding("/stepSize", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "opacity";
            AbstractDelegateModel::registerExternalBinding("/opacity", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "sourceIndex";
            AbstractDelegateModel::registerExternalBinding("/source", this, b);
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

    ~LevelImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    int blackLevel() const { return m_blackLevel; }
    int brightness() const { return m_brightness; }
    int gamma() const { return m_gamma; }
    int stepSize() const { return m_stepSize; }
    int opacity() const { return m_opacity; }
    LevelSource source() const { return m_source; }
    int sourceIndex() const { return static_cast<int>(m_source); }

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
            case 1: return "BRIGHTNESS";
            case 2: return "GAMMA";
            case 3: return "BLACK";
            case 4: return "STEP";
            case 5: return "OPACITY";
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
                setBrightness(variable->value().toInt());
            } else if (!data) {
                setBrightness(0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setGamma(variable->value().toInt());
            } else if (!data) {
                setGamma(100);
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setBlackLevel(variable->value().toInt());
            } else if (!data) {
                setBlackLevel(0);
            }
            break;
        case 4:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setStepSize(variable->value().toInt());
            } else if (!data) {
                setStepSize(0);
            }
            break;
        case 5:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setOpacity(variable->value().toInt());
            } else if (!data) {
                setOpacity(255);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["blackLevel"] = m_blackLevel;
        values["brightness"] = m_brightness;
        values["gamma"] = m_gamma;
        values["stepSize"] = m_stepSize;
        values["opacity"] = m_opacity;
        values["source"] = static_cast<int>(m_source);

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        const QJsonObject values = p["values"].toObject();
        setBlackLevel(values.value("blackLevel").toInt(0));
        setBrightness(values.value("brightness").toInt(0));
        setGamma(values.value("gamma").toInt(100));
        setStepSize(values.value("stepSize").toInt(0));
        setOpacity(values.value("opacity").toInt(255));
        setSourceIndex(values.value("source").toInt(static_cast<int>(LevelSource::Luminance)));
    }

public Q_SLOTS:
    void setBlackLevel(int v)
    {
        const int clamped = std::clamp(v, 0, 255);
        if (m_blackLevel == clamped) {
            return;
        }
        m_blackLevel = clamped;
        m_paramsDirty = true;
        Q_EMIT blackLevelChanged(m_blackLevel);
    }

    void setBrightness(int v)
    {
        const int clamped = std::clamp(v, -255, 255);
        if (m_brightness == clamped) {
            return;
        }
        m_brightness = clamped;
        m_paramsDirty = true;
        Q_EMIT brightnessChanged(m_brightness);
    }

    void setGamma(int v)
    {
        const int clamped = std::clamp(v, 10, 400);
        if (m_gamma == clamped) {
            return;
        }
        m_gamma = clamped;
        m_paramsDirty = true;
        Q_EMIT gammaChanged(m_gamma);
    }

    void setStepSize(int v)
    {
        const int clamped = std::clamp(v, 0, 255);
        if (m_stepSize == clamped) {
            return;
        }
        m_stepSize = clamped;
        m_paramsDirty = true;
        Q_EMIT stepSizeChanged(m_stepSize);
    }

    void setOpacity(int v)
    {
        const int clamped = std::clamp(v, 0, 255);
        if (m_opacity == clamped) {
            return;
        }
        m_opacity = clamped;
        m_paramsDirty = true;
        Q_EMIT opacityChanged(m_opacity);
    }

    void setSource(LevelSource v)
    {
        if (m_source == v) {
            return;
        }
        m_source = v;
        m_paramsDirty = true;
        Q_EMIT sourceChanged();
        Q_EMIT sourceIndexChanged(sourceIndex());
    }

    void setSourceIndex(int v)
    {
        setSource(static_cast<LevelSource>(std::clamp(v, 0, 6)));
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/blackLevel")) {
            setBlackLevel(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/brightness")) {
            setBrightness(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/gamma")) {
            setGamma(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/stepSize")) {
            setStepSize(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/opacity")) {
            setOpacity(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/source")) {
            setSourceIndex(ev.payload.toInt());
        }
    }

Q_SIGNALS:
    void blackLevelChanged(int v);
    void brightnessChanged(int v);
    void gammaChanged(int v);
    void stepSizeChanged(int v);
    void opacityChanged(int v);
    void sourceChanged();
    void sourceIndexChanged(int v);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/blackLevel"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/brightness"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/gamma"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/stepSize"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/opacity"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/source"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        GpuTextureHandle out = LevelImageOperateGpu::run(
            inputFrame.texture,
            m_blackLevel,
            m_brightness,
            m_gamma,
            m_stepSize,
            m_opacity,
            static_cast<int>(m_source));
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
    int m_blackLevel = 0;
    int m_brightness = 0;
    int m_gamma = 100;
    int m_stepSize = 0;
    int m_opacity = 255;
    LevelSource m_source = LevelSource::Luminance;
};

} // namespace Nodes
