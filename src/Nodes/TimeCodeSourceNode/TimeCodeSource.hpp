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
#include <QJsonObject>
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
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
            OutPortCount = 4;
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = true;
            PortEditable = false;
            Caption = "TimeCode Source";

            // 连接 Widget 的信号到 receiver

            // 连接 receiver 的信号到 Widget
            connect(receiver, &LTCReceiver::statusChanged, _label, &TimeCodeInterface::setStatus);
            // connect(receiver, &LTCReceiver::newFrame, _label, &TimeCodeInterface::setTimeStamp);
            connect(receiver, &LTCReceiver::newFrame, this, &TimeCodeSource::onReceivedTimecodeFrame);
            // connect(receiver, &LTCReceiver::newFrame, [this](TimeCodeFrame frame) {
            //     *_timeCodeFrame = timecode_frame_add(frame, _label->timeCodeOffsetSpinBox->value());
            //     Q_EMIT dataUpdated(0);
            //     Q_EMIT dataUpdated(1);
            //     Q_EMIT dataUpdated(2);
            //     Q_EMIT dataUpdated(3);
            // });
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

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portType);
            Q_UNUSED(portIndex);
            return VariableData().type();
        }
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return "INPUT "+QString::number(portIndex);
            case PortType::Out:
                switch (portIndex)
                {
                case 0:
                        return "FRAMES";
                case 1:
                        return "SECONDS";
                case 2:
                        return "MINUTES";
                case 3:
                        return "HOURS";
                default:
                        break;
                    }

            }
            return "";
        }
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port)
            {
            case 0:
                return std::make_shared<VariableData>(_timeCodeFrame.frames);
            case 1:
                return std::make_shared<VariableData>(_timeCodeFrame.seconds);
            case 2:
                return std::make_shared<VariableData>(_timeCodeFrame.minutes);
            case 3:
                return std::make_shared<VariableData>(_timeCodeFrame.hours);
            default:
                return std::make_shared<VariableData>();
            }
        }

        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            Q_UNUSED(port);
            Q_EMIT dataUpdated(0);
        }

        QWidget *embeddedWidget() override { return _label; }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["device"] = _label->deviceComboBox->currentText();
            modelJson["channel"] = _label->channelComboBox->currentIndex();
            modelJson["offset"] = _label->timeCodeOffsetSpinBox->value();
            return modelJson;
        }
        void load(QJsonObject const& jsonObj) override
        {
            const QJsonValue device = jsonObj["device"];
            if (!device.isUndefined()) {
                _label->deviceComboBox->setCurrentText(device.toString());
            }
            const QJsonValue channel = jsonObj["channel"];
            if (!channel.isUndefined()) {
                _label->channelComboBox->setCurrentIndex(channel.toInt());
            }
        }
    public slots:
        void deviceChanged()
        {

            if (receiver) {
                receiver->start(_label->deviceComboBox->currentText());
            }
        }
        void onReceivedTimecodeFrame(TimeCodeFrame frame)
        {
            _timeCodeFrame=timecode_frame_add(frame,_label->timeCodeOffsetSpinBox->value());
            _label->setTimeStamp(frame);
            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            emit dataUpdated(3);
        }
    private:
        TimeCodeInterface* _label;
        LTCReceiver* receiver;
        TimeCodeFrame _timeCodeFrame;
    };
}