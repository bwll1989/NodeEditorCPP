#pragma once

#include "ImageOperateCommon.hpp"
#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QAbstractScrollArea>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>
struct GlobalEvent;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class ImageThresholdDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        enum class ThresholdMethod : int {
            Binary = 0,
            BinaryInv = 1,
            Trunc = 2,
            ToZero = 3,
            ToZeroInv = 4
        };
        Q_ENUM(ThresholdMethod)

        Q_PROPERTY(int thresh READ thresh WRITE setThresh NOTIFY threshChanged)
        Q_PROPERTY(int maxval READ maxval WRITE setMaxval NOTIFY maxvalChanged)
        Q_PROPERTY(ThresholdMethod method READ method WRITE setMethod NOTIFY methodChanged)
        Q_PROPERTY(int methodIndex READ methodIndex WRITE setMethodIndex NOTIFY methodIndexChanged DESIGNABLE false)

        public:
        ImageThresholdDataModel()
        {
            InPortCount =4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Image Threshold";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            ImageOperateHelpers::ensureSharedOutput(m_outImageData, m_outBuffer);
            m_worker.setParent(this);
            m_worker.setFinishedCallback([this](cv::Mat&& image, qint64 outputTimestamp, qint64 inputTimestamp, std::uint64_t) {
                ImageOperateHelpers::pushWorkerResult(
                    m_outBuffer, std::move(image), outputTimestamp, m_lastPushedTimestamp, m_tick, inputTimestamp);
            });
            // connect(widget->threshEdit,&IntDragValueWidget::valueChanged,this,[this](int v){ setThresh(v); });
            // connect(widget->maxvalEdit,&IntDragValueWidget::valueChanged,this,[this](int v){ setMaxval(v); });
            // connect(widget->methodEdit,&QComboBox::currentIndexChanged,this,[this](int v){ setMethod(static_cast<ThresholdMethod>(v)); });

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "methodIndex";
                AbstractDelegateModel::registerExternalBinding("/method", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "thresh";
                // b.control = widget->threshEdit;
                AbstractDelegateModel::registerExternalBinding("/thresh", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "maxval";
                // b.control = widget->maxvalEdit;
                AbstractDelegateModel::registerExternalBinding("/maxval", this, b);
            }

            // m_thresh = widget->threshEdit->value();
            // m_maxval = widget->maxvalEdit->value();
            // m_method = static_cast<ThresholdMethod>(widget->methodEdit->currentIndex());

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

        /**
         * @brief 析构函数，解除事件总线订阅，避免节点销毁后订阅残留导致内存增长
         */
        ~ImageThresholdDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);
        }

        int thresh() const { return m_thresh; }
        int maxval() const { return m_maxval; }
        ThresholdMethod method() const { return m_method; }
        int methodIndex() const { return static_cast<int>(m_method); }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "IMAGE";
                case 1:
                    return "THRESHOLD";
                case 2:
                    return "MAXVAL";
                case 3:
                    return "METHOD";
                default:
                    return "INPUT "+QString::number(portIndex);
                }
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType)
            {
            case PortType::In:
                switch (portIndex)
                {
            case 0:
                    return ImageData().type();
            case 1:
                    return VariableData().type();

            default:
                    return VariableData().type();
            };

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
            if (data == nullptr) {
                if (portIndex == 0) {
                    m_inImage.reset();
                    if (m_outBuffer) {
                        m_outBuffer->clear();
                    }
                    m_lastPushedTimestamp = -1;
                    Q_EMIT dataUpdated(0);
                }
                return;
            }
            switch (portIndex)
            {
            case 0:
                m_inImage=std::dynamic_pointer_cast<ImageData>(data);
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
                setThresh(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 2:
                setMaxval(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 3:
                setMethodIndex(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            }
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

            const int thresh = m_thresh;
            const int maxval = m_maxval;
            const int method = static_cast<int>(m_method);

            m_worker.submit(
                [input = inputFrame.image.clone(), thresh, maxval, method]() {
                    cv::Mat inputMat = input;
                    if (inputMat.empty()) {
                        return cv::Mat();
                    }
                    if (inputMat.channels() > 1) {
                        cv::cvtColor(inputMat, inputMat, cv::COLOR_BGR2GRAY);
                    }
                    cv::Mat thresholdedMat;
                    cv::threshold(inputMat, thresholdedMat, thresh, maxval, method);
                    return thresholdedMat;
                },
                outputTimestamp,
                inputFrame.timestamp);
        }
        // QWidget *embeddedWidget() override
        // {
        //     return widget;
        // }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson  = NodeDelegateModel::save();

            modelJson1["thresh"] = m_thresh;
            modelJson1["maxval"] = m_maxval;
            modelJson1["method"] = static_cast<int>(m_method);
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setThresh(v.toObject()["thresh"].toInt());
                setMaxval(v.toObject()["maxval"].toInt());
                setMethodIndex(v.toObject()["method"].toInt());
            }
        }

    private:

        // ImageThresholdInterface *widget=new ImageThresholdInterface();
        std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
        std::shared_ptr<ImageData> m_outImageData;
        std::shared_ptr<ImageData> m_inImage;
        std::shared_ptr<VariableData> m_inVariable;
        ImageOperateHelpers::ImageOperateWorkerQueue m_worker;
        ImageOperateHelpers::ImageOperateTickState m_tick;
        qint64 m_lastPushedTimestamp = -1;
        int m_thresh = 0;
        int m_maxval = 0;
        ThresholdMethod m_method = ThresholdMethod::Binary;

    public Q_SLOTS:
        void setThresh(int v)
        {
            if (m_thresh == v) return;
            m_thresh = v;
            // if (widget && widget->threshEdit) {
            //     const QSignalBlocker blocker(widget->threshEdit);
            //     widget->threshEdit->setValue(v);
            // }
            m_tick.markParamsDirty();
            Q_EMIT threshChanged(v);
        }

        void setMaxval(int v)
        {
            if (m_maxval == v) return;
            m_maxval = v;
            // if (widget && widget->maxvalEdit) {
            //     const QSignalBlocker blocker(widget->maxvalEdit);
            //     widget->maxvalEdit->setValue(v);
            // }
            m_tick.markParamsDirty();
            Q_EMIT maxvalChanged(v);
        }

        void setMethod(ThresholdMethod v)
        {
            if (m_method == v) return;
            m_method = v;
            m_tick.markParamsDirty();
            Q_EMIT methodChanged();
            Q_EMIT methodIndexChanged(methodIndex());
        }

        void setMethodIndex(int v)
        {
            setMethod(static_cast<ThresholdMethod>(v));
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            const QString addrThresh = makeFullOscAddress("/thresh");
            const QString addrMaxval = makeFullOscAddress("/maxval");
            const QString addrMethod = makeFullOscAddress("/method");
            if (ev.address == addrThresh) {
                setThresh(ev.payload.toInt());
            } else if (ev.address == addrMaxval) {
                setMaxval(ev.payload.toInt());
            } else if (ev.address == addrMethod) {
                setMethodIndex(ev.payload.toInt());
            }
        }

    Q_SIGNALS:
        void threshChanged(int v);
        void maxvalChanged(int v);
        void methodChanged();
        void methodIndexChanged(int v);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/thresh"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/maxval"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/method"),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
        }
    };
}
