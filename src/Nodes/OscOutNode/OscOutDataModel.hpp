#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "Common/Devices/OSCSender/OSCSender.h"
#include "OscOutInterface.hpp"
#include <QVariantMap>
#include "QThread"
#include "spdlog/fmt/bundled/base.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class OscOutDataModel : public NodeDelegateModel
    {

        Q_OBJECT
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

        void setup() {
            widget=new OscOutInterface();
            NodeDelegateModel::registerOSCControl("/host",widget->hostLineEdit);
            NodeDelegateModel::registerOSCControl("/port",widget->portSpinBox);
            NodeDelegateModel::registerOSCControl("/address",widget->addressLineEdit);
            NodeDelegateModel::registerOSCControl("/value",widget->valueLineEdit);
            // connect(widget,&OscOutInterface::hostChanged,OSC_Sender,&OSCSender::setHost);
            connect(this,&OscOutDataModel::onHasOSC,this,[this](OSCMessage &msg){
                OSCSender::instance()->sendOSCMessageWithQueue(msg);
            });

            connect(widget->valueLineEdit,&QLineEdit::textChanged,this,&OscOutDataModel::sendOSCMessage);

            connect(widget->sendButton,&QPushButton::clicked,this,&OscOutDataModel::sendOSCMessage);

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
                    widget->hostLineEdit->setText(textData->value().toString());
                    break;
                case 1:
                    widget->portSpinBox->setValue(textData->value().toInt());
                    break;
                case 2:
                    widget->addressLineEdit->setText(textData->value().toString());
                    sendOSCMessage();
                    break;
                case 3:
                    widget->valueLineEdit->setText(textData->value().toString());
                    sendOSCMessage();
                    break;
                case 4:
                    if(textData->value().toBool())
                    {
                        sendOSCMessage();
                    }
                    break;
            }


        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = widget->portSpinBox->value();
            modelJson1["Host"] = widget->hostLineEdit->text();
            modelJson1["Address"] = widget->addressLineEdit->text();
            modelJson1["Type"] = widget->typeComboBox->currentIndex();
            modelJson1["Value"] = widget->valueLineEdit->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->portSpinBox->setValue(v["Port"].toInt());
                widget->hostLineEdit->setText(v["Host"].toString());
                widget->addressLineEdit->setText(v["Address"].toString());
                widget->typeComboBox->setCurrentIndex(v["Type"].toInt());
                widget->valueLineEdit->setText(v["Value"].toString());
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
            msg.host = widget->hostLineEdit->text();
            msg.port = widget->portSpinBox->value();
            msg.address = widget->addressLineEdit->text();
            msg.type = widget->typeComboBox->currentText();
            switch (widget->typeComboBox->currentIndex())
            {
            case 0:
                msg.value= widget->valueLineEdit->text().toInt();
                break;
            case 1:
                msg.value= widget->valueLineEdit->text().toDouble();
                break;
            case 2:
                msg.value= widget->valueLineEdit->text();
            }
            emit onHasOSC(msg);
        }

    Q_SIGNALS:

        void onHasOSC(OSCMessage &data);

    private:
        std::shared_ptr<VariableData> inData;
        OscOutInterface *widget;

    };
}