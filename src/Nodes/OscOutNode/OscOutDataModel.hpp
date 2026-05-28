#pragma once

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "Common/Devices/OSCSender/OSCSender.h"
#include "OscOutInterface.hpp"
#include <QVariantMap>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/AppConfig/ConfigManager.h"
#include "QThread"
#include "spdlog/fmt/bundled/base.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class OscOutDataModel : public AbstractDelegateModel
    {

        Q_OBJECT
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString address READ getAddress WRITE setAddress NOTIFY addressChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(int typeIndex READ getTypeIndex WRITE setTypeIndex NOTIFY typeIndexChanged)
        Q_PROPERTY(bool send READ send WRITE setSend NOTIFY sendChanged)

    public:
        OscOutDataModel()
        {
            InPortCount =5;
            OutPortCount=0;
            CaptionVisible=true;
            Caption="OSC Output";
            WidgetEmbeddable=false;
            Resizable=false;
            inData=std::make_shared<VariableData>();
            setup();
        }

        ~OscOutDataModel() override {}

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/address"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/type"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        void setup() {
            widget=new OscOutInterface();
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "host";
                b.control = widget->hostLineEdit;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "port";
                b.control = widget->portSpinBox;
                AbstractDelegateModel::registerExternalBinding("/port", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "address";
                b.control = widget->addressLineEdit;
                AbstractDelegateModel::registerExternalBinding("/address", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "value";
                b.control = widget->valueLineEdit;
                AbstractDelegateModel::registerExternalBinding("/value", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "typeIndex";
                b.control = widget->typeComboBox;
                AbstractDelegateModel::registerExternalBinding("/type", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "send";
                b.control = widget->sendButton;
                AbstractDelegateModel::registerExternalBinding("/send", this, b);
            }

            connect(this,&OscOutDataModel::onHasOSC,this,[this](OSCMessage &msg){
                OSCSender::instance()->sendOSCMessageWithQueue(msg);
            });

            connect(widget->valueLineEdit, &QLineEdit::textChanged, this, &OscOutDataModel::setValue);
            connect(widget->sendButton, &QPushButton::clicked, this, [this](){ setSend(true); });

            connect(widget->hostLineEdit, &QLineEdit::textChanged, this, &OscOutDataModel::setHost);
            connect(widget->portSpinBox, &IntDragValueWidget::valueChanged, this, &OscOutDataModel::setPort);
            connect(widget->addressLineEdit, &QLineEdit::textChanged, this, &OscOutDataModel::setAddress);
            connect(widget->typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OscOutDataModel::setTypeIndex);

        }

        QString getHost() const { return m_host; }
        void setHost(const QString& value) {
            if (m_host == value) return;
            m_host = value;
            if (widget && widget->hostLineEdit->text() != value) {
                QSignalBlocker blocker(widget->hostLineEdit);
                widget->hostLineEdit->setText(value);
            }
            emit hostChanged(value);
            // AbstractDelegateModel::stateFeedBack("/host", value);
        }

        int getPort() const { return m_port; }
        void setPort(int value) {
            if (m_port == value) return;
            m_port = value;
            if (widget && widget->portSpinBox->value() != value) {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(value);
            }
            emit portChanged(value);
            // AbstractDelegateModel::stateFeedBack("/port", value);
        }

        QString getAddress() const { return m_address; }
        void setAddress(const QString& value) {
            if (m_address == value) return;
            m_address = value;
            if (widget && widget->addressLineEdit->text() != value) {
                QSignalBlocker blocker(widget->addressLineEdit);
                widget->addressLineEdit->setText(value);
            }
            emit addressChanged(value);
            // AbstractDelegateModel::stateFeedBack("/address", value);
        }

        QString getValue() const { return m_value; }
        void setValue(const QString& value) {
            if (m_value == value) return;
            m_value = value;
            if (widget && widget->valueLineEdit->text() != value) {
                QSignalBlocker blocker(widget->valueLineEdit);
                widget->valueLineEdit->setText(value);
            }
            emit valueChanged(value);
            // AbstractDelegateModel::stateFeedBack("/value", value);
            // Auto-send when value changes? The original code connected textChanged to sendOSCMessage.
            // Let's keep it consistent: manual send or via trigger. 
            // BUT wait, original line 55: connect(widget->valueLineEdit,&QLineEdit::textChanged,this,&OscOutDataModel::sendOSCMessage);
            // So yes, value change triggers send.
            sendOSCMessage(); 
        }

        int getTypeIndex() const { return m_typeIndex; }
        void setTypeIndex(int value) {
            if (m_typeIndex == value) return;
            m_typeIndex = value;
            if (widget && widget->typeComboBox->currentIndex() != value) {
                QSignalBlocker blocker(widget->typeComboBox);
                widget->typeComboBox->setCurrentIndex(value);
            }
            emit typeIndexChanged(value);
            // AbstractDelegateModel::stateFeedBack("/type", value);
        }

        bool send() const { return m_send; }
        void setSend(bool value)
        {
            if (!value) return;
            if (!m_send) {
                m_send = true;
                emit sendChanged(m_send);
                sendOSCMessage();
                m_send = false;
                emit sendChanged(m_send);
            } else {
                sendOSCMessage();
            }
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {

            switch (portIndex) {
            case 0:
                return "HOST";
            case 1:
                return "PORT";
            case 2:
                return "ADDRESS";
            case 3:
                return "VALUE";
            case 4:
                return "TRIGGER";
            default:
                break;
            }
            return "";
        }
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
            // FIXME: control may reach end of non-void function [-Wreturn-type]
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return inData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);

            switch (portIndex)
            {
                case 0:
                    setHost(textData->value().toString());
                    break;
                case 1:
                    setPort(textData->value().toInt());
                    break;
                case 2:
                    setAddress(textData->value().toString());
                    sendOSCMessage();
                    break;
                case 3:
                    setValue(textData->value().toString());
                    // setValue already calls sendOSCMessage()
                    break;
                case 4:
                    if(textData->value().toBool())
                    {
                        setSend(true);
                    }
                    break;
            }


        }

        // void stateFeedBack(const QString& oscAddress,QVariant value) override {
        //     // Removed override to use AbstractDelegateModel::stateFeedBack
        //     // which should handle OSC feedback correctly via GlobalEventBus or similar.
        //     AbstractDelegateModel::stateFeedBack(oscAddress, value);
        // }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = m_port;
            modelJson1["Host"] = m_host;
            modelJson1["Address"] = m_address;
            modelJson1["Type"] = m_typeIndex;
            modelJson1["Value"] = m_value;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setPort(v["Port"].toInt());
                setHost(v["Host"].toString());
                setAddress(v["Address"].toString());
                setTypeIndex(v["Type"].toInt());
                setValue(v["Value"].toString());
            }
        }


        QWidget *embeddedWidget() override {
            return widget;
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
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

    public slots:
        /**
         *  @brief 构建并发送OSC消息
         */
        void sendOSCMessage()
        {
            OSCMessage msg;
            msg.host = m_host;
            msg.port = m_port;
            msg.address = m_address;
            msg.type = widget->typeComboBox->currentText(); // Type string might still be useful from combo
            // OR derive type string from m_typeIndex if possible, but combo has text.
            // Let's keep accessing combo text for type name as we only store index.
            // Or better: store type names in a list if we want to be pure.
            // For now, accessing widget for READ-ONLY properties like items list is fine, 
            // but we should probably cache it if we want to be fully headless-capable.
            // However, type string is derived from type index.
            
            // To support headless, we should know what index 0, 1, 2 map to.
            // Assuming 0: Int, 1: Float, 2: String based on original switch.

            switch (m_typeIndex)
            {
            case 0: // Int
                msg.type = "Int";
                msg.value = m_value.toInt();
                break;
            case 1: // Float
                msg.type = "Float";
                msg.value = m_value.toDouble();
                break;
            case 2: // String
                msg.type = "String";
                msg.value = m_value;
                break;
            default:
                msg.type = "String";
                msg.value = m_value;
            }
            // AbstractDelegateModel::stateFeedBack("/send",true);
            emit onHasOSC(msg);
        }

    Q_SIGNALS:
        void hostChanged(const QString& value);
        void portChanged(int value);
        void addressChanged(const QString& value);
        void valueChanged(const QString& value);
        void typeIndexChanged(int value);
        void sendChanged(bool value);

        void onHasOSC(OSCMessage &data);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "host") {
                setHost(ev.payload.toString());
            } else if (localPath == "port") {
                setPort(ev.payload.toInt());
            } else if (localPath == "address") {
                setAddress(ev.payload.toString());
            } else if (localPath == "value") {
                setValue(ev.payload.toString());
            } else if (localPath == "type") {
                setTypeIndex(ev.payload.toInt());
            } else if (localPath == "send") {
                if (!ev.payload.isValid() || ev.payload.toBool()) {
                    setSend(true);
                }
            }
        }

    private:
        std::shared_ptr<VariableData> inData;
        OscOutInterface *widget;
        
        QString m_host = "127.0.0.1";
        int m_port = 8000;
        QString m_address = "/test";
        QString m_value = "0";
        int m_typeIndex = 0;
        bool m_send = false;

    };
}