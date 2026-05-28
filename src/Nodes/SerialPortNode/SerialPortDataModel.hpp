#pragma once

#include <QtCore/QObject>
#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "SerialPortInterface.hpp"
#include <QtWidgets/QLineEdit>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QSerialPort>
#include "Common/Devices/SerialPort/SerialPort.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes
{
    class SerialPortDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(int baudRate READ getBaudRate WRITE setBaudRate NOTIFY baudRateChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(bool connected READ getConnected WRITE setConnected NOTIFY connectedChanged)

    public:
        SerialPortDataModel()
        {
            InPortCount = 2;
            OutPortCount = 1;
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;

            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "port";
                b.control = widget->portCombo;
                AbstractDelegateModel::registerExternalBinding("/port", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "baudRate";
                b.control = widget->baudRateSpinBox;
                AbstractDelegateModel::registerExternalBinding("/baudRate", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "value";
                b.control = widget->valueEdit;
                AbstractDelegateModel::registerExternalBinding("/value", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "connected";
                b.control = widget->statusButton;
                AbstractDelegateModel::registerExternalBinding("/connected", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "send";
                b.control = widget->sendButton;
                AbstractDelegateModel::registerExternalBinding("/send", this, b);
            }

            connect(this, &SerialPortDataModel::openSerialPort, client, &SerialPort::openPort, Qt::QueuedConnection);
            connect(this, &SerialPortDataModel::closeSerialPort, client, &SerialPort::closePort, Qt::QueuedConnection);
            connect(this, &SerialPortDataModel::sendSerialMessage, client, &SerialPort::sendMessage, Qt::QueuedConnection);
            connect(client, &SerialPort::recMsg, this, &SerialPortDataModel::recMsg, Qt::QueuedConnection);
            connect(client, &SerialPort::isReady, this, [this](bool ready) {
                if (m_connected != ready) {
                    m_connected = ready;
                    emit connectedChanged(m_connected);
                }

                QSignalBlocker blocker(widget->statusButton);
                widget->statusButton->setText(ready ? "Connected" : "Disconnect");
                widget->statusButton->setChecked(ready);
                widget->statusButton->setStyleSheet(ready ? "color: green; font-weight: bold;"
                                                          : "color: red; font-weight: bold;");
                widget->sendButton->setEnabled(ready);
            }, Qt::QueuedConnection);

            connect(widget->portCombo, &QComboBox::currentTextChanged, this, [this](const QString &port) {
                setPort(port);
            });
            connect(widget->baudRateSpinBox, &IntDragValueWidget::valueChanged, this, &SerialPortDataModel::setBaudRate);
            connect(widget->dataBitsCombo, &QComboBox::currentIndexChanged, this, [this]() { openCurrentPort(); });
            connect(widget->parityCombo, &QComboBox::currentIndexChanged, this, [this]() { openCurrentPort(); });
            connect(widget->stopBitsCombo, &QComboBox::currentIndexChanged, this, [this]() { openCurrentPort(); });
            connect(widget->valueEdit, &QLineEdit::editingFinished, this, [this]() {
                setValue(widget->valueEdit->text());
            });
            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                sendMessage();
            });

            m_port = widget->portCombo->currentText();
            m_baudRate = widget->baudRateSpinBox->value();
            openCurrentPort();
        }

        ~SerialPortDataModel()
        {
            if (client) {
                client->closePort();
                delete client;
                client = nullptr;
            }
        }

        QString getPort() const { return m_port; }
        void setPort(const QString &port)
        {
            if (m_port == port) {
                return;
            }
            m_port = port;
            {
                QSignalBlocker blocker(widget->portCombo);
                const int index = widget->portCombo->findText(m_port);
                if (index >= 0) {
                    widget->portCombo->setCurrentIndex(index);
                } else {
                    widget->portCombo->addItem(m_port);
                    widget->portCombo->setCurrentText(m_port);
                }
            }
            openCurrentPort();
            emit portChanged(m_port);
        }

        int getBaudRate() const { return m_baudRate; }
        void setBaudRate(int baudRate)
        {
            if (m_baudRate == baudRate) {
                return;
            }
            m_baudRate = baudRate;
            {
                QSignalBlocker blocker(widget->baudRateSpinBox);
                widget->baudRateSpinBox->setValue(m_baudRate);
            }
            openCurrentPort();
            emit baudRateChanged(m_baudRate);
        }

        QString getValue() const { return m_value; }
        void setValue(const QString &value)
        {
            m_value = value;
            QSignalBlocker blocker(widget->valueEdit);
            widget->valueEdit->setText(m_value);
            emit valueChanged(m_value);
        }

        bool getConnected() const { return m_connected; }
        void setConnected(bool connected)
        {
            Q_UNUSED(connected)
        }

        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/baudRate"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/connected"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0: return "VALUE";
                case 1: return "TRIGGER";
                default: break;
                }
                break;
            case PortType::Out:
                switch (portIndex) {
                case 0: return "REC";
                default: break;
                }
                break;
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
            switch (portIndex) {
            case 0: return m_outData;
            default: return nullptr;
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
                setValue(m_inData->value().toString());
                sendMessage();
                break;
            case 1:
                sendMessage();
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
            QJsonObject values;
            values["Port"] = m_port;
            values["BaudRate"] = m_baudRate;
            values["Value"] = m_value;
            values["DataBits"] = widget->dataBitsCombo->currentIndex();
            values["Parity"] = widget->parityCombo->currentIndex();
            values["StopBits"] = widget->stopBitsCombo->currentIndex();
            values["Format"] = widget->formatCombo->currentIndex();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                setPort(v["Port"].toString());
                setBaudRate(v["BaudRate"].toInt());
                setValue(v["Value"].toString());
                widget->dataBitsCombo->setCurrentIndex(v["DataBits"].toInt());
                widget->parityCombo->setCurrentIndex(v["Parity"].toInt());
                widget->stopBitsCombo->setCurrentIndex(v["StopBits"].toInt());
                widget->formatCombo->setCurrentIndex(v["Format"].toInt());
                openCurrentPort();
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index)
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            case PortType::None:
                break;
            }
            return ConnectionPolicy::One;
        }

    public slots:
        void recMsg(const QVariantMap &msg)
        {
            m_outData = std::make_shared<VariableData>(msg);
            Q_EMIT dataUpdated(0);
        }

        void sendMessage()
        {
            AbstractDelegateModel::stateFeedBack("/send", true);
            emit sendSerialMessage(m_value, widget->formatCombo->currentIndex());
            AbstractDelegateModel::stateFeedBack("/send", false);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "port") {
                setPort(ev.payload.toString());
            } else if (localPath == "baudRate") {
                setBaudRate(ev.payload.toInt());
            } else if (localPath == "value") {
                setValue(ev.payload.toString());
            } else if (localPath == "connected") {
                setConnected(ev.payload.toBool());
            } else if (localPath == "send") {
                sendMessage();
            }
        }

    signals:
        void portChanged(QString port);
        void baudRateChanged(int baudRate);
        void valueChanged(QString value);
        void connectedChanged(bool connected);

        void openSerialPort(const QString &portName,
                            int baudRate,
                            QSerialPort::DataBits dataBits,
                            QSerialPort::Parity parity,
                            QSerialPort::StopBits stopBits);
        void closeSerialPort();
        void sendSerialMessage(const QString &message, int format = 0);

    private:
        void openCurrentPort()
        {
            if (m_port.isEmpty()) {
                return;
            }
            emit openSerialPort(m_port,
                                m_baudRate,
                                static_cast<QSerialPort::DataBits>(widget->dataBitsCombo->currentData().toInt()),
                                static_cast<QSerialPort::Parity>(widget->parityCombo->currentData().toInt()),
                                static_cast<QSerialPort::StopBits>(widget->stopBitsCombo->currentData().toInt()));
        }

        SerialPortInterface *widget = new SerialPortInterface();
        SerialPort *client = new SerialPort();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;

        QString m_port = "COM1";
        int m_baudRate = 9600;
        QString m_value;
        bool m_connected = false;
    };
}
