#pragma once

#include "QtNodes/NodeDelegateModel"

#include <QtCore/QObject>

#include <iostream>
#include <QPushButton>
#include "QLayout"
#include "../NodeDataList.hpp"
#include "AudioDeviceOutInterface.hpp"
#include <portaudio.h>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class AudioDeviceOutDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    AudioDeviceOutDataModel():Data(std::make_shared<AudioNodeData2>()) {
        InPortCount =1;
        OutPortCount=0;
        CaptionVisible= false;
        Caption="Audio Device Out";
        WidgetEmbeddable=false;
        PortEditable= true;
        Resizable=false;
        getDeviceList();
        connect(widget->device_selector,&QComboBox::currentTextChanged,this, &AudioDeviceOutDataModel::selectDevice);
    }

    ~AudioDeviceOutDataModel() override= default;

public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = " "+QString::number(portIndex);
        QString out = QString::number(portIndex)+"  ";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }

    unsigned int nPorts(PortType portType) const override
    {

        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }
        return result;
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
            case PortType::In:
                return Data->type();
         
            case PortType::Out:

                break;

            case PortType::None:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return VariantData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        Q_UNUSED(port)
        return Data;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        auto d = std::dynamic_pointer_cast<AudioNodeData2>(data);

        if(d!= nullptr){

        }
        Q_EMIT dataUpdated(0);
    }

    QWidget *embeddedWidget() override
    {
        return widget;
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;

        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;

        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {

        }
    }

public slots:

    void getDeviceList(){
        PaError err;
        int     i, numDevices;
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
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( i );

            if (deviceInfo != nullptr && deviceInfo->maxOutputChannels>0)
            {
                deviceList << QString("%1: %2").arg(i).arg(deviceInfo->name);

            }
        }
        widget->device_selector->addItems(deviceList);

    }
    void selectDevice(const QString device)
    {
        widget->treeWidget->clear();
//        qDebug()<<device<<device.section(':', 0, 0).toInt();
        int deviceIndex=device.section(':', 0, 0).toInt();
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

        // 添加设备名称
        QTreeWidgetItem *nameItem = new QTreeWidgetItem(widget->treeWidget);
        nameItem->setText(0, "Device Name");
        nameItem->setText(1, deviceInfo->name);

        // 添加驱动类型
        QTreeWidgetItem *driverItem = new QTreeWidgetItem(widget->treeWidget);
        driverItem->setText(0, "Driver Type");
        driverItem->setText(1, hostApiInfo->name);
//
//        // 添加最大输出通道数
        QTreeWidgetItem *maxOutputChannelsItem = new QTreeWidgetItem(widget->treeWidget);
        maxOutputChannelsItem->setText(0, "Max Output Channels");
        maxOutputChannelsItem->setText(1, QString::number(deviceInfo->maxOutputChannels));
//
        // 添加最大输入通道数
        QTreeWidgetItem *maxInputChannelsItem = new QTreeWidgetItem(widget->treeWidget);
        maxInputChannelsItem->setText(0, "Max Input Channels");
        maxInputChannelsItem->setText(1, QString::number(deviceInfo->maxInputChannels));

        // 添加默认采样率
        QTreeWidgetItem *defaultSampleRateItem = new QTreeWidgetItem(widget->treeWidget);
        defaultSampleRateItem->setText(0, "Default Sample Rate");
        defaultSampleRateItem->setText(1, QString::number(deviceInfo->defaultSampleRate));

        // 检查是否为默认输入设备
        PaDeviceIndex defaultInputDevice = Pa_GetDefaultInputDevice();
        QTreeWidgetItem *isDefaultInputDeviceItem = new QTreeWidgetItem(widget->treeWidget);
        isDefaultInputDeviceItem->setText(0, "Is Default Input Device");
        isDefaultInputDeviceItem->setText(1, (deviceIndex == defaultInputDevice) ? "Yes" : "No");

        // 检查是否为默认输出设备
        PaDeviceIndex defaultOutputDevice = Pa_GetDefaultOutputDevice();
        QTreeWidgetItem *isDefaultOutputDeviceItem = new QTreeWidgetItem(widget->treeWidget);
        isDefaultOutputDeviceItem->setText(0, "Is Default Output Device");
        isDefaultOutputDeviceItem->setText(1, (deviceIndex == defaultOutputDevice) ? "Yes" : "No");
        // 展开所有项
        widget->treeWidget->expandAll();
    }

private:
    std::shared_ptr<AudioNodeData2> Data;

    AudioDeviceOutInterface *widget=new AudioDeviceOutInterface();



};

//==============================================================================

