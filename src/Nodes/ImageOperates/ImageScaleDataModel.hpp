//
// ImageScaleDataModel.hpp — 缩放到指定宽/高（GPU resize）
//
// 单输入 GPU 算子；width/height 为输出像素尺寸。tick 约定见 Doc.md §3。
//

#ifndef ImageScaleDataModel_H
#define ImageScaleDataModel_H


#include <QtNodes/NodeDelegateModel>
#include "NodeDataList.hpp"
#include "ImageOperateCommon.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

using namespace NodeDataTypes;
namespace Nodes
{
namespace ImageScaleGpu
{
/** 双线性缩放至 outWidth×outHeight */
inline GpuTextureHandle run(const GpuTextureHandle& src, int outWidth, int outHeight)
{
    if (!src.valid() || outWidth <= 0 || outHeight <= 0) {
        return {};
    }
    return ImageGpuPass::instance().resize(src, outWidth, outHeight);
}
} // namespace ImageScaleGpu

    /** @brief 图像缩放 — 单输入 GPU 算子 */
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
                m_lastProcessedInputTimestamp = -1;
                m_paramsDirty = true;
                emit dataUpdated(0);
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
            m_paramsDirty = true;
            Q_EMIT widthChanged(w);
        }

        void setHeight(int h)
        {
            if (m_height == h) return;
            m_height = h;
            m_inScaleFactor.setHeight(h);
            m_paramsDirty = true;
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
        void clearOutput()
        {
            if (m_outBuffer) {
                m_outBuffer->clear();
            }
            m_lastPushedTimestamp = -1;
            m_lastProcessedInputTimestamp = -1;
            m_paramsDirty = false;
        }

        void requestProcess(qint64 targetTimestamp = -1) {
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            const qint64 lookupTimestamp = ImageOperateHelpers::normalizeTargetTimestamp(targetTimestamp);

            if (!m_inImageData || imageDataIsEmpty(m_inImageData)) {
                clearOutput();
                return;
            }

            ImageFrame frame;
            if (!ImageOperateHelpers::resolveInputGpuFrame(m_inImageData, lookupTimestamp, frame)) {
                if (!ImageOperateHelpers::hasInputImage(m_inImageData)) {
                    clearOutput();
                }
                return;
            }

            if (!m_paramsDirty && frame.timestamp == m_lastProcessedInputTimestamp) {
                return;
            }

            GpuTextureHandle out = ImageScaleGpu::run(
                frame.texture, m_width, m_height);
            ImageOperateHelpers::pushGpuResult(m_outBuffer, std::move(out), m_lastPushedTimestamp);

            m_lastProcessedInputTimestamp = frame.timestamp;
            m_paramsDirty = false;
        }

    private:
        std::shared_ptr<ImageData> m_inImageData;
        QSize m_inScaleFactor;
        std::shared_ptr<ImageData> m_outImageData;
        std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
        qint64 m_lastRequestedFrame = -1;
        qint64 m_lastProcessedInputTimestamp = -1;
        bool m_paramsDirty = false;
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
