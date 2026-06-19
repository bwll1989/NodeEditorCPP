#pragma once

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
class CropImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double leftPercent READ leftPercent WRITE setLeftPercent NOTIFY leftPercentChanged)
    Q_PROPERTY(double rightPercent READ rightPercent WRITE setRightPercent NOTIFY rightPercentChanged)
    Q_PROPERTY(double topPercent READ topPercent WRITE setTopPercent NOTIFY topPercentChanged)
    Q_PROPERTY(double bottomPercent READ bottomPercent WRITE setBottomPercent NOTIFY bottomPercentChanged)

public:
    CropImageOperateModel()
    {
        InPortCount = 5;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Crop";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "leftPercent";
            AbstractDelegateModel::registerExternalBinding("/left", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "rightPercent";
            AbstractDelegateModel::registerExternalBinding("/right", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "topPercent";
            AbstractDelegateModel::registerExternalBinding("/top", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "bottomPercent";
            AbstractDelegateModel::registerExternalBinding("/bottom", this, binding);
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

    ~CropImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    double leftPercent() const { return m_leftPercent; }
    double rightPercent() const { return m_rightPercent; }
    double topPercent() const { return m_topPercent; }
    double bottomPercent() const { return m_bottomPercent; }

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
            case 1: return "LEFT %";
            case 2: return "RIGHT %";
            case 3: return "TOP %";
            case 4: return "BOTTOM %";
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
                setLeftPercent(variable->value().toDouble());
            } else if (!data) {
                setLeftPercent(0.0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setRightPercent(variable->value().toDouble());
            } else if (!data) {
                setRightPercent(0.0);
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setTopPercent(variable->value().toDouble());
            } else if (!data) {
                setTopPercent(0.0);
            }
            break;
        case 4:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setBottomPercent(variable->value().toDouble());
            } else if (!data) {
                setBottomPercent(0.0);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["leftPercent"] = m_leftPercent;
        values["rightPercent"] = m_rightPercent;
        values["topPercent"] = m_topPercent;
        values["bottomPercent"] = m_bottomPercent;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setLeftPercent(values.value("leftPercent").toDouble());
        setRightPercent(values.value("rightPercent").toDouble());
        setTopPercent(values.value("topPercent").toDouble());
        setBottomPercent(values.value("bottomPercent").toDouble());
    }

public slots:
    void setLeftPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_leftPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_leftPercent = clamped;
        m_tick.markParamsDirty();
        Q_EMIT leftPercentChanged(m_leftPercent);
    }

    void setRightPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_rightPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_rightPercent = clamped;
        m_tick.markParamsDirty();
        Q_EMIT rightPercentChanged(m_rightPercent);
    }

    void setTopPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_topPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_topPercent = clamped;
        m_tick.markParamsDirty();
        Q_EMIT topPercentChanged(m_topPercent);
    }

    void setBottomPercent(double value)
    {
        const double clamped = clampPercent(value);
        if (qFuzzyCompare(m_bottomPercent + 1.0, clamped + 1.0)) {
            return;
        }
        m_bottomPercent = clamped;
        m_tick.markParamsDirty();
        Q_EMIT bottomPercentChanged(m_bottomPercent);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        if (ev.address == makeFullOscAddress("/left")) {
            setLeftPercent(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/right")) {
            setRightPercent(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/top")) {
            setTopPercent(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/bottom")) {
            setBottomPercent(ev.payload.toDouble());
        }
    }

signals:
    void leftPercentChanged(double value);
    void rightPercentChanged(double value);
    void topPercentChanged(double value);
    void bottomPercentChanged(double value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/left"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/right"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/top"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/bottom"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    static double clampPercent(double value)
    {
        return std::clamp(value, 0.0, 100.0);
    }

    static int percentToPixels(int size, double percent)
    {
        return static_cast<int>(std::round(static_cast<double>(size) * percent / 100.0));
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

        const double leftPercent = m_leftPercent;
        const double rightPercent = m_rightPercent;
        const double topPercent = m_topPercent;
        const double bottomPercent = m_bottomPercent;

        if (leftPercent <= 0.0 && rightPercent <= 0.0 && topPercent <= 0.0 && bottomPercent <= 0.0) {
            m_worker.submit(
                [input = inputFrame.image.clone()]() { return input; },
                outputTimestamp,
                inputFrame.timestamp);
            return;
        }

        m_worker.submit(
            [input = inputFrame.image.clone(), leftPercent, rightPercent, topPercent, bottomPercent]() {
                const int left = percentToPixels(input.cols, leftPercent);
                const int right = percentToPixels(input.cols, rightPercent);
                const int top = percentToPixels(input.rows, topPercent);
                const int bottom = percentToPixels(input.rows, bottomPercent);

                const int outputWidth = input.cols - left - right;
                const int outputHeight = input.rows - top - bottom;
                if (outputWidth <= 0 || outputHeight <= 0) {
                    return cv::Mat();
                }

                if (left == 0 && right == 0 && top == 0 && bottom == 0) {
                    return input;
                }

                const cv::Rect roi(left, top, outputWidth, outputHeight);
                return input(roi).clone();
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
    double m_leftPercent = 0.0;
    double m_rightPercent = 0.0;
    double m_topPercent = 0.0;
    double m_bottomPercent = 0.0;
};
} // namespace Nodes
