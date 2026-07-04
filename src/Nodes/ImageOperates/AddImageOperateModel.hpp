#pragma once

/**
 * @file AddImageOperateModel.hpp
 * @brief Image Add — 双路像素相加 clamp(A+B, 0, 1)，类似 TD Add TOP
 *
 * A 决定输出尺寸；B 经 matchTextureSize 对齐。
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include "Common/BaseClass/AbstractDelegateModel.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
namespace AddImageOperateGpu
{
static const char kFragAdd[] = R"(
uniform sampler2D uTextureA;
uniform sampler2D uTextureB;
varying vec2 vTexCoord;
void main() {
    vec3 a = texture2D(uTextureA, vTexCoord).rgb;
    vec3 b = texture2D(uTextureB, vTexCoord).rgb;
    gl_FragColor = vec4(clamp(a + b, 0.0, 1.0), 1.0);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& a, const GpuTextureHandle& b)
{
    if (!a.valid() || !b.valid()) {
        return {};
    }
    const GpuTextureHandle bMatched = ImageOperateHelpers::matchTextureSize(b, a.width, a.height);
    if (!bMatched.valid()) {
        return {};
    }
    return ImageGpuPass::instance().runFragmentPass(
        a.width,
        a.height,
        kFragAdd,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTextureA", 0);
            program.setUniformValue("uTextureB", 1);
        },
        [&](QOpenGLFunctions* f) {
            ImageGpuPass::bindTexture(f, 0, a.textureId);
            ImageGpuPass::bindTexture(f, 1, bMatched.textureId);
        });
}
} // namespace AddImageOperateGpu

/** @brief 双路像素相加 — 双输入 GPU 算子 */
class AddImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    AddImageOperateModel()
    {
        InPortCount = 2;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Add";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

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

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex);
        if (portType == PortType::In || portType == PortType::Out) {
            return ImageData().type();
        }
        return ImageData().type();
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::In) {
            switch (portIndex) {
            case 0: return "A";
            case 1: return "B";
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
        default:
            break;
        }
    }

    QJsonObject save() const override { return NodeDelegateModel::save(); }

    void load(const QJsonObject& data) override { Q_UNUSED(data); }

private:
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

        GpuTextureHandle out = AddImageOperateGpu::run(inputAFrame.texture, inputBFrame.texture);
        ImageOperateHelpers::pushGpuResultDual(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestampA = inputAFrame.timestamp;
        m_lastProcessedInputTimestampB = inputBFrame.timestamp;
        m_paramsDirty = false;
        Q_EMIT dataUpdated(0);
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
};
} // namespace Nodes
