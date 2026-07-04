#pragma once

/**
 * @file ImageThresholdDataModel.hpp
 * @brief Image Threshold — 阈值抠像（类似 TD Threshold TOP）
 *
 * 按选定通道（亮度 LUM 或 R/G/B）与阈值比较：
 * - 满足比较条件：保留原 RGB，alpha = 1
 * - 不满足：保留原 RGB，alpha = 0
 *
 * 阈值参数 0–255，shader 内归一化到 [0,1] 再与纹理通道比较（对齐 TD）。
 *
 * Comparator / Source 语义对齐 TouchDesigner Threshold TOP。
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
namespace ImageThresholdGpu
{
static const char kFragThreshold[] = R"(
uniform sampler2D uTexture;
uniform float uThresh;
uniform int uComparator;
uniform int uSource;
varying vec2 vTexCoord;

float channelValue(vec4 c, int source) {
    vec3 bgr = vec3(c.b, c.g, c.r);
    if (source == 0) {
        return dot(bgr, vec3(0.114, 0.587, 0.299));
    }
    if (source == 1) {
        return bgr.r;
    }
    if (source == 2) {
        return bgr.g;
    }
    return bgr.b;
}

bool compareValue(float value, float thresh, int comparator) {
    if (comparator == 0) {
        return value < thresh;
    }
    if (comparator == 1) {
        return value > thresh;
    }
    if (comparator == 2) {
        return value <= thresh;
    }
    if (comparator == 3) {
        return value >= thresh;
    }
    if (comparator == 4) {
        return abs(value - thresh) < 0.001;
    }
    return abs(value - thresh) >= 0.001;
}

void main() {
    vec4 c = texture2D(uTexture, vTexCoord);
    float v = channelValue(c, uSource);
    bool pass = compareValue(v, uThresh, uComparator);
    gl_FragColor = vec4(c.rgb, pass ? 1.0 : 0.0);
}
)";

inline GpuTextureHandle run(const GpuTextureHandle& src, int thresh, int comparator, int source)
{
    if (!src.valid()) {
        return {};
    }
    const int comp = std::clamp(comparator, 0, 5);
    const int srcMode = std::clamp(source, 0, 3);
    return ImageGpuPass::instance().runFragmentPass(
        src.width,
        src.height,
        kFragThreshold,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uThresh", static_cast<float>(thresh) / 255.0f);
            program.setUniformValue("uComparator", comp);
            program.setUniformValue("uSource", srcMode);
        },
        [&](QOpenGLFunctions* f) { ImageGpuPass::bindTexture(f, 0, src.textureId); });
}
} // namespace ImageThresholdGpu

/** @brief 阈值抠像 — 单输入 GPU 算子（LUM/RGB + Comparator） */
class ImageThresholdDataModel final : public AbstractDelegateModel
{
    Q_OBJECT

    /** 比较通道，对齐 TD Threshold TOP → RGB */
    enum class ThresholdSource : int {
        Luminance = 0,
        Red = 1,
        Green = 2,
        Blue = 3
    };
    Q_ENUM(ThresholdSource)

    /** 比较方式，对齐 TD Threshold TOP → Comparator */
    enum class ThresholdComparator : int {
        Less = 0,
        Greater = 1,
        LessOrEqual = 2,
        GreaterOrEqual = 3,
        Equal = 4,
        NotEqual = 5
    };
    Q_ENUM(ThresholdComparator)

    Q_PROPERTY(int thresh READ thresh WRITE setThresh NOTIFY threshChanged)
    Q_PROPERTY(ThresholdComparator comparator READ comparator WRITE setComparator NOTIFY comparatorChanged)
    Q_PROPERTY(int comparatorIndex READ comparatorIndex WRITE setComparatorIndex NOTIFY comparatorIndexChanged DESIGNABLE false)
    Q_PROPERTY(ThresholdSource source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int sourceIndex READ sourceIndex WRITE setSourceIndex NOTIFY sourceIndexChanged DESIGNABLE false)

public:
    ImageThresholdDataModel()
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Threshold";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "thresh";
            AbstractDelegateModel::registerExternalBinding("/thresh", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "comparatorIndex";
            AbstractDelegateModel::registerExternalBinding("/comparator", this, b);
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

    ~ImageThresholdDataModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    int thresh() const { return m_thresh; }
    ThresholdComparator comparator() const { return m_comparator; }
    int comparatorIndex() const { return static_cast<int>(m_comparator); }
    ThresholdSource source() const { return m_source; }
    int sourceIndex() const { return static_cast<int>(m_source); }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        switch (portType) {
        case PortType::In:
            switch (portIndex) {
            case 0: return "IMAGE";
            case 1: return "THRESHOLD";
            case 2: return "COMPARATOR";
            case 3: return "SOURCE";
            default: return QString("INPUT %1").arg(portIndex);
            }
        case PortType::Out:
            return "IMAGE";
        default:
            return QString();
        }
    }

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
                setThresh(variable->value().toInt());
            } else if (!data) {
                setThresh(128);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setComparatorIndex(variable->value().toInt());
            } else if (!data) {
                setComparatorIndex(static_cast<int>(ThresholdComparator::GreaterOrEqual));
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setSourceIndex(variable->value().toInt());
            } else if (!data) {
                setSourceIndex(static_cast<int>(ThresholdSource::Luminance));
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["thresh"] = m_thresh;
        values["comparator"] = static_cast<int>(m_comparator);
        values["source"] = static_cast<int>(m_source);

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        const QJsonObject values = p["values"].toObject();
        setThresh(values.value("thresh").toInt(128));
        setComparatorIndex(values.value("comparator").toInt(static_cast<int>(ThresholdComparator::GreaterOrEqual)));
        setSourceIndex(values.value("source").toInt(static_cast<int>(ThresholdSource::Luminance)));
    }

public Q_SLOTS:
    void setThresh(int v)
    {
        const int clamped = std::clamp(v, 0, 255);
        if (m_thresh == clamped) {
            return;
        }
        m_thresh = clamped;
        m_paramsDirty = true;
        Q_EMIT threshChanged(m_thresh);
    }

    void setComparator(ThresholdComparator v)
    {
        if (m_comparator == v) {
            return;
        }
        m_comparator = v;
        m_paramsDirty = true;
        Q_EMIT comparatorChanged();
        Q_EMIT comparatorIndexChanged(comparatorIndex());
    }

    void setComparatorIndex(int v)
    {
        setComparator(static_cast<ThresholdComparator>(std::clamp(v, 0, 5)));
    }

    void setSource(ThresholdSource v)
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
        setSource(static_cast<ThresholdSource>(std::clamp(v, 0, 3)));
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/thresh")) {
            setThresh(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/comparator")) {
            setComparatorIndex(ev.payload.toInt());
        } else if (ev.address == makeFullOscAddress("/source")) {
            setSourceIndex(ev.payload.toInt());
        }
    }

Q_SIGNALS:
    void threshChanged(int v);
    void comparatorChanged();
    void comparatorIndexChanged(int v);
    void sourceChanged();
    void sourceIndexChanged(int v);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/thresh"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/comparator"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        GpuTextureHandle out = ImageThresholdGpu::run(
            inputFrame.texture,
            m_thresh,
            static_cast<int>(m_comparator),
            static_cast<int>(m_source));
        ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp);

        m_lastProcessedInputTimestamp = inputFrame.timestamp;
        m_paramsDirty = false;
        Q_EMIT dataUpdated(0);
    }

    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageData> m_inImage;
    qint64 m_lastRequestedFrame = -1;
    qint64 m_lastProcessedInputTimestamp = -1;
    qint64 m_lastPushedTimestamp = -1;
    bool m_paramsDirty = false;
    int m_thresh = 128;
    ThresholdComparator m_comparator = ThresholdComparator::GreaterOrEqual;
    ThresholdSource m_source = ThresholdSource::Luminance;
};

} // namespace Nodes
