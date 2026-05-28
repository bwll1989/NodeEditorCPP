#pragma once

#include <QtCore/QObject>
#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QUrl>
#include "MpvControllerInterface.hpp"
#include <QtWidgets/QPushButton>
#include <QtCore/qglobal.h>
#include <QSignalBlocker>

#include "Common/Devices/HttpClient/HttpClient.h"
#include "QTimer"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class MpvControllerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString hostAddress READ getHostAddress WRITE setHostAddress NOTIFY hostAddressChanged)
        Q_PROPERTY(double volume READ getVolume WRITE setVolume NOTIFY volumeChanged)

    public:
        MpvControllerDataModel() {
            InPortCount = 1;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Mpv Controller";
            WidgetEmbeddable = false;
            Resizable = false;
            
            client = new HttpClient();
            widget = new MpvControllerInterface();
            
            // Restore external control registration
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->Play;
                AbstractDelegateModel::registerExternalBinding("/play", this, b);
            }
            // AbstractDelegateModel::registerExternalControl("/play", widget->Play);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->Fullscreen;
                AbstractDelegateModel::registerExternalBinding("/fullscreen", this, b);
            }
            // AbstractDelegateModel::registerExternalControl("/fullscreen", widget->Fullscreen);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->volumeEditor;
                AbstractDelegateModel::registerExternalBinding("/volume", this, b);
            }
            // AbstractDelegateModel::registerExternalControl("/volume", widget->volumeEditor);
            {
            NodeDelegateModel::ExternalBinding b;
            b.member = "trigger";
            b.control = widget->playlist_prev;
            AbstractDelegateModel::registerExternalBinding("/playlist_prev", this,b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->playlist_next;
                AbstractDelegateModel::registerExternalBinding("/playlist_next", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/playlist_next", widget->playlist_next);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->speedAdd;
                AbstractDelegateModel::registerExternalBinding("/speed_add", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/speed_add", widget->speedAdd);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->speedSub;
                AbstractDelegateModel::registerExternalBinding("/speed_sub", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/speed_sub", widget->speedSub);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "trigger";
                b.control = widget->speedReset;
                AbstractDelegateModel::registerExternalBinding("/speed_reset", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/speed_reset", widget->speedReset);
            // Sync host address from widget
            m_hostAddress = widget->hostEdit->text();
            if (m_hostAddress.isEmpty()) m_hostAddress = "127.0.0.1";

            timer = new QTimer();
            timer->setInterval(900);
            timer->setSingleShot(true);

            // Button Connections
            connect(widget->Play, &QPushButton::clicked, this, &MpvControllerDataModel::onPlay);
            connect(widget->Fullscreen, &QPushButton::clicked, this, &MpvControllerDataModel::onFullscreen);
            connect(widget->playlist_prev, &QPushButton::clicked, this, &MpvControllerDataModel::onplaylist_prev);
            connect(widget->playlist_next, &QPushButton::clicked, this, &MpvControllerDataModel::onplaylist_next);
            connect(widget->speedAdd, &QPushButton::clicked, this, &MpvControllerDataModel::speedAdd);
            connect(widget->speedSub, &QPushButton::clicked, this, &MpvControllerDataModel::speedSub);
            connect(widget->speedReset, &QPushButton::clicked, this, &MpvControllerDataModel::speedReset);

            // Client Status Connection - Fix for stack pointer bug
            connect(client, &HttpClient::getSatus, this, [this](QJsonObject state){
                *this->status = state; // Deep copy instead of pointer assignment
                emit dataUpdated(0);
            });

            // Property Connections
            connect(widget->volumeEditor, &FloatDragValueWidget::valueChanged, this, &MpvControllerDataModel::setVolume);
            connect(widget->hostEdit, &QLineEdit::textChanged, this, &MpvControllerDataModel::setHostAddress);
            
            connect(timer, &QTimer::timeout, this, &MpvControllerDataModel::getStatus);
        }

        ~MpvControllerDataModel()
        {
            // delete widget;
            // delete client;
        }

    public:
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            case PortType::None:
                break;
            default:
                break;
            }
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(status);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if (!textData->isEmpty()) {
                client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/" + textData->value().toString()));
                Q_EMIT dataUpdated(portIndex);
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values["hostAddress"] = m_hostAddress;
            values["volume"] = m_volume;
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                if (values.contains("hostAddress")) {
                    setHostAddress(values["hostAddress"].toString());
                }
                if (values.contains("volume")) {
                    setVolume(values["volume"].toDouble());
                }
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }
            return result;
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            
            // Subscribe to properties
            bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/volume"), this, SLOT(onGlobalEvent(GlobalEvent)));
            
            // Subscribe to commands
            bus->subscribe(makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/fullscreen"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/playlist_prev"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/playlist_next"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/speed_add"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/speed_sub"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/speed_reset"), this, SLOT(onGlobalEvent(GlobalEvent)));

        }

    public:
        QString getHostAddress() const { return m_hostAddress; }
        void setHostAddress(const QString& value)
        {
            if (m_hostAddress == value) return;
            m_hostAddress = value;
            
            if (widget && widget->hostEdit->text() != value) {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(value);
            }
            
            Q_EMIT hostAddressChanged(value);
            AbstractDelegateModel::stateFeedBack("/host", value);
        }

        double getVolume() const { return m_volume; }
        void setVolume(double value)
        {
            if (qFuzzyCompare(m_volume, value)) return;
            m_volume = value;

            if (widget && !qFuzzyCompare(widget->volumeEditor->value(), value)) {
                QSignalBlocker blocker(widget->volumeEditor);
                widget->volumeEditor->setValue(value);
            }

            // Send request to MPV
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/set_volume/" + QString::number(value)));
            timer->start();

            Q_EMIT volumeChanged(value);
            AbstractDelegateModel::stateFeedBack("/volume", value);
        }

    Q_SIGNALS:
        void hostAddressChanged(QString value);
        void volumeChanged(double value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "host") {
                setHostAddress(ev.payload.toString());
            } else if (localPath == "volume") {
                setVolume(ev.payload.toDouble());
            } else if (localPath == "play") {
                onPlay();
            } else if (localPath == "fullscreen") {
                onFullscreen();
            } else if (localPath == "playlist_prev") {
                onplaylist_prev();
            } else if (localPath == "playlist_next") {
                onplaylist_next();
            } else if (localPath == "speed_add") {
                speedAdd();
            } else if (localPath == "speed_sub") {
                speedSub();
            } else if (localPath == "speed_reset") {
                speedReset();
            }
        }

        void onPlay()
        {
            AbstractDelegateModel::stateFeedBack("/play", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/toggle_pause"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/play", false);
        }
        void onFullscreen()
        {
            AbstractDelegateModel::stateFeedBack("/fullscreen", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/fullscreen"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/fullscreen", false);
        }
        void getStatus()
        {
            client->sendGetRequest(QUrl("http://" + m_hostAddress + ":8080/api/status"));
        }

        void onplaylist_prev()
        {
            AbstractDelegateModel::stateFeedBack("/playlist_prev", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/playlist_prev"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/playlist_prev", false);
        }
        void onplaylist_next()
        {
            AbstractDelegateModel::stateFeedBack("/playlist_next", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/playlist_next"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/playlist_next", false);
        }
        void speedAdd()
        {
            AbstractDelegateModel::stateFeedBack("/speed_add", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/speed_adjust/1.1"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/speed_add", false);
        }
        void speedSub()
        {
            AbstractDelegateModel::stateFeedBack("/speed_sub", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/speed_adjust/0.9"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/speed_sub", false);
        }
        void speedReset()
        {
            AbstractDelegateModel::stateFeedBack("/speed_reset", true);
            client->sendPostRequest(QUrl("http://" + m_hostAddress + ":8080/api/speed_set"));
            timer->start();
            AbstractDelegateModel::stateFeedBack("/speed_reset", false);
        }

    private:
        HttpClient *client;
        QString m_hostAddress = "127.0.0.1";
        double m_volume = 0.0;
        MpvControllerInterface *widget;
        QTimer *timer;
        QJsonObject *status = new QJsonObject();
    };
}
