#pragma once

/**
 * @file ChromaKeyImageOperateModel.hpp
 * @brief Image Chroma Key — 色相区间抠像（输出 BGRA）
 *
 * Shader 在 RGB→Hue 空间判断 keyAlpha，支持 hue 环绕与 soft 过渡带。
 * 输入 alpha 与抠像 alpha 相乘。
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>

#include <algorithm>

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace ChromaKeyImageOperateGpu
{
static const char kFragChromaKey[] = R"(
uniform sampler2D uTexture;
uniform float uHueMin;
uniform float uHueMax;
uniform float uSoftLow;
uniform float uSoftHigh;
varying vec2 vTexCoord;

float rgb2hue(vec3 bgr) {
    float maxc = max(max(bgr.r, bgr.g), bgr.b);
    float minc = min(min(bgr.r, bgr.g), bgr.b);
    if (maxc - minc < 0.00001) return 0.0;
    float h;
    if (maxc == bgr.r) h = mod((bgr.g - bgr.b) / (maxc - minc), 6.0);
    else if (maxc == bgr.g) h = (bgr.b - bgr.r) / (maxc - minc) + 2.0;
    else h = (bgr.r - bgr.g) / (maxc - minc) + 4.0;
    h *= 60.0;
    if (h < 0.0) h += 360.0;
    return h;
}

float keyAlpha(float hueDeg) {
    float intervalMax = uHueMax;
    if (intervalMax < uHueMin) intervalMax += 360.0;
    if ((intervalMax - uHueMin) >= 360.0) return 0.0;
    float alpha = 1.0;
    for (int k = -1; k <= 1; ++k) {
        float candidate = hueDeg + float(k) * 360.0;
        if (candidate >= uHueMin && candidate <= intervalMax) return 0.0;
        if (candidate < uHueMin && uSoftLow > 0.0 && candidate >= uHueMin - uSoftLow) {
            alpha = min(alpha, (uHueMin - candidate) / uSoftLow);
        } else if (candidate > intervalMax && uSoftHigh > 0.0 && candidate <= intervalMax + uSoftHigh) {
            alpha = min(alpha, (candidate - intervalMax) / uSoftHigh);
        }
    }
    return alpha;
}

void main() {
    vec4 c = texture2D(uTexture, vTexCoord);
    vec3 bgr = vec3(c.b, c.g, c.r);
    float hue = rgb2hue(bgr);
    float a = keyAlpha(hue);
    gl_FragColor = vec4(c.rgb, c.a * a);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& src,
                            double hueMin,
                            double hueMax,
                            double softLow,
                            double softHigh)
{
    if (!src.valid()) {
        return {};
    }
    return ImageGpuPass::instance().runFragmentPass(
        src.width,
        src.height,
        kFragChromaKey,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uHueMin", static_cast<float>(hueMin));
            program.setUniformValue("uHueMax", static_cast<float>(hueMax));
            program.setUniformValue("uSoftLow", static_cast<float>(softLow));
            program.setUniformValue("uSoftHigh", static_cast<float>(softHigh));
        },
        [&](QOpenGLFunctions* f) { ImageGpuPass::bindTexture(f, 0, src.textureId); });
}
} // namespace ChromaKeyImageOperateGpu

/** @brief 色相抠像 — 单输入 GPU 算子，输出带 alpha */
class ChromaKeyImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double hueMin READ hueMin WRITE setHueMin NOTIFY hueMinChanged)
    Q_PROPERTY(double hueMax READ hueMax WRITE setHueMax NOTIFY hueMaxChanged)
    Q_PROPERTY(double hueSoftLow READ hueSoftLow WRITE setHueSoftLow NOTIFY hueSoftLowChanged)
    Q_PROPERTY(double hueSoftHigh READ hueSoftHigh WRITE setHueSoftHigh NOTIFY hueSoftHighChanged)

public:
    ChromaKeyImageOperateModel()
    {
        InPortCount = 5;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Chroma Key";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueMin";
            AbstractDelegateModel::registerExternalBinding("/huemin", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueMax";
            AbstractDelegateModel::registerExternalBinding("/huemax", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueSoftLow";
            AbstractDelegateModel::registerExternalBinding("/hsoftlow", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueSoftHigh";
            AbstractDelegateModel::registerExternalBinding("/hsofthigh", this, binding);
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

    ~ChromaKeyImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    double hueMin() const { return m_hueMin; }
    double hueMax() const { return m_hueMax; }
    double hueSoftLow() const { return m_hueSoftLow; }
    double hueSoftHigh() const { return m_hueSoftHigh; }

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
            case 1: return "HUE MIN";
            case 2: return "HUE MAX";
            case 3: return "SOFT LOW";
            case 4: return "SOFT HIGH";
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
                setHueMin(variable->asNumber());
            } else if (!data) {
                setHueMin(0.0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHueMax(variable->asNumber());
            } else if (!data) {
                setHueMax(120.0);
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHueSoftLow(variable->asNumber());
            } else if (!data) {
                setHueSoftLow(0.0);
            }
            break;
        case 4:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHueSoftHigh(variable->asNumber());
            } else if (!data) {
                setHueSoftHigh(0.0);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["hueMin"] = m_hueMin;
        values["hueMax"] = m_hueMax;
        values["hueSoftLow"] = m_hueSoftLow;
        values["hueSoftHigh"] = m_hueSoftHigh;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setHueMin(values.value("hueMin").toDouble(0.0));
        setHueMax(values.value("hueMax").toDouble(120.0));
        setHueSoftLow(values.value("hueSoftLow").toDouble(0.0));
        setHueSoftHigh(values.value("hueSoftHigh").toDouble(0.0));
    }

public slots:
    void setHueMin(double value)
    {
        const double clamped = clampHue(value);
        if (qFuzzyCompare(m_hueMin + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueMin = clamped;
        m_paramsDirty = true;
        Q_EMIT hueMinChanged(m_hueMin);
    }

    void setHueMax(double value)
    {
        const double clamped = clampHue(value);
        if (qFuzzyCompare(m_hueMax + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueMax = clamped;
        m_paramsDirty = true;
        Q_EMIT hueMaxChanged(m_hueMax);
    }

    void setHueSoftLow(double value)
    {
        const double clamped = clampSoftness(value);
        if (qFuzzyCompare(m_hueSoftLow + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueSoftLow = clamped;
        m_paramsDirty = true;
        Q_EMIT hueSoftLowChanged(m_hueSoftLow);
    }

    void setHueSoftHigh(double value)
    {
        const double clamped = clampSoftness(value);
        if (qFuzzyCompare(m_hueSoftHigh + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueSoftHigh = clamped;
        m_paramsDirty = true;
        Q_EMIT hueSoftHighChanged(m_hueSoftHigh);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        if (ev.address == makeFullOscAddress("/huemin")) {
            setHueMin(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/huemax")) {
            setHueMax(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/hsoftlow")) {
            setHueSoftLow(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/hsofthigh")) {
            setHueSoftHigh(ev.payload.toDouble());
        }
    }

signals:
    void hueMinChanged(double value);
    void hueMaxChanged(double value);
    void hueSoftLowChanged(double value);
    void hueSoftHighChanged(double value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/huemin"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/huemax"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/hsoftlow"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/hsofthigh"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    static double clampHue(double value)
    {
        return std::clamp(value, 0.0, 360.0);
    }

    static double clampSoftness(double value)
    {
        return std::clamp(value, 0.0, 360.0);
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

        GpuTextureHandle out = ChromaKeyImageOperateGpu::run(
            inputFrame.texture, m_hueMin, m_hueMax, m_hueSoftLow, m_hueSoftHigh);
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
    double m_hueMin = 80.0;
    double m_hueMax = 160.0;
    double m_hueSoftLow = 10.0;
    double m_hueSoftHigh = 10.0;
};
} // namespace Nodes
