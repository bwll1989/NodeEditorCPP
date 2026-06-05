#pragma once

#include "NodeDataList.hpp"
#include "RTSPInInterface.hpp"
#include "RtspStreamReceiver.h"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QMetaType>
#include <QJsonObject>
#include <QSignalBlocker>
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
                QSignalBlocker blocker(m_widget->m_urlEdit);
                m_widget->m_urlEdit->setText(value);
            });
            connect(this, &RTSPInDataModel::enableChanged, this, [this](bool value) {
                QSignalBlocker blocker(m_widget->m_startStopButton);
                m_widget->m_startStopButton->setChecked(value);
            });
        }

        ~RTSPInDataModel() override
        {
            if (m_receiver) {
                disconnect(m_receiver, nullptr, this, nullptr);
                m_receiver->stop();
            }

            GlobalEventBus::instance()->unsubscribe(this);

            if (m_widget) {
                m_widget->blockSignals(true);
                if (m_widget->m_urlEdit) {
                    m_widget->m_urlEdit->blockSignals(true);
                }
                if (m_widget->m_startStopButton) {
                    m_widget->m_startStopButton->blockSignals(true);
                }
            }

            m_isReceiving = false;
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
                setUrl(v->value().toString());
                break;
            case EnablePort:
                setEnable(v->value().toBool());
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
            return m_widget;
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
                QTimer::singleShot(500, this, [this]() { setEnable(true); });
            }
        }

        QString getUrl() const { return m_url; }

        void setUrl(const QString& value)
        {
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
            if (m_isReceiving == value) {
                return;
            }

            if (value) {
                if (m_url.trimmed().isEmpty()) {
                    onErrorOccurred(QStringLiteral("RTSP 地址不能为空"));
                    if (m_widget && m_widget->m_startStopButton) {
                        QSignalBlocker blocker(m_widget->m_startStopButton);
                        m_widget->m_startStopButton->setChecked(false);
                    }
                    return;
                }
                m_receiver->start(m_url);
                m_isReceiving = true;
            } else {
                m_receiver->stop();
                m_isReceiving = false;
                if (m_widget) {
                    m_widget->updateConnectionStatus(false);
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
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/url"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onFrameReceived(const cv::Mat& frame)
        {
            if (frame.empty()) {
                return;
            }
            m_outputImageData = std::make_shared<ImageData>(frame);
            Q_EMIT dataUpdated(0);
        }

        void onConnectionStatusChanged(bool connected)
        {
            if (!m_widget) {
                return;
            }

            if (connected) {
                m_widget->updateConnectionStatus(true);
                return;
            }

            if (m_isReceiving) {
                m_widget->showError(QStringLiteral("RTSP 连接中断，正在重连..."));
            } else {
                m_widget->updateConnectionStatus(false);
            }
        }

        void onErrorOccurred(const QString& message)
        {
            if (m_widget) {
                m_widget->showError(message);
            }
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
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
        RTSPInInterface* m_widget = nullptr;
        RtspStreamReceiver* m_receiver = nullptr;
        std::shared_ptr<ImageData> m_outputImageData;
        QString m_url = QStringLiteral("rtsp://127.0.0.1:8554/live");
        bool m_isReceiving = false;
    };
}
