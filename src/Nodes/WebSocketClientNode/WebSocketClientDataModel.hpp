#pragma once

#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "WebSocketClientInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QUrl>

#include "ConstantDefines.h"
#include "Common/Devices/WebSocketClient/WebSocketClient.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class WebSocketClientDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        WebSocketClientDataModel() {
            InPortCount = 3;
            OutPortCount = 3;
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();
            AbstractDelegateModel::registerOSCControl("/host", widget->hostUrlEdit);
            AbstractDelegateModel::registerOSCControl("/value", widget->valueEdit);
            AbstractDelegateModel::registerOSCControl("/send", widget->send);
            AbstractDelegateModel::registerOSCControl("/status", widget->statusButton);
            m_client = new WebSocketClient(this,QUrl(widget->hostUrlEdit->text()));

            connect(widget->send, &QPushButton::clicked, this, &WebSocketClientDataModel::onSendClicked);
            connect(widget->hostUrlEdit, &QLineEdit::textChanged, this, &WebSocketClientDataModel::onHostChanged);
            connect(this, &WebSocketClientDataModel::connectToUrl, m_client, &WebSocketClient::connectToServer);
            connect(this, &WebSocketClientDataModel::sendMessage, m_client, &WebSocketClient::sendMessage);
            connect(m_client, &WebSocketClient::recMsg, this, &WebSocketClientDataModel::recMsg);
            connect(m_client, &WebSocketClient::isReady, this, &WebSocketClientDataModel::onConnected);
            // connect(m_client, &WebSocketClient::errorOccurred, this, &WebSocketClientDataModel::onError);
        }
        ~WebSocketClientDataModel() {
        }
    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
            case 0:
                    return "URL";
            case 1:
                    return "VALUE";
            case 2:
                    return "TRIGGER";
            default:
                    break;
                }
            case PortType::Out:
                switch (portIndex)
                {
            case 0:
                    return "RESULT";
            case 1:
                    return "URL";
            case 2:
                    return "STRING";
            default:
                    break;
                }
            default:
                break;
            }
            return "";
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex)
            {
            case 0:
                return m_outData;
            case 1:
                return std::make_shared<VariableData>(m_outData->value("url").toString());
            case 2:
                return std::make_shared<VariableData>(m_outData->value());
            default:
                return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
            case 0:
                widget->hostUrlEdit->setText(m_inData->value().toString());
                // emit connectToUrl(QUrl(widget->hostUrlEdit->text()));
                break;
            case 1:
                widget->valueEdit->setText(m_inData->value().toString());
                emit sendMessage(widget->valueEdit->text(), widget->messageType->currentIndex(),widget->format->currentIndex());
                break;
            case 2:
                emit sendMessage(widget->valueEdit->text(), widget->messageType->currentIndex(),widget->format->currentIndex());
                break;
            default:
                break;
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["URL"] = widget->hostUrlEdit->text();
            modelJson1["Value"] = widget->valueEdit->text();
            modelJson1["Format"] = widget->format->currentIndex();
            modelJson1["MessageType"] = widget->messageType->currentIndex();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->hostUrlEdit->setText(v["URL"].toString());
                widget->valueEdit->setText(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
                widget->messageType->setCurrentIndex(v["MessageType"].toInt());
            }
        }

    public slots:
        void onSendClicked() {
            emit sendMessage(widget->valueEdit->text(), widget->messageType->currentIndex(),widget->format->currentIndex());
        }
        void onHostChanged() {
            emit connectToUrl(QUrl(widget->hostUrlEdit->text()));
        }

        void recMsg(const QVariantMap &msg)
        {
            m_outData=std::make_shared<VariableData>(msg);
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
        }

        void onConnected(bool isReady) {
            // 可选：更新UI状态
            widget->send->setEnabled(isReady);
            widget->statusButton->setChecked(isReady);
            widget->statusButton->setText(isReady?"Connected":"Disconnected");
            widget->statusButton->setStyleSheet(isReady?"color: green; font-weight: bold;":"color: red; font-weight: bold;");
        }

    signals:
        void connectToUrl(const QUrl &url);
        void sendMessage(const QString &msg,const int &messageType = 0 ,const int &format = 0);
    private:
        WebSocketClientInterface *widget = new WebSocketClientInterface();
        WebSocketClient *m_client;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
    };
}