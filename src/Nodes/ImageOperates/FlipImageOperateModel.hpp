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
/**
 * @brief 图像翻转节点，类似 TD 的 Flip TOP
 */
class FlipImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(bool horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged)
    Q_PROPERTY(bool vertical READ vertical WRITE setVertical NOTIFY verticalChanged)

public:
    FlipImageOperateModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Flip";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "horizontal";
            AbstractDelegateModel::registerExternalBinding("/horizontal", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "vertical";
            AbstractDelegateModel::registerExternalBinding("/vertical", this, binding);
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

    ~FlipImageOperateModel() override
    {
        GlobalEventBus::instance()->unsubscribe(this);
    }

    bool horizontal() const { return m_horizontal; }
    bool vertical() const { return m_vertical; }

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
            case 1: return "H";
            case 2: return "V";
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
                setHorizontal(variable->value().toBool());
            } else if (!data) {
                setHorizontal(false);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setVertical(variable->value().toBool());
            } else if (!data) {
                setVertical(false);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["horizontal"] = m_horizontal;
        values["vertical"] = m_vertical;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setHorizontal(values.value("horizontal").toBool());
        setVertical(values.value("vertical").toBool());
    }

public slots:
    void setHorizontal(bool value)
    {
        if (m_horizontal == value) {
            return;
        }
        m_horizontal = value;
        m_tick.markParamsDirty();
        Q_EMIT horizontalChanged(value);
    }

    void setVertical(bool value)
    {
        if (m_vertical == value) {
            return;
        }
        m_vertical = value;
        m_tick.markParamsDirty();
        Q_EMIT verticalChanged(value);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        if (ev.address == makeFullOscAddress("/horizontal")) {
            setHorizontal(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/vertical")) {
            setVertical(ev.payload.toBool());
        }
    }

signals:
    void horizontalChanged(bool value);
    void verticalChanged(bool value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/horizontal"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/vertical"), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        const bool horizontal = m_horizontal;
        const bool vertical = m_vertical;
        const int flipCode = horizontal && vertical ? -1 : (vertical ? 0 : 1);

        if (!horizontal && !vertical) {
            m_worker.submit(
                [input = inputFrame.image.clone()]() { return input; },
                outputTimestamp,
                inputFrame.timestamp);
            return;
        }

        m_worker.submit(
            [input = inputFrame.image.clone(), flipCode]() {
                cv::Mat output;
                cv::flip(input, output, flipCode);
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
    bool m_horizontal = true;
    bool m_vertical = false;
};
} // namespace Nodes
