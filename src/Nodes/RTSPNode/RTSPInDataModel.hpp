#pragma once

#include "NodeDataList.hpp"
#include "RTSPInInterface.hpp"
#include "RtspStreamReceiver.h"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
#include <QJsonObject>
#include <QPointer>
#include <QSignalBlocker>
#include <atomic>
#include <opencv2/opencv.hpp>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    class RTSPInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        Q_PROPERTY(QString url READ getUrl WRITE setUrl NOTIFY urlChanged)
        Q_PROPERTY(bool enable READ getEnable WRITE setEnable NOTIFY enableChanged)

        enum InputPort : PortIndex {
            UrlPort = 0,
            EnablePort = 1
        };

    public:
        RTSPInDataModel()
            : m_widget(new RTSPInInterface())
            , m_receiver(new RtspStreamReceiver(this))
        {
            InPortCount = 2;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = QStringLiteral("RTSP In");
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            qRegisterMetaType<cv::Mat>("cv::Mat");
            ensureImageDataBuffer(m_outputImageData, m_outputBuffer);

            m_widget->m_urlEdit->setText(m_url);

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "url";
                b.control = m_widget->m_urlEdit;
                AbstractDelegateModel::registerExternalBinding("/url", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "enable";
                b.control = m_widget->m_startStopButton;
                AbstractDelegateModel::registerExternalBinding("/enable", this, b);
            }

            connect(m_widget, &RTSPInInterface::startReceiving, this, [this]() { setEnable(true); });
            connect(m_widget, &RTSPInInterface::stopReceiving, this, [this]() { setEnable(false); });
            connect(m_widget, &RTSPInInterface::urlChanged, this, &RTSPInDataModel::setUrl);

            connect(m_receiver, &RtspStreamReceiver::frameReceived,
                    this, &RTSPInDataModel::onFrameReceived, Qt::QueuedConnection);
            connect(m_receiver, &RtspStreamReceiver::connectionStatusChanged,
                    this, &RTSPInDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(m_receiver, &RtspStreamReceiver::errorOccurred,
                    this, &RTSPInDataModel::onErrorOccurred, Qt::QueuedConnection);

            connect(this, &RTSPInDataModel::urlChanged, this, [this](const QString& value) {
                if (RTSPInInterface* widget = m_widget.data()) {
                    QSignalBlocker blocker(widget->m_urlEdit);
                    widget->m_urlEdit->setText(value);
                }
            });
            connect(this, &RTSPInDataModel::enableChanged, this, [this](bool value) {
                if (RTSPInInterface* widget = m_widget.data()) {
                    QSignalBlocker blocker(widget->m_startStopButton);
                    widget->m_startStopButton->setChecked(value);
                }
            });
        }

        ~RTSPInDataModel() override
        {
            m_shuttingDown.store(true);
            GlobalEventBus::instance()->unsubscribe(this);

            setEnable(false);

            if (m_receiver) {
                disconnect(m_receiver, nullptr, this, nullptr);
                m_receiver->stop();
            }

            m_uploadScheduled.store(false);
            {
                QMutexLocker locker(&m_pendingMutex);
                m_pendingFrame.release();
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex);
            if (portType == PortType::Out) {
                return ImageData().type();
            }
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return m_outputImageData ? m_outputImageData : std::make_shared<ImageData>();
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) {
                return;
            }

            auto v = std::dynamic_pointer_cast<VariableData>(data);
            if (!v) {
                return;
            }

            switch (portIndex) {
            case UrlPort:
                setUrl(v->asString());
                break;
            case EnablePort:
                setEnable(v->asBool());
                break;
            default:
                break;
            }
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                switch (portIndex) {
                case UrlPort:
                    return QStringLiteral("URL");
                case EnablePort:
                    return QStringLiteral("Enable");
                default:
                    break;
                }
            } else if (portType == PortType::Out) {
                return QStringLiteral("IMAGE");
            }
            return {};
        }

        QWidget* embeddedWidget() override
        {
            return m_widget.data();
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values["url"] = m_url;
            values["enable"] = m_isReceiving;
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject& jsonObject) override
        {
            NodeDelegateModel::load(jsonObject);

            const QJsonValue v = jsonObject["values"];
            if (!v.isObject()) {
                return;
            }

            const QJsonObject values = v.toObject();
            if (values.contains("url")) {
                setUrl(values["url"].toString());
            }
            if (values.contains("enable") && values["enable"].toBool()) {
                QTimer::singleShot(500, this, [self = QPointer<RTSPInDataModel>(this)]() {
                    if (self && !self->m_shuttingDown.load()) {
                        self->setEnable(true);
                    }
                });
            }
        }

        QString getUrl() const { return m_url; }

        void setUrl(const QString& value)
        {
            if (m_shuttingDown.load()) {
                return;
            }
            const QString trimmed = value.trimmed();
            if (m_url == trimmed) {
                return;
            }

            m_url = trimmed;
            if (m_receiver) {
                m_receiver->setUrl(m_url);
            }
            Q_EMIT urlChanged(m_url);
        }

        bool getEnable() const { return m_isReceiving; }

        void setEnable(bool value)
        {
            if (m_shuttingDown.load()) {
                return;
            }
            if (m_isReceiving == value) {
                return;
            }

            if (value) {
                if (m_url.trimmed().isEmpty()) {
                    onErrorOccurred(QStringLiteral("RTSP 地址不能为空"));
                    if (RTSPInInterface* widget = m_widget.data()) {
                        if (widget->m_startStopButton) {
                            QSignalBlocker blocker(widget->m_startStopButton);
                            widget->m_startStopButton->setChecked(false);
                        }
                    }
                    return;
                }
                if (m_receiver) {
                    m_receiver->start(m_url);
                }
                m_isReceiving = true;
            } else {
                if (m_receiver) {
                    m_receiver->stop();
                }
                m_isReceiving = false;
                m_uploadScheduled.store(false);
                {
                    QMutexLocker locker(&m_pendingMutex);
                    m_pendingFrame.release();
                }
                if (m_outputBuffer) {
                    m_outputBuffer->clear();
                }
                m_lastPushedTimestamp = -1;
                if (RTSPInInterface* widget = m_widget.data()) {
                    widget->updateConnectionStatus(false);
                }
            }

            Q_EMIT enableChanged(m_isReceiving);
        }

    signals:
        void urlChanged(const QString& url);
        void enableChanged(bool enabled);

    protected:
        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            ImageGpuUpload::instance().warmup();
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/url"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onFrameReceived(const cv::Mat& frame)
        {
            if (m_shuttingDown.load() || !m_isReceiving || frame.empty()) {
                return;
            }
            {
                QMutexLocker locker(&m_pendingMutex);
                m_pendingFrame = frame;
            }
            scheduleUploadIfNeeded();
        }

        /** GUI 线程：RTSP 帧 → GPU 纹理 + CPU 缓存 → 输出 ring buffer */
        void publishPendingFrame()
        {
            m_uploadScheduled.store(false);

            if (m_shuttingDown.load() || !m_isReceiving) {
                return;
            }

            cv::Mat mat;
            {
                QMutexLocker locker(&m_pendingMutex);
                mat = std::move(m_pendingFrame);
            }

            if (!m_outputBuffer || mat.empty()) {
                return;
            }

            ensureImageDataBuffer(m_outputImageData, m_outputBuffer);
            const qint64 timestamp = TimestampGenerator::getInstance()->getCurrentFrameCount();
            ImageFrame imageFrame = ImageFrame::fromMat(std::move(mat), timestamp);
            if (!imageFrame.texture.valid()) {
                qWarning() << "RTSP In: GPU upload failed";
                return;
            }

            pushFrameToImageBufferDedup(m_outputBuffer, std::move(imageFrame), m_lastPushedTimestamp);

            bool hasPending = false;
            {
                QMutexLocker locker(&m_pendingMutex);
                hasPending = !m_pendingFrame.empty();
            }
            if (hasPending && !m_shuttingDown.load() && m_isReceiving) {
                scheduleUploadIfNeeded();
            }
        }

        void onConnectionStatusChanged(bool connected)
        {
            if (m_shuttingDown.load()) {
                return;
            }
            RTSPInInterface* widget = m_widget.data();
            if (!widget) {
                return;
            }

            if (connected) {
                widget->updateConnectionStatus(true);
                return;
            }

            if (m_isReceiving) {
                widget->showError(QStringLiteral("RTSP 连接中断，正在重连..."));
            } else {
                widget->updateConnectionStatus(false);
            }
        }

        void onErrorOccurred(const QString& message)
        {
            if (m_shuttingDown.load()) {
                return;
            }
            if (RTSPInInterface* widget = m_widget.data()) {
                widget->showError(message);
            }
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (m_shuttingDown.load() || ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
            if (localPath == "url") {
                setUrl(ev.payload.toString());
            } else if (localPath == "enable") {
                setEnable(ev.payload.toBool());
            }
        }

    private:
        void scheduleUploadIfNeeded()
        {
            if (m_shuttingDown.load() || !m_isReceiving) {
                return;
            }
            if (!m_uploadScheduled.exchange(true)) {
                QMetaObject::invokeMethod(this, "publishPendingFrame", Qt::QueuedConnection);
            }
        }

        QPointer<RTSPInInterface> m_widget;
        RtspStreamReceiver* m_receiver = nullptr;

        std::shared_ptr<ImageData> m_outputImageData;
        std::shared_ptr<ImageTimestampRingQueue> m_outputBuffer;
        qint64 m_lastPushedTimestamp = -1;

        QMutex m_pendingMutex;
        cv::Mat m_pendingFrame;
        std::atomic<bool> m_uploadScheduled{false};
        std::atomic<bool> m_shuttingDown{false};

        QString m_url = QStringLiteral("rtsp://127.0.0.1:8554/live");
        bool m_isReceiving = false;
    };
}
