#pragma once

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <vector>

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
        m_tick.markParamsDirty();
        Q_EMIT maskChannelChanged(value);
        Q_EMIT maskChannelIndexChanged(maskChannelIndex());
        if (!ImageOperateHelpers::hasSharedImageBufferInput(m_inputA, m_inputB)) {
            requestProcess();
        }
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

    /** 输入2：被遮挡的彩色图像（OpenCV SIMD 路径） */
    static cv::Mat toImageBgra8(const cv::Mat& image)
    {
        const cv::Mat normalized = ImageOperateHelpers::normalizeTo8Bit(image);
        if (normalized.empty()) {
            return cv::Mat();
        }

        if (normalized.channels() == 4) {
            cv::Mat bgra;
            if (normalized.type() == CV_8UC4) {
                bgra = normalized;
            } else {
                normalized.convertTo(bgra, CV_8UC4);
            }
            if (bgra.empty()) {
                return cv::Mat();
            }

            cv::Mat alpha;
            cv::extractChannel(bgra, alpha, 3);
            double minAlpha = 0.0;
            double maxAlpha = 0.0;
            cv::minMaxLoc(alpha, &minAlpha, &maxAlpha);
            if (maxAlpha <= 0.0) {
                alpha.setTo(255);
                cv::insertChannel(alpha, bgra, 3);
            }
            return bgra;
        }

        const cv::Mat bgr = normalized.channels() == 3
            ? normalized
            : ImageOperateHelpers::ensureBgr(normalized);
        if (bgr.empty()) {
            return cv::Mat();
        }

        cv::Mat bgra;
        cv::cvtColor(bgr, bgra, cv::COLOR_BGR2BGRA);
        return bgra;
    }

    /** 输入1：按所选 BGRA 通道提取单通道遮罩 */
    static cv::Mat extractMaskChannel8(const cv::Mat& image, MaskChannel channel)
    {
        const cv::Mat normalized = ImageOperateHelpers::normalizeTo8Bit(image);
        if (normalized.empty()) {
            return cv::Mat();
        }

        if (normalized.channels() == 1) {
            return normalized;
        }

        const int channelIndex = static_cast<int>(channel);

        if (normalized.channels() == 4) {
            cv::Mat bgra;
            if (normalized.type() == CV_8UC4) {
                bgra = normalized;
            } else {
                normalized.convertTo(bgra, CV_8UC4);
            }
            cv::Mat mask;
            cv::extractChannel(bgra, mask, channelIndex);
            return mask;
        }

        if (channel == MaskChannel::Alpha) {
            cv::Mat gray;
            cv::cvtColor(normalized, gray, cv::COLOR_BGR2GRAY);
            return gray;
        }

        cv::Mat mask;
        cv::extractChannel(normalized, mask, channelIndex);
        return mask;
    }

    /** visibility = image * (255 - mask) / 255，全程 OpenCV 向量化 */
    static cv::Mat compositeMaskOcclude(const cv::Mat& maskAlpha, const cv::Mat& imageBgra)
    {
        if (maskAlpha.empty() || imageBgra.empty()) {
            return cv::Mat();
        }

        cv::Mat mask = maskAlpha;
        if (mask.type() != CV_8UC1) {
            mask.convertTo(mask, CV_8U);
        }
        if (mask.size() != imageBgra.size()) {
            cv::resize(mask, mask, imageBgra.size(), 0.0, 0.0, cv::INTER_LINEAR);
        }

        cv::Mat visibility;
        cv::subtract(cv::Scalar(255), mask, visibility);

        cv::Mat visibilityBgra;
        {
            const cv::Mat channels[] = {visibility, visibility, visibility, visibility};
            cv::merge(channels, 4, visibilityBgra);
        }

        cv::Mat output;
        cv::multiply(imageBgra, visibilityBgra, output, 1.0 / 255.0);
        return output;
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

        const qint64 tsA = inputAFrame.timestamp;
        const qint64 tsB = inputBFrame.timestamp;
        const MaskChannel maskChannel = m_maskChannel;

        m_worker.setFinishedCallback([this, tsA, tsB](cv::Mat&& image, qint64 outTs, qint64, std::uint64_t) {
            ImageOperateHelpers::pushWorkerResultDual(
                m_outBuffer, std::move(image), outTs, m_lastPushedTimestamp, m_tick, tsA, tsB);
        });

        m_worker.submit(
            [maskInput = inputAFrame.image.clone(), imageInput = inputBFrame.image.clone(), maskChannel]() {
                const cv::Mat maskAlpha = extractMaskChannel8(maskInput, maskChannel);
                const cv::Mat imageBgra = toImageBgra8(imageInput);
                if (maskAlpha.empty() || imageBgra.empty()) {
                    return cv::Mat();
                }
                return compositeMaskOcclude(maskAlpha, imageBgra);
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
    MaskChannel m_maskChannel = MaskChannel::Alpha;
};
} // namespace Nodes
