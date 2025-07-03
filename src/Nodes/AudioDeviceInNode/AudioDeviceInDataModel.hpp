#pragma once

#include "QtNodes/NodeDelegateModel"

#include <QtCore/QObject>

#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "../NodeDataList.hpp"
#include "AudioDeviceInInterface.hpp"
#include <portaudio.h>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
namespace Nodes
{
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    class AudioDeviceInDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        AudioDeviceInDataModel():Data(std::make_shared<ClientNodeData>()) {

            get_device_list();
            //        connect(widget->button,&QPushButton::clicked,this, &AudioDeviceOutDataModel::get_device_list);
        }

        ~AudioDeviceInDataModel() override= default;
        [[nodiscard]] QString caption() const override { return Node_Name; }

        bool captionVisible() const override { return true; }

        QString name() const override { return QStringLiteral("audio device in"); }
        bool resizable() const override{return isResizeable;}

        unsigned int nPorts(PortType portType) const override
        {

            unsigned int result = 1;

            switch (portType) {
            case PortType::In:
                result = 1;
                break;

            case PortType::Out:
                result = 1;

            default:
                break;
            }

            return result;
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
            case 0:
                    return Data->type();
                }
                break;


            case PortType::Out:
                switch (portIndex) {
            case 0:
                    return ProjectorNodeData().type();

                }
                break;

            case PortType::None:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return Data->type();
        }

        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            Q_UNUSED(port)
            return Data;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            auto d = std::dynamic_pointer_cast<ScreenNodeData>(data);

            if(d!= nullptr){
                Data->NodeValues.name=d->NodeValues.name;
                widget->namelabel->setText(Data->NodeValues.name);
            }
            Q_EMIT dataUpdated(0);
        }
        QWidget *embeddedWidget() override
        {

            widget->setMinimumSize(100, 50);
            widget->setMaximumSize(200,100);
            return widget;
        }

        bool widgetEmbeddable() const override { return isEmbeddable; }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["name"] = Data->NodeValues.name;
            modelJson1["path"] = Data->NodeValues.path;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {

                Data->NodeValues.name = v["name"].toString();
                Data->NodeValues.path = v["path"].toString();

            }
        }

    public slots:
        void get_device_list(){
            PaError err;
            int     i, numDevices;
            const   PaDeviceInfo *deviceInfo;
            QStringList deviceList;
            err = Pa_Initialize();
            //        widget->device_selector->clear();
            if( err != paNoError )
            {
                widget->namelabel->setText("INIT ERROR");
                Pa_Terminate();
            }
            numDevices = Pa_GetDeviceCount();
            if( numDevices < 0 )
            {
                widget->namelabel->setText("NO DEVICES");
                Pa_Terminate();
            }
            for( i=0; i<numDevices; i++ )
            {
                deviceInfo = Pa_GetDeviceInfo( i );

                if (deviceInfo != nullptr && deviceInfo->maxInputChannels>0)
                {
                    deviceList << QString("%1: %2").arg(i).arg(deviceInfo->name);


                }
            }
            widget->device_selector->addItems(deviceList);
            Pa_Terminate();

        }


    private:
        std::shared_ptr<ClientNodeData> Data;
        AudioDeviceInInterface *widget=new AudioDeviceInInterface();
        QString Node_Name="Audio Device In";
        bool isEmbeddable=false;
        bool isResizeable=false;

    };
}
//==============================================================================

