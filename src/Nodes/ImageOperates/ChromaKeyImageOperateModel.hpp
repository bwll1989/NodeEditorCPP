#pragma once

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtNodes/NodeDelegateModel>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
class ChromaKeyImageOperateModel final : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(double hueMin READ hueMin WRITE setHueMin NOTIFY hueMinChanged)
    Q_PROPERTY(double hueMax READ hueMax WRITE setHueMax NOTIFY hueMaxChanged)
    Q_PROPERTY(double hueSoftLow READ hueSoftLow WRITE setHueSoftLow NOTIFY hueSoftLowChanged)
    Q_PROPERTY(double hueSoftHigh READ hueSoftHigh WRITE setHueSoftHigh NOTIFY hueSoftHighChanged)

public:
    ChromaKeyImageOperateModel()
    {
        InPortCount = 5;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Chroma Key";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = false;

        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueMin";
            AbstractDelegateModel::registerExternalBinding("/huemin", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueMax";
            AbstractDelegateModel::registerExternalBinding("/huemax", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueSoftLow";
            AbstractDelegateModel::registerExternalBinding("/hsoftlow", this, binding);
        }
        {
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "hueSoftHigh";
            AbstractDelegateModel::registerExternalBinding("/hsofthigh", this, binding);
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

    ~ChromaKeyImageOperateModel() override { GlobalEventBus::instance()->unsubscribe(this); }

    double hueMin() const { return m_hueMin; }
    double hueMax() const { return m_hueMax; }
    double hueSoftLow() const { return m_hueSoftLow; }
    double hueSoftHigh() const { return m_hueSoftHigh; }

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
            case 1: return "HUE MIN";
            case 2: return "HUE MAX";
            case 3: return "SOFT LOW";
            case 4: return "SOFT HIGH";
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
                setHueMin(variable->value().toDouble());
            } else if (!data) {
                setHueMin(0.0);
            }
            break;
        case 2:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHueMax(variable->value().toDouble());
            } else if (!data) {
                setHueMax(120.0);
            }
            break;
        case 3:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHueSoftLow(variable->value().toDouble());
            } else if (!data) {
                setHueSoftLow(0.0);
            }
            break;
        case 4:
            if (auto variable = std::dynamic_pointer_cast<VariableData>(data)) {
                setHueSoftHigh(variable->value().toDouble());
            } else if (!data) {
                setHueSoftHigh(0.0);
            }
            break;
        default:
            break;
        }
    }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["hueMin"] = m_hueMin;
        values["hueMax"] = m_hueMax;
        values["hueSoftLow"] = m_hueSoftLow;
        values["hueSoftHigh"] = m_hueSoftHigh;

        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = values;
        return modelJson;
    }

    void load(const QJsonObject& data) override
    {
        const QJsonObject values = data["values"].toObject();
        setHueMin(values.value("hueMin").toDouble(0.0));
        setHueMax(values.value("hueMax").toDouble(120.0));
        setHueSoftLow(values.value("hueSoftLow").toDouble(0.0));
        setHueSoftHigh(values.value("hueSoftHigh").toDouble(0.0));
    }

public slots:
    void setHueMin(double value)
    {
        const double clamped = clampHue(value);
        if (qFuzzyCompare(m_hueMin + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueMin = clamped;
        m_tick.markParamsDirty();
        Q_EMIT hueMinChanged(m_hueMin);
    }

    void setHueMax(double value)
    {
        const double clamped = clampHue(value);
        if (qFuzzyCompare(m_hueMax + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueMax = clamped;
        m_tick.markParamsDirty();
        Q_EMIT hueMaxChanged(m_hueMax);
    }

    void setHueSoftLow(double value)
    {
        const double clamped = clampSoftness(value);
        if (qFuzzyCompare(m_hueSoftLow + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueSoftLow = clamped;
        m_tick.markParamsDirty();
        Q_EMIT hueSoftLowChanged(m_hueSoftLow);
    }

    void setHueSoftHigh(double value)
    {
        const double clamped = clampSoftness(value);
        if (qFuzzyCompare(m_hueSoftHigh + 1.0, clamped + 1.0)) {
            return;
        }
        m_hueSoftHigh = clamped;
        m_tick.markParamsDirty();
        Q_EMIT hueSoftHighChanged(m_hueSoftHigh);
    }

    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        if (ev.address == makeFullOscAddress("/huemin")) {
            setHueMin(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/huemax")) {
            setHueMax(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/hsoftlow")) {
            setHueSoftLow(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/hsofthigh")) {
            setHueSoftHigh(ev.payload.toDouble());
        }
    }

signals:
    void hueMinChanged(double value);
    void hueMaxChanged(double value);
    void hueSoftLowChanged(double value);
    void hueSoftHighChanged(double value);

private:
    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        auto* bus = GlobalEventBus::instance();
        bus->subscribe(makeFullOscAddress("/huemin"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/huemax"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/hsoftlow"), this, SLOT(onGlobalEvent(GlobalEvent)));
        bus->subscribe(makeFullOscAddress("/hsofthigh"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    static double clampHue(double value)
    {
        return std::clamp(value, 0.0, 360.0);
    }

    static double clampSoftness(double value)
    {
        return std::clamp(value, 0.0, 360.0);
    }

    static uchar computeAlpha(double hueDegrees, double hueMin, double hueMax, double softLow, double softHigh)
    {
        double intervalMin = hueMin;
        double intervalMax = hueMax;
        if (intervalMax < intervalMin) {
            intervalMax += 360.0;
        }

        if ((intervalMax - intervalMin) >= 360.0) {
            return 0;
        }

        double alpha = 255.0;
        const std::array<double, 3> candidates {hueDegrees - 360.0, hueDegrees, hueDegrees + 360.0};

        for (double candidate : candidates) {
            if (candidate >= intervalMin && candidate <= intervalMax) {
                return 0;
            }

            if (candidate < intervalMin && softLow > 0.0 && candidate >= intervalMin - softLow) {
                const double t = (intervalMin - candidate) / softLow;
                alpha = std::min(alpha, t * 255.0);
            } else if (candidate > intervalMax && softHigh > 0.0 && candidate <= intervalMax + softHigh) {
                const double t = (candidate - intervalMax) / softHigh;
                alpha = std::min(alpha, t * 255.0);
            }
        }

        return static_cast<uchar>(std::clamp(alpha, 0.0, 255.0));
    }

    static void splitColorAndAlpha(const cv::Mat& input, cv::Mat& bgr, cv::Mat& baseAlpha)
    {
        cv::Mat normalized = ImageOperateHelpers::normalizeTo8Bit(input);
        if (normalized.empty()) {
            bgr.release();
            baseAlpha.release();
            return;
        }

        if (normalized.channels() == 4) {
            std::vector<cv::Mat> channels;
            cv::split(normalized, channels);
            baseAlpha = channels[3].clone();
            cv::cvtColor(normalized, bgr, cv::COLOR_BGRA2BGR);
            return;
        }

        bgr = ImageOperateHelpers::ensureBgr(normalized);
        baseAlpha = cv::Mat(bgr.rows, bgr.cols, CV_8UC1, cv::Scalar(255));
    }

    static cv::Mat applyChromaKey(const cv::Mat& input,
                                  double hueMin,
                                  double hueMax,
                                  double hueSoftLow,
                                  double hueSoftHigh)
    {
        cv::Mat bgr;
        cv::Mat baseAlpha;
        splitColorAndAlpha(input, bgr, baseAlpha);
        if (bgr.empty() || baseAlpha.empty()) {
            return cv::Mat();
        }

        cv::Mat hsv;
        cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);

        cv::Mat alpha = cv::Mat::zeros(hsv.rows, hsv.cols, CV_8UC1);
        for (int y = 0; y < hsv.rows; ++y) {
            const auto* hsvRow = hsv.ptr<cv::Vec3b>(y);
            const auto* baseAlphaRow = baseAlpha.ptr<uchar>(y);
            auto* alphaRow = alpha.ptr<uchar>(y);
            for (int x = 0; x < hsv.cols; ++x) {
                const double hueDegrees = static_cast<double>(hsvRow[x][0]) * 2.0;
                const uchar maskAlpha = computeAlpha(hueDegrees, hueMin, hueMax, hueSoftLow, hueSoftHigh);
                alphaRow[x] = static_cast<uchar>((static_cast<int>(baseAlphaRow[x]) * static_cast<int>(maskAlpha)) / 255);
            }
        }

        cv::Mat output;
        cv::cvtColor(bgr, output, cv::COLOR_BGR2BGRA);
        std::vector<cv::Mat> channels;
        cv::split(output, channels);
        channels[3] = alpha;
        cv::merge(channels, output);
        return output;
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

        const double hueMin = m_hueMin;
        const double hueMax = m_hueMax;
        const double hueSoftLow = m_hueSoftLow;
        const double hueSoftHigh = m_hueSoftHigh;

        m_worker.submit(
            [input = inputFrame.image.clone(), hueMin, hueMax, hueSoftLow, hueSoftHigh]() {
                return applyChromaKey(input, hueMin, hueMax, hueSoftLow, hueSoftHigh);
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
    double m_hueMin = 80.0;
    double m_hueMax = 160.0;
    double m_hueSoftLow = 10.0;
    double m_hueSoftHigh = 10.0;
};
} // namespace Nodes
