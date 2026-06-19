//
// Created by pablo on 3/5/24.
//

#ifndef ImageScaleDataModel_H
#define ImageScaleDataModel_H


#include <QtNodes/NodeDelegateModel>
#include "NodeDataList.hpp"
#include "ImageOperateCommon.hpp"
#include <QElapsedTimer>
#include <QFileDialog>
#include <opencv2/imgproc.hpp>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

using namespace NodeDataTypes;
namespace Nodes
{
    class ImageScaleDataModel final : public AbstractDelegateModel {
        Q_OBJECT
        Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
        Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

    public:
        ImageScaleDataModel() {
            InPortCount =4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Image Scale";
            WidgetEmbeddable=false;
            Resizable=false;
            PortEditable=false;

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "width";
                AbstractDelegateModel::registerExternalBinding("/width", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "height";
                AbstractDelegateModel::registerExternalBinding("/height", this, b);
            }

            m_width = 0;
            m_height = 0;
            m_inScaleFactor = QSize(m_width, m_height);

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
                        if (!ImageOperateHelpers::usesSharedImageBuffer(m_inImageData)) {
                            return;
                        }
                        if (!m_tick.beginFrameTick(frameCount)) {
                            return;
                        }
                        requestProcess(frameCount);
                    },
                    Qt::QueuedConnection);
        };

        ~ImageScaleDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);
        }
        int width() const { return m_width; }
        int height() const { return m_height; }

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
            switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex) {
                case 0:
                        return ImageData().type();
                case 1:
                        return VariableData().type();
                case 2:
                        return VariableData().type();
                case 3:
                        return VariableData().type();
                default:
                        return ImageData().type();
                }
            case QtNodes::PortType::Out:
                switch (portIndex) {
                case 0:
                        return ImageData().type();
                default:
                        return ImageData().type();
                    }
                default:
                    return VariableData().type();
            }
        }
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {

            switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
            case 0:
                    return "Image";
            case 1:
                    return "SIZE";
            case 2:
                    return "Width";
            case 3:
                    return "Height";
                }
            case QtNodes::PortType::Out:
                return "Image";
            default:
                break;
            }
            return "";
        }

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override {

            switch (portIndex) {
            case 0:
                m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
                ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
                if (m_inImageData) {
                    m_tick.markInputConnected();
                    emit dataUpdated(0);
                    if (!ImageOperateHelpers::usesSharedImageBuffer(m_inImageData)) {
                        requestProcess();
                    }
                }
                break;
            case 1:

                if (auto data=std::dynamic_pointer_cast<VariableData>(nodeData))
                {
                    m_inScaleFactor=data->value().toSize();
                    setWidth(m_inScaleFactor.width());
                    setHeight(m_inScaleFactor.height());
                }
                break;
            case 2:
                if (auto data=std::dynamic_pointer_cast<VariableData>(nodeData))
                {
                    setWidth(data->value().toInt());
                }
                break;
            case 3:
                if (auto data=std::dynamic_pointer_cast<VariableData>(nodeData))
                {
                    setHeight(data->value().toInt());
                }
                break;
            default:
                break;
            }


        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override {
            return m_outImageData;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["width"] = m_width;
            modelJson1["height"] = m_height;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["size"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["size"];
            if (!v.isUndefined()&&v.isObject()) {
                setWidth(v["width"].toInt());
                setHeight(v["height"].toInt());
            }
        }

    public slots:
        void setWidth(int w)
        {
            if (m_width == w) return;
            m_width = w;
            m_inScaleFactor.setWidth(w);
            m_tick.markParamsDirty();
            Q_EMIT widthChanged(w);
        }

        void setHeight(int h)
        {
            if (m_height == h) return;
            m_height = h;
            m_inScaleFactor.setHeight(h);
            m_tick.markParamsDirty();
            Q_EMIT heightChanged(h);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addrWidth = makeFullOscAddress("/Width");
            const QString addrHeight = makeFullOscAddress("/Height");
            if (ev.address == addrWidth) {
                setWidth(ev.payload.toInt());
            } else if (ev.address == addrHeight) {
                setHeight(ev.payload.toInt());
            }
        }

    signals:
        void widthChanged(int width);
        void heightChanged(int height);
        void lastProcessMsChanged(qint64 ms);
    private:
         static cv::Mat processImage(const cv::Mat& inputImage, const QSize& scaleFactor) {
            if (inputImage.empty() || scaleFactor.width() <= 0 || scaleFactor.height() <= 0) {
                return cv::Mat();
            }

            cv::Mat outputImage;
            try {
                cv::resize(inputImage, outputImage,
                          cv::Size(scaleFactor.width(), scaleFactor.height()),
                          0, 0, cv::INTER_LINEAR);
            } catch (const cv::Exception& e) {
                qWarning() << "OpenCV resize error:" << e.what();
                return cv::Mat();
            }

            return outputImage;
        }

        void requestProcess(qint64 targetTimestamp = -1) {
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            const qint64 outputTimestamp = ImageOperateHelpers::normalizeTargetTimestamp(targetTimestamp);

            if (imageDataIsEmpty(m_inImageData)) {
                m_worker.cancelPending();
                if (m_outBuffer) {
                    m_outBuffer->clear();
                }
                m_lastPushedTimestamp = -1;
                m_tick.resetOutputState();
                return;
            }

            ImageFrame frame;
            if (!ImageOperateHelpers::resolveImageFrameAtTimestamp(m_inImageData, outputTimestamp, frame) ||
                frame.image.empty()) {
                m_worker.cancelPending();
                if (m_outBuffer) {
                    m_outBuffer->clear();
                }
                m_lastPushedTimestamp = -1;
                return;
            }

            if (!m_tick.shouldProcess(frame.timestamp)) {
                return;
            }

            const QSize scaleFactor = m_inScaleFactor;
            m_worker.submit(
                [input = frame.image.clone(), scaleFactor]() {
                    return processImage(input, scaleFactor);
                },
                outputTimestamp,
                frame.timestamp);
        }

    private:
        std::shared_ptr<ImageData> m_inImageData;
        QSize m_inScaleFactor;
        std::shared_ptr<ImageData> m_outImageData;
        std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
        ImageOperateHelpers::ImageOperateWorkerQueue m_worker;
        ImageOperateHelpers::ImageOperateTickState m_tick;
        int m_width = 0;
        int m_height = 0;
        qint64 m_lastPushedTimestamp = -1;

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/Width"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/Height"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }
    };
}
#endif //ImageScaleDataModel_H
