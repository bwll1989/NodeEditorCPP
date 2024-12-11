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
#include "HttpClient.h"
#include "QTimer"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class MpvControllerDataModel : public NodeDelegateModel
{
    Q_OBJECT


public:

    MpvControllerDataModel(){
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Mpv Controller";
        WidgetEmbeddable=true;
        Resizable=true;
        client=new HttpClient();
        widget=new MpvControllerInterface();
        timer=new QTimer();
        timer->setInterval(900);
        connect(widget->Play, &QPushButton::clicked, this, &MpvControllerDataModel::onPlay);
        connect(widget->Fullscreen, &QPushButton::clicked, this, &MpvControllerDataModel::onFullscreen);
        connect(client,&HttpClient::getSatus,widget->browser,&QPropertyBrowser::buildPropertiesFromJson);
        connect(timer,&QTimer::timeout,this,&MpvControllerDataModel::getStatus);
        timer->start();
    }
    ~MpvControllerDataModel(){
        delete widget;
        delete client;

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
        return std::make_shared<VariableData>(widget->browser->exportToMap());
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

private Q_SLOTS:

    void onPlay()
    {
//        button->setText(QString::number(string));
        hostAddress=widget->browser->getProperties("Host").toString();
        client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/toggle_pause"));
    }
    void onFullscreen()
    {
//        button->setText(QString::number(string));
        hostAddress=widget->browser->getProperties("Host").toString();
        client->sendPostRequest(QUrl("http://"+hostAddress+":8080/api/fullscreen"));
    }
    void getStatus(){
        hostAddress=widget->browser->getProperties("Host").toString();
        client->sendGetRequest(QUrl("http://"+hostAddress+":8080/api/status"));
        Q_EMIT dataUpdated(0);
    }
private:

    HttpClient *client;
    QString hostAddress;
    MpvControllerInterface *widget;
    QTimer *timer;

};
