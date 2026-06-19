#pragma once

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include <opencv2/imgproc.hpp>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
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
        m_worker.setParent(this);
        m_worker.setFinishedCallback([this](cv::Mat&& image, qint64 outputTimestamp, qint64 inputTimestamp, std::uint64_t) {
            ImageOperateHelpers::pushWorkerResult(
                m_outBuffer, std::move(image), outputTimestamp, m_lastPushedTimestamp, m_tick, inputTimestamp);
        });

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64 frameCount) {
                    if (!ImageOperateHelpers::usesSharedImageBuffer(m_inImage)) {
                        return;
                    }
                    if (!m_tick.beginFrameTick(frameCount)) {
                        return;
                    }
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
            if (m_inImage) {
                m_tick.markInputConnected();
                Q_EMIT dataUpdated(0);
                if (!ImageOperateHelpers::usesSharedImageBuffer(m_inImage)) {
                    requestProcess();
                }
            }
            break;
        case 1:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setRadiusX(variable->value().toInt());
            } else if (!data) {
                setRadiusX(0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setRadiusY(variable->value().toInt());
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
        m_tick.markParamsDirty();
        Q_EMIT radiusXChanged(m_radiusX);
    }

    void setRadiusY(int value)
    {
        const int clamped = std::clamp(value, 0, 32);
        if (m_radiusY == clamped) {
            return;
        }
        m_radiusY = clamped;
        m_tick.markParamsDirty();
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

    void requestProcess(qint64 targetTimestamp = -1)
    {
        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
        const qint64 outputTimestamp = ImageOperateHelpers::normalizeTargetTimestamp(targetTimestamp);

        if (imageDataIsEmpty(m_inImage)) {
            m_worker.cancelPending();
            if (m_outBuffer) {
                m_outBuffer->clear();
            }
            m_lastPushedTimestamp = -1;
            m_tick.resetOutputState();
            return;
        }

        ImageFrame inputFrame;
        if (!ImageOperateHelpers::resolveImageFrameAtTimestamp(m_inImage, outputTimestamp, inputFrame) ||
            inputFrame.image.empty()) {
            m_worker.cancelPending();
            if (m_outBuffer) {
                m_outBuffer->clear();
            }
            m_lastPushedTimestamp = -1;
            return;
        }

        if (!m_tick.shouldProcess(inputFrame.timestamp)) {
            return;
        }

        const int radiusX = m_radiusX;
        const int radiusY = m_radiusY;

        if (radiusX == 0 && radiusY == 0) {
            m_worker.submit(
                [input = inputFrame.image.clone()]() { return input; },
                outputTimestamp,
                inputFrame.timestamp);
            return;
        }

        m_worker.submit(
            [input = inputFrame.image.clone(), radiusX, radiusY]() {
                if (input.empty()) {
                    return cv::Mat();
                }
                const int kernelWidth = std::max(1, radiusX * 2 + 1);
                const int kernelHeight = std::max(1, radiusY * 2 + 1);
                cv::Mat output;
                cv::GaussianBlur(input, output, cv::Size(kernelWidth, kernelHeight), 0.0, 0.0, cv::BORDER_REPLICATE);
                return output;
            },
            outputTimestamp,
            inputFrame.timestamp);
    }

    std::shared_ptr<ImageData> m_inImage;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    ImageOperateHelpers::ImageOperateWorkerQueue m_worker;
    ImageOperateHelpers::ImageOperateTickState m_tick;
    qint64 m_lastPushedTimestamp = -1;
    int m_radiusX = 8;
    int m_radiusY = 8;
};
} // namespace Nodes
