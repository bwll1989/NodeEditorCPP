#pragma once

/**
 * @file CrossImageOperateModel.hpp
 * @brief Image Cross — 双图线性混合 mix(A, B, blend)
 *
 * A 决定输出尺寸；B 经 matchTextureSize 对齐。blend∈[0,1] 在 GPU 端 mix。
 * 节点 tick / setInData 约定同 Doc.md §3。
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
namespace CrossImageOperateGpu
{
static const char kFragCross[] = R"(
uniform sampler2D uTextureA;
uniform sampler2D uTextureB;
uniform float uBlend;
varying vec2 vTexCoord;
void main() {
    vec4 a = texture2D(uTextureA, vTexCoord);
    vec4 b = texture2D(uTextureB, vTexCoord);
    gl_FragColor = mix(a, b, uBlend);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& a, const GpuTextureHandle& b, double blend)
{
    if (!a.valid()) {
        return {};
    }
    if (blend <= 0.0) {
        return ImageGpuPass::instance().resize(a, a.width, a.height);
    }
    if (!b.valid() || blend >= 1.0) {
        return ImageGpuPass::instance().resize(b, a.width, a.height);
    }

    const GpuTextureHandle bMatched = ImageOperateHelpers::matchTextureSize(b, a.width, a.height);
    if (!bMatched.valid()) {
        return {};
    }
    return ImageGpuPass::instance().runFragmentPass(
        a.width,
        a.height,
        kFragCross,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTextureA", 0);
            program.setUniformValue("uTextureB", 1);
            program.setUniformValue("uBlend", static_cast<float>(blend));
        },
        [&](QOpenGLFunctions* f) {
            ImageGpuPass::bindTexture(f, 0, a.textureId);
            ImageGpuPass::bindTexture(f, 1, bMatched.textureId);
        });
}
} // namespace CrossImageOperateGpu

/** @brief 双路交叉混合 — 双输入 GPU 算子 */
class CrossImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double blend READ blend WRITE setBlend NOTIFY blendChanged)

public:
    CrossImageOperateModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Cross";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        NodeDelegateModel::ExternalBinding binding;
        binding.member = "blend";
        AbstractDelegateModel::registerExternalBinding("/blend", this, binding);

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

    ~CrossImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    double blend() const { return m_blend; }

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
            case 2: return "BLEND";
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
                setBlend(variable->value().toDouble());
            } else if (!data) {
                setBlend(0.0);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["blend"] = m_blend;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setBlend(values.value("blend").toDouble());
    }

public slots:
    void setBlend(double value)
    {
        const double clamped = std::clamp(value, 0.0, 1.0);
        if (qFuzzyCompare(m_blend, clamped)) {
            return;
        }
        m_blend = clamped;
        m_paramsDirty = true;
        Q_EMIT blendChanged(m_blend);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        if (ev.address == makeFullOscAddress("/blend")) {
            setBlend(ev.payload.toDouble());
        }
    }

signals:
    void blendChanged(double value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/blend"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        GpuTextureHandle out = CrossImageOperateGpu::run(
            inputAFrame.texture, inputBFrame.texture, m_blend);
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
    double m_blend = 0.0;
};
} // namespace Nodes
