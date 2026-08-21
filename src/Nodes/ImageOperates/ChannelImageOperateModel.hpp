#pragma once

/**
 * @file ChannelImageOperateModel.hpp
 * @brief Image Channel — 逐通道增益/偏移（Channel Mix TOP 精简版）
 *
 * 仅对各通道独立缩放与加常数，不做 RGBA 交叉混合：
 *   out.ch = clamp(in.ch * gain + offset, 0, 1)
 *
 * 纹理采样 c.r / c.g / c.b / c.a 分别对应图像 R / G / B / A。
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
namespace ChannelImageOperateGpu
{
static const char kFragChannel[] = R"(
uniform sampler2D uTexture;
uniform float uRedGain;
uniform float uGreenGain;
uniform float uBlueGain;
uniform float uAlphaGain;
uniform float uRedOffset;
uniform float uGreenOffset;
uniform float uBlueOffset;
uniform float uAlphaOffset;
varying vec2 vTexCoord;
void main() {
    vec4 c = texture2D(uTexture, vTexCoord);
    // 采样结果 c.r/c.g/c.b 分别对应图像 R/G/B（与 ChromaKey 等节点一致），勿交换输出顺序
    gl_FragColor = vec4(
        clamp(c.r * uRedGain + uRedOffset, 0.0, 1.0),
        clamp(c.g * uGreenGain + uGreenOffset, 0.0, 1.0),
        clamp(c.b * uBlueGain + uBlueOffset, 0.0, 1.0),
        clamp(c.a * uAlphaGain + uAlphaOffset, 0.0, 1.0)
    );
}
)";

struct ChannelParams
{
    int redGain = 100;
    int greenGain = 100;
    int blueGain = 100;
    int alphaGain = 100;
    int redOffset = 0;
    int greenOffset = 0;
    int blueOffset = 0;
    int alphaOffset = 0;
};

inline float gainFrom100(int v)
{
    return static_cast<float>(std::clamp(v, -400, 400)) / 100.0f;
}

inline float offsetNorm(int v)
{
    return static_cast<float>(std::clamp(v, -255, 255)) / 255.0f;
}

inline GpuTextureHandle run(const GpuTextureHandle& src, const ChannelParams& p)
{
    if (!src.valid()) {
        return {};
    }
    return ImageGpuPass::instance().runFragmentPass(
        src.width,
        src.height,
        kFragChannel,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uRedGain", gainFrom100(p.redGain));
            program.setUniformValue("uGreenGain", gainFrom100(p.greenGain));
            program.setUniformValue("uBlueGain", gainFrom100(p.blueGain));
            program.setUniformValue("uAlphaGain", gainFrom100(p.alphaGain));
            program.setUniformValue("uRedOffset", offsetNorm(p.redOffset));
            program.setUniformValue("uGreenOffset", offsetNorm(p.greenOffset));
            program.setUniformValue("uBlueOffset", offsetNorm(p.blueOffset));
            program.setUniformValue("uAlphaOffset", offsetNorm(p.alphaOffset));
        },
        [&](QOpenGLFunctions* f) { ImageGpuPass::bindTexture(f, 0, src.textureId); });
}
} // namespace ChannelImageOperateGpu

/** @brief 逐通道调整 — 单输入 GPU 算子 */
class ChannelImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT

    Q_PROPERTY(int redGain READ redGain WRITE setRedGain NOTIFY redGainChanged)
    Q_PROPERTY(int greenGain READ greenGain WRITE setGreenGain NOTIFY greenGainChanged)
    Q_PROPERTY(int blueGain READ blueGain WRITE setBlueGain NOTIFY blueGainChanged)
    Q_PROPERTY(int alphaGain READ alphaGain WRITE setAlphaGain NOTIFY alphaGainChanged)
    Q_PROPERTY(int redOffset READ redOffset WRITE setRedOffset NOTIFY redOffsetChanged)
    Q_PROPERTY(int greenOffset READ greenOffset WRITE setGreenOffset NOTIFY greenOffsetChanged)
    Q_PROPERTY(int blueOffset READ blueOffset WRITE setBlueOffset NOTIFY blueOffsetChanged)
    Q_PROPERTY(int alphaOffset READ alphaOffset WRITE setAlphaOffset NOTIFY alphaOffsetChanged)

public:
    ChannelImageOperateModel()
    {
        InPortCount = 5;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Channel";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

        registerGainBinding("redGain", "/redGain");
        registerGainBinding("greenGain", "/greenGain");
        registerGainBinding("blueGain", "/blueGain");
        registerGainBinding("alphaGain", "/alphaGain");
        registerGainBinding("redOffset", "/redOffset");
        registerGainBinding("greenOffset", "/greenOffset");
        registerGainBinding("blueOffset", "/blueOffset");
        registerGainBinding("alphaOffset", "/alphaOffset");

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

    ~ChannelImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    int redGain() const { return m_params.redGain; }
    int greenGain() const { return m_params.greenGain; }
    int blueGain() const { return m_params.blueGain; }
    int alphaGain() const { return m_params.alphaGain; }
    int redOffset() const { return m_params.redOffset; }
    int greenOffset() const { return m_params.greenOffset; }
    int blueOffset() const { return m_params.blueOffset; }
    int alphaOffset() const { return m_params.alphaOffset; }

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
            case 1: return "R GAIN";
            case 2: return "G GAIN";
            case 3: return "B GAIN";
            case 4: return "A GAIN";
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
                setRedGain(variable->asNumber());
            } else if (!data) {
                setRedGain(100);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setGreenGain(variable->asNumber());
            } else if (!data) {
                setGreenGain(100);
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setBlueGain(variable->asNumber());
            } else if (!data) {
                setBlueGain(100);
            }
            break;
        case 4:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setAlphaGain(variable->asNumber());
            } else if (!data) {
                setAlphaGain(100);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["redGain"] = m_params.redGain;
        values["greenGain"] = m_params.greenGain;
        values["blueGain"] = m_params.blueGain;
        values["alphaGain"] = m_params.alphaGain;
        values["redOffset"] = m_params.redOffset;
        values["greenOffset"] = m_params.greenOffset;
        values["blueOffset"] = m_params.blueOffset;
        values["alphaOffset"] = m_params.alphaOffset;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        const QJsonObject values = p["values"].toObject();
        setRedGain(values.value("redGain").toInt(100));
        setGreenGain(values.value("greenGain").toInt(100));
        setBlueGain(values.value("blueGain").toInt(100));
        setAlphaGain(values.value("alphaGain").toInt(100));
        setRedOffset(values.value("redOffset").toInt(0));
        setGreenOffset(values.value("greenOffset").toInt(0));
        setBlueOffset(values.value("blueOffset").toInt(0));
        setAlphaOffset(values.value("alphaOffset").toInt(0));
    }

public Q_SLOTS:
    void setRedGain(int v) { setGain(&ChannelImageOperateGpu::ChannelParams::redGain, v, &ChannelImageOperateModel::redGainChanged); }
    void setGreenGain(int v) { setGain(&ChannelImageOperateGpu::ChannelParams::greenGain, v, &ChannelImageOperateModel::greenGainChanged); }
    void setBlueGain(int v) { setGain(&ChannelImageOperateGpu::ChannelParams::blueGain, v, &ChannelImageOperateModel::blueGainChanged); }
    void setAlphaGain(int v) { setGain(&ChannelImageOperateGpu::ChannelParams::alphaGain, v, &ChannelImageOperateModel::alphaGainChanged); }
    void setRedOffset(int v) { setOffset(&ChannelImageOperateGpu::ChannelParams::redOffset, v, &ChannelImageOperateModel::redOffsetChanged); }
    void setGreenOffset(int v) { setOffset(&ChannelImageOperateGpu::ChannelParams::greenOffset, v, &ChannelImageOperateModel::greenOffsetChanged); }
    void setBlueOffset(int v) { setOffset(&ChannelImageOperateGpu::ChannelParams::blueOffset, v, &ChannelImageOperateModel::blueOffsetChanged); }
    void setAlphaOffset(int v) { setOffset(&ChannelImageOperateGpu::ChannelParams::alphaOffset, v, &ChannelImageOperateModel::alphaOffsetChanged); }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/redGain")) {
            setRedGain(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/greenGain")) {
            setGreenGain(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/blueGain")) {
            setBlueGain(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/alphaGain")) {
            setAlphaGain(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/redOffset")) {
            setRedOffset(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/greenOffset")) {
            setGreenOffset(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/blueOffset")) {
            setBlueOffset(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/alphaOffset")) {
            setAlphaOffset(ev.payload.toInt());
        }
    }

Q_SIGNALS:
    void redGainChanged(int v);
    void greenGainChanged(int v);
    void blueGainChanged(int v);
    void alphaGainChanged(int v);
    void redOffsetChanged(int v);
    void greenOffsetChanged(int v);
    void blueOffsetChanged(int v);
    void alphaOffsetChanged(int v);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/redGain"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/greenGain"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/blueGain"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/alphaGain"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/redOffset"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/greenOffset"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/blueOffset"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/alphaOffset"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private:
    void registerGainBinding(const char* member, const char* address)
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = member;
        AbstractDelegateModel::registerExternalBinding(address, this, b);
    }

    void setGain(int ChannelImageOperateGpu::ChannelParams::*field, int v, void (ChannelImageOperateModel::*signal)(int))
    {
        const int clamped = std::clamp(v, -400, 400);
        if (m_params.*field == clamped) {
            return;
        }
        m_params.*field = clamped;
        m_paramsDirty = true;
        Q_EMIT (this->*signal)(clamped);
    }

    void setOffset(int ChannelImageOperateGpu::ChannelParams::*field, int v, void (ChannelImageOperateModel::*signal)(int))
    {
        const int clamped = std::clamp(v, -255, 255);
        if (m_params.*field == clamped) {
            return;
        }
        m_params.*field = clamped;
        m_paramsDirty = true;
        Q_EMIT (this->*signal)(clamped);
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

        GpuTextureHandle out = ChannelImageOperateGpu::run(inputFrame.texture, m_params);
        ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
        Q_EMIT dataUpdated(0);
    }

    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    ChannelImageOperateGpu::ChannelParams m_params;
    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    qint64 m_lastPushedTimestamp = -1;
    bool m_paramsDirty = false;
};

} // namespace Nodes
