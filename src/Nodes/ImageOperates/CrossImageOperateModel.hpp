#pragma once

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <opencv2/imgproc.hpp>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
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
        m_worker.setParent(this);
        m_worker.setFinishedCallback([this](cv::Mat&& image, qint64 outputTimestamp, qint64, std::uint64_t) {
            if (!image.empty()) {
                ImageOperateHelpers::pushOutputFrame(
                    m_outBuffer, std::move(image), outputTimestamp, m_lastPushedTimestamp);
            }
        });

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [this](qint64 frameCount) {
                    if (!ImageOperateHelpers::hasSharedImageBufferInput(m_inputA, m_inputB)) {
                        return;
                    }
                    if (!m_tick.beginFrameTick(frameCount)) {
                        return;
                    }
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
            if (m_inputA) {
                m_tick.markInputConnected();
                Q_EMIT dataUpdated(0);
                if (!ImageOperateHelpers::hasSharedImageBufferInput(m_inputA, m_inputB)) {
                    requestProcess();
                }
            }
            break;
        case 1:
            m_inputB = std::dynamic_pointer_cast<ImageData>(data);
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            if (m_inputB) {
                m_tick.markInputConnected();
                Q_EMIT dataUpdated(0);
                if (!ImageOperateHelpers::hasSharedImageBufferInput(m_inputA, m_inputB)) {
                    requestProcess();
                }
            }
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
        m_tick.markParamsDirty();
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

    void requestProcess(qint64 targetTimestamp = -1)
    {
        ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
        const qint64 outputTimestamp = ImageOperateHelpers::normalizeTargetTimestamp(targetTimestamp);

        if (!ImageOperateHelpers::hasInputImage(m_inputA) || !ImageOperateHelpers::hasInputImage(m_inputB)) {
            ImageOperateHelpers::clearDualInputOutput(
                m_worker, m_outBuffer, m_lastPushedTimestamp, m_tick);
            return;
        }

        ImageFrame inputAFrame;
        ImageFrame inputBFrame;
        if (!ImageOperateHelpers::resolveDualInputFramesForOperate(
                m_inputA, m_inputB, outputTimestamp, inputAFrame, inputBFrame)) {
            m_worker.cancelPending();
            return;
        }

        if (!m_tick.shouldProcess(inputAFrame.timestamp, inputBFrame.timestamp)) {
            return;
        }

        const double blend = m_blend;
        const qint64 tsA = inputAFrame.timestamp;
        const qint64 tsB = inputBFrame.timestamp;

        if (blend <= 0.0) {
            m_worker.setFinishedCallback([this, tsA, tsB](cv::Mat&& image, qint64 outTs, qint64, std::uint64_t) {
                ImageOperateHelpers::pushWorkerResultDual(
                    m_outBuffer, std::move(image), outTs, m_lastPushedTimestamp, m_tick, tsA, tsB);
            });
            m_worker.submit(
                [input = inputAFrame.image.clone()]() { return input; },
                outputTimestamp,
                tsA);
            return;
        }

        if (blend >= 1.0) {
            m_worker.setFinishedCallback([this, tsA, tsB](cv::Mat&& image, qint64 outTs, qint64, std::uint64_t) {
                ImageOperateHelpers::pushWorkerResultDual(
                    m_outBuffer, std::move(image), outTs, m_lastPushedTimestamp, m_tick, tsA, tsB);
            });
            m_worker.submit(
                [input = inputBFrame.image.clone()]() { return input; },
                outputTimestamp,
                tsA);
            return;
        }

        m_worker.setFinishedCallback([this, tsA, tsB](cv::Mat&& image, qint64 outTs, qint64, std::uint64_t) {
            ImageOperateHelpers::pushWorkerResultDual(
                m_outBuffer, std::move(image), outTs, m_lastPushedTimestamp, m_tick, tsA, tsB);
        });

        m_worker.submit(
            [inputA = inputAFrame.image.clone(), inputB = inputBFrame.image.clone(), blend]() {
                const cv::Mat normalizedA = ImageOperateHelpers::ensureBgr(ImageOperateHelpers::normalizeTo8Bit(inputA));
                const cv::Mat normalizedBSource = ImageOperateHelpers::ensureBgr(ImageOperateHelpers::normalizeTo8Bit(inputB));
                if (normalizedA.empty() || normalizedBSource.empty()) {
                    return cv::Mat();
                }

                cv::Mat normalizedB;
                if (normalizedA.size() == normalizedBSource.size()) {
                    normalizedB = normalizedBSource;
                } else {
                    cv::resize(normalizedBSource, normalizedB, normalizedA.size(), 0.0, 0.0, cv::INTER_LINEAR);
                }

                cv::Mat output;
                cv::addWeighted(normalizedA, 1.0 - blend, normalizedB, blend, 0.0, output);
                return output;
            },
            outputTimestamp,
            tsA);
    }

    std::shared_ptr<ImageData> m_inputA;
    std::shared_ptr<ImageData> m_inputB;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    std::shared_ptr<ImageData> m_outImageData;
    ImageOperateHelpers::ImageOperateWorkerQueue m_worker;
    ImageOperateHelpers::ImageOperateDualTickState m_tick;
    qint64 m_lastPushedTimestamp = -1;
    double m_blend = 0.0;
};
} // namespace Nodes
