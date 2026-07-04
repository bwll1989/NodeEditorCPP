#pragma once

/**
 * @file UnderImageOperateModel.hpp
 * @brief Image Under — 遮罩叠加（类似 TD Under TOP）
 *
 * 输入 A：遮罩；输入 B：被遮挡图像。遮罩指定通道越高，B 越不可见。
 * vis = (255 − mask) / 255，输出 B.rgb * vis。
 */

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

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
namespace UnderImageOperateGpu
{
static const char kFragUnder[] = R"(
uniform sampler2D uTextureMask;
uniform sampler2D uTextureImage;
uniform int uMaskChannel;
varying vec2 vTexCoord;
float pickChannel(vec4 c, int ch) {
    if (ch == 0) return c.b;
    if (ch == 1) return c.g;
    if (ch == 2) return c.r;
    return c.a;
}
void main() {
    float mask = pickChannel(texture2D(uTextureMask, vTexCoord), uMaskChannel) * 255.0;
    vec4 image = texture2D(uTextureImage, vTexCoord);
    float vis = clamp((255.0 - mask) / 255.0, 0.0, 1.0);
    gl_FragColor = vec4(image.rgb * vis, image.a * vis);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& maskSrc,
                            const GpuTextureHandle& imageSrc,
                            int maskChannel)
{
    if (!maskSrc.valid() || !imageSrc.valid()) {
        return {};
    }
    const int ch = std::clamp(maskChannel, 0, 3);
    const GpuTextureHandle imageMatched =
        ImageOperateHelpers::matchTextureSize(imageSrc, maskSrc.width, maskSrc.height);
    if (!imageMatched.valid()) {
        return {};
    }
    return ImageGpuPass::instance().runFragmentPass(
        maskSrc.width,
        maskSrc.height,
        kFragUnder,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTextureMask", 0);
            program.setUniformValue("uTextureImage", 1);
            program.setUniformValue("uMaskChannel", ch);
        },
        [&](QOpenGLFunctions* f) {
            ImageGpuPass::bindTexture(f, 0, maskSrc.textureId);
            ImageGpuPass::bindTexture(f, 1, imageMatched.textureId);
        });
}
} // namespace UnderImageOperateGpu

/**
 * @brief 图像遮罩合成：输入1(A) 为遮罩，输入2(B) 为被遮挡图像；遮罩值越高 B 越不可见
 */
class UnderImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT

    /** OpenCV BGRA 通道索引：Blue=0, Green=1, Red=2, Alpha=3 */
    enum class MaskChannel : int {
        Blue = 0,
        Green = 1,
        Red = 2,
        Alpha = 3,
    };
    Q_ENUM(MaskChannel)

    Q_PROPERTY(MaskChannel maskChannel READ maskChannel WRITE setMaskChannel NOTIFY maskChannelChanged)
    Q_PROPERTY(int maskChannelIndex READ maskChannelIndex WRITE setMaskChannelIndex NOTIFY maskChannelIndexChanged DESIGNABLE false)

public:
    UnderImageOperateModel()
    {
        InPortCount = 2;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Under";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "maskChannelIndex";
            AbstractDelegateModel::registerExternalBinding("/maskChannel", this, binding);
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

    ~UnderImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    MaskChannel maskChannel() const { return m_maskChannel; }
    int maskChannelIndex() const { return static_cast<int>(m_maskChannel); }

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
            case 0: return "MASK";
            case 1: return "IMAGE";
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

    QJsonObject save() const override
    {
        QJsonObject values;
        values["maskChannel"] = static_cast<int>(m_maskChannel);

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setMaskChannel(static_cast<MaskChannel>(values.value("maskChannel").toInt(static_cast<int>(MaskChannel::Alpha))));
    }

public slots:
    void setMaskChannel(MaskChannel value)
    {
        if (m_maskChannel == value) {
            return;
        }
        m_maskChannel = value;
        m_paramsDirty = true;
        Q_EMIT maskChannelChanged(value);
        Q_EMIT maskChannelIndexChanged(maskChannelIndex());
    }

    void setMaskChannelIndex(int value)
    {
        setMaskChannel(static_cast<MaskChannel>(std::clamp(value, 0, 3)));
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/maskChannel")) {
            setMaskChannelIndex(ev.payload.toInt());
        }
    }

signals:
    void maskChannelChanged(MaskChannel value);
    void maskChannelIndexChanged(int value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/maskChannel"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        GpuTextureHandle out = UnderImageOperateGpu::run(
            inputAFrame.texture, inputBFrame.texture, static_cast<int>(m_maskChannel));
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
    MaskChannel m_maskChannel = MaskChannel::Alpha;
};
} // namespace Nodes
