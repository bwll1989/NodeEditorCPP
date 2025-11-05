#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QUrl>
#include "MpvControllerInterface.hpp"
#include <QtWidgets/QPushButton>
#include <QtCore/qglobal.h>

#include "ConstantDefines.h"
#include "HttpClient.h"
#include "QTimer"
#include "OSCSender/OSCSender.h"
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
    class MpvControllerDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    public:
        MpvControllerDataModel(){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Mpv Controller";
            WidgetEmbeddable=false;
            Resizable=false;
            client=new HttpClient();
            widget=new MpvControllerInterface();
            NodeDelegateModel::registerOSCControl("/play",widget->Play);
            NodeDelegateModel::registerOSCControl("/fullscreen",widget->Fullscreen);
            timer=new QTimer();
            timer->setInterval(900);
            timer->setSingleShot(true);
            connect(widget->Play, &QPushButton::clicked, this, &MpvControllerDataModel::onPlay);
            connect(widget->Fullscreen, &QPushButton::clicked, this, &MpvControllerDataModel::onFullscreen);
            connect(widget->playlist_prev, &QPushButton::clicked, this, &MpvControllerDataModel::onplaylist_prev);
            connect(widget->playlist_next, &QPushButton::clicked, this, &MpvControllerDataModel::onplaylist_next);
            connect(widget->speedAdd, &QPushButton::clicked, this, &MpvControllerDataModel::speedAdd);
            connect(widget->speedSub, &QPushButton::clicked, this, &MpvControllerDataModel::speedSub);
            connect(widget->speedReset, &QPushButton::clicked, this, &MpvControllerDataModel::speedReset);
            connect(client,&HttpClient::getSatus,this,[this](QJsonObject state){
                this->status=&state;
                emit dataUpdated(0);
                });
            connect(widget->volumeEditor,&QDoubleSpinBox::valueChanged,this,&MpvControllerDataModel::setValume);
            connect(timer,&QTimer::timeout,this,&MpvControllerDataModel::getStatus);

        }
        ~MpvControllerDataModel(){
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
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return VariableData().type();

        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(status);
        }
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

            if (data== nullptr){
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if (!textData->isEmpty()) {
                //            qDebug()<<textData->value().toString();
                client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/"+textData->value().toString()));
                Q_EMIT dataUpdated(portIndex);
            }


        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            //        modelJson1["val"] = QString::number(button->isChecked());
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                //            button->setChecked(v["val"].toBool(false));
                //            button->setText(v["val"].toString());
            }
        }
        QWidget *embeddedWidget() override{return widget;}

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

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    private Q_SLOTS:

        void onPlay()
        {
            //        button->setText(QString::number(string));
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/toggle_pause"));
            timer->start();
        }
        void onFullscreen()
        {
            //        button->setText(QString::number(string));
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/fullscreen"));
            timer->start();
        }
        void getStatus(){
            hostAddress=widget->hostEdit->text();
            client->sendGetRequest(QUrl("http://"+hostAddress+":8080/api/status"));
        }

        void onplaylist_prev()
        {
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/playlist_prev"));
            timer->start();
        }
        void onplaylist_next()
        {
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/playlist_next"));
            timer->start();
        }
        void speedAdd(){
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/speed_adjust/1.1"));
            timer->start();
            }
        void speedSub()
        {
           
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/speed_adjust/0.9"));
            timer->start();
        }
        void speedReset()
        {
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/speed_set"));
            timer->start();
        }
        void setValume(float valume){
            hostAddress=widget->hostEdit->text();
            client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/set_volume/"+QString::number(valume)));
            timer->start();
        }

    private:
        HttpClient *client;
        QString hostAddress;
        MpvControllerInterface *widget;
        QTimer *timer;
        QJsonObject *status=new QJsonObject();
    };
}