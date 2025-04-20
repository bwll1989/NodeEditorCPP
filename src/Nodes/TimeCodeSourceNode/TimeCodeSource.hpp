#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "DataTypes/NodeDataList.hpp"

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/qglobal.h>
#include "TimeCodeInterface.h"
#include "../../Common/Devices/LtcReceiver/ltcreceiver.h"
#include <QComboBox>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class TimeCodeSource : public NodeDelegateModel
{
    Q_OBJECT

public:
    TimeCodeSource()
        : _label(new TimeCodeInterface()),
          receiver(new LTCReceiver())
    {
        InPortCount = 1;
        OutPortCount = 1;
        CaptionVisible = true;
        WidgetEmbeddable = true;
        Resizable = true;
        PortEditable = false;
        Caption = "TimeCode Source";
        
        // 连接 Widget 的信号到 receiver
       
        // 连接 receiver 的信号到 Widget
        connect(receiver, &LTCReceiver::statusChanged, _label, &TimeCodeInterface::setStatus);
        connect(receiver, &LTCReceiver::newFrame, _label, &TimeCodeInterface::setTimeStamp);
        connect(_label->deviceComboBox, &QComboBox::currentIndexChanged, this, &TimeCodeSource::deviceChanged);
        connect(_label->channelComboBox, &QComboBox::currentIndexChanged, receiver, &LTCReceiver::setChannel);
    }

    ~TimeCodeSource()
    {
        if (receiver) {
            receiver->stop();
            delete receiver;
        }
        delete _label;
    }

public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "In "+QString::number(portIndex);
        QString out = "Out "+QString::number(portIndex);
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


public:

    unsigned int nPorts(PortType const portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result =0;
                break;

            case PortType::Out:
                result = 0;

            default:
                break;
        }

        return result;
    }

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
    {
        Q_UNUSED(portType);
        Q_UNUSED(portIndex);
        return PixmapData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return _nodeData;
    }

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
    {
        Q_UNUSED(port);
        Q_EMIT dataUpdated(0);
    }
   
    QWidget *embeddedWidget() override { return _label; }

    bool resizable() const override { return true; }

public slots:
    void deviceChanged()
    {
        
        if (receiver) {
            receiver->start(_label->deviceComboBox->currentText());
        }
    }

protected:
   
private:
    TimeCodeInterface* _label;
    LTCReceiver* receiver;
    std::shared_ptr<NodeData> _nodeData;
};