#pragma once

/**
 * @file DifferenceImageOperateModel.hpp
 * @brief Image Difference — 双路绝对差 |A−B|×gain
 *
 * 双输入算子；B 对齐到 A 尺寸。gain≥1，结果 clamp 到 [0,1]。
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
namespace DifferenceImageOperateGpu
{
static const char kFragDifference[] = R"(
uniform sampler2D uTextureA;
uniform sampler2D uTextureB;
uniform float uGain;
varying vec2 vTexCoord;
void main() {
    vec3 a = texture2D(uTextureA, vTexCoord).rgb;
    vec3 b = texture2D(uTextureB, vTexCoord).rgb;
    vec3 d = abs(a - b) * uGain;
    gl_FragColor = vec4(clamp(d, 0.0, 1.0), 1.0);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& a, const GpuTextureHandle& b, int gain)
{
    if (!a.valid() || !b.valid()) {
        return {};
    }
    const float g = static_cast<float>(std::max(1, gain));
    const GpuTextureHandle bMatched = ImageOperateHelpers::matchTextureSize(b, a.width, a.height);
    if (!bMatched.valid()) {
        return {};
    }
    return ImageGpuPass::instance().runFragmentPass(
        a.width,
        a.height,
        kFragDifference,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTextureA", 0);
            program.setUniformValue("uTextureB", 1);
            program.setUniformValue("uGain", g);
        },
        [&](QOpenGLFunctions* f) {
            ImageGpuPass::bindTexture(f, 0, a.textureId);
            ImageGpuPass::bindTexture(f, 1, bMatched.textureId);
        });
}
} // namespace DifferenceImageOperateGpu

/** @brief 差异图 — 双输入 GPU 算子 */
class DifferenceImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)

public:
    DifferenceImageOperateModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Difference";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        NodeDelegateModel::ExternalBinding binding;
        binding.member = "gain";
        AbstractDelegateModel::registerExternalBinding("/gain", this, binding);

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

    ~DifferenceImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    int gain() const { return m_gain; }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            return portIndex == 2 ? VariableData().type() : ImageData().type();
        }
        return ImageData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case 0: return "A";
            case 1: return "B";
            case 2: return "GAIN";
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
            m_inputA = std::dynamic_pointer_cast<ImageData>(data);
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            m_lastProcessedInputTimestampA = -1;
            m_lastProcessedInputTimestampB = -1;
            m_paramsDirty = true;
            Q_EMIT dataUpdated(0);
            break;
        case 1:
            m_inputB = std::dynamic_pointer_cast<ImageData>(data);
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            m_lastProcessedInputTimestampA = -1;
            m_lastProcessedInputTimestampB = -1;
            m_paramsDirty = true;
            Q_EMIT dataUpdated(0);
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setGain(variable->value().toInt());
            } else if (!data) {
                setGain(1);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["gain"] = m_gain;
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        setGain(data["values"].toObject().value("gain").toInt(1));
    }

public slots:
    void setGain(int value)
    {
        const int clamped = std::clamp(value, 1, 16);
        if (m_gain == clamped) {
            return;
        }
        m_gain = clamped;
        m_paramsDirty = true;
        Q_EMIT gainChanged(m_gain);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/gain")) {
            setGain(ev.payload.toInt());
        }
    }

signals:
    void gainChanged(int value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/gain"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    void clearOutput()
    {
        if (m_outBuffer) {
            m_outBuffer->clear();
        }
        m_lastPushedTimestamp = -1;
        m_lastProcessedInputTimestampA = -1;
        m_lastProcessedInputTimestampB = -1;
        m_paramsDirty = false;
    }

    void requestProcess(qint64 targetTimestamp = -1)
    {
        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
        const qint64 lookupTimestamp = ImageOperateHelpers::normalizeTargetTimestamp(targetTimestamp);

        if (!ImageOperateHelpers::hasInputImage(m_inputA) || !ImageOperateHelpers::hasInputImage(m_inputB)) {
            clearOutput();
            return;
        }

        ImageFrame inputAFrame;
        ImageFrame inputBFrame;
        if (!ImageOperateHelpers::resolveDualInputGpuFrames(
                m_inputA, m_inputB, lookupTimestamp, inputAFrame, inputBFrame)) {
            clearOutput();
            return;
        }

        if (!m_paramsDirty
            && inputAFrame.timestamp == m_lastProcessedInputTimestampA
            && inputBFrame.timestamp == m_lastProcessedInputTimestampB) {
            return;
        }

        GpuTextureHandle out = DifferenceImageOperateGpu::run(
            inputAFrame.texture, inputBFrame.texture, m_gain);
        ImageOperateHelpers::pushGpuResultDual(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestampA = inputAFrame.timestamp;
        m_lastProcessedInputTimestampB = inputBFrame.timestamp;
        m_paramsDirty = false;
    }

    std::shared_ptr<ImageData> m_inputA;
    std::shared_ptr<ImageData> m_inputB;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestampA = -1;
    qint64 m_lastProcessedInputTimestampB = -1;
    qint64 m_lastPushedTimestamp = -1;
    bool m_paramsDirty = false;
    int m_gain = 1;
};
} // namespace Nodes
