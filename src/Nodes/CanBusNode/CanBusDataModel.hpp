#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QGridLayout>
#include <QtWidgets/QLineEdit>

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>

#include "CanBusInterface.hpp"
#include "Common/Devices/CanBus/CanBus.h"
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
    class CanBusDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int deviceIndex READ getDeviceIndex WRITE setDeviceIndex NOTIFY deviceIndexChanged)
        Q_PROPERTY(int bitrate READ getBitrate WRITE setBitrate NOTIFY bitrateChanged)
        Q_PROPERTY(QString canId READ getCanId WRITE setCanId NOTIFY canIdChanged)
        Q_PROPERTY(QString data READ getData WRITE setData NOTIFY dataChanged)
        Q_PROPERTY(bool connected READ getConnected WRITE setConnected NOTIFY connectedChanged)

    public:
        CanBusDataModel()
        {
            InPortCount = 3;
            OutPortCount = 1;
            CaptionVisible = true;
            PortEditable = false;
            Caption = "CAN Bus";
            WidgetEmbeddable = false;
            Resizable = false;

            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "deviceIndex";
                b.control = widget->deviceCombo;
                AbstractDelegateModel::registerExternalBinding("/deviceIndex", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "bitrate";
                b.control = widget->bitrateCombo;
                AbstractDelegateModel::registerExternalBinding("/bitrate", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "canId";
                b.control = widget->canIdEdit;
                AbstractDelegateModel::registerExternalBinding("/canId", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "data";
                b.control = widget->dataEdit;
                AbstractDelegateModel::registerExternalBinding("/data", this, b);
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

            connect(this, &CanBusDataModel::openCanBus, client, &CanBus::openDevice, Qt::QueuedConnection);
            connect(this, &CanBusDataModel::closeCanBus, client, &CanBus::closeDevice, Qt::QueuedConnection);
            connect(this, &CanBusDataModel::sendCanFrame, client, &CanBus::sendFrame, Qt::QueuedConnection);
            connect(client, &CanBus::recMsg, this, &CanBusDataModel::recMsg, Qt::QueuedConnection);
            connect(client, &CanBus::errorOccurred, this, [this](const QString &msg) {
                Q_UNUSED(msg)
            }, Qt::QueuedConnection);
            connect(client, &CanBus::isReady, this, [this](bool ready) {
                if (m_connected != ready) {
                    m_connected = ready;
                    emit connectedChanged(m_connected);
                }

                QSignalBlocker blocker(widget->statusButton);
                widget->statusButton->setText(ready ? QStringLiteral("Connected") : QStringLiteral("Disconnected"));
                widget->statusButton->setChecked(ready);
                widget->statusButton->setStyleSheet(ready ? QStringLiteral("color: green; font-weight: bold;")
                                                          : QStringLiteral("color: red; font-weight: bold;"));
                widget->sendButton->setEnabled(ready);
            }, Qt::QueuedConnection);

            connect(widget->deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                setDeviceIndex(widget->deviceCombo->currentData().toInt());
            });
            connect(widget->bitrateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                setBitrate(widget->bitrateCombo->currentData().toInt());
            });
            connect(widget->channelSpin, &IntDragValueWidget::valueChanged, this, [this](int) {
                openCurrentDevice();
            });
            connect(widget->modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                openCurrentDevice();
            });
            connect(widget->canIdEdit, &QLineEdit::editingFinished, this, [this]() {
                setCanId(widget->canIdEdit->text());
            });
            connect(widget->dataEdit, &QLineEdit::editingFinished, this, [this]() {
                setData(widget->dataEdit->text());
            });
            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                sendMessage();
            });

            m_deviceIndex = widget->deviceCombo->currentData().toInt();
            m_bitrate = widget->bitrateCombo->currentData().toInt();
            m_canId = widget->canIdEdit->text();
            m_data = widget->dataEdit->text();
            openCurrentDevice();
        }

        ~CanBusDataModel()
        {
            if (client) {
                client->closeDevice();
                delete client;
                client = nullptr;
            }
        }

        int getDeviceIndex() const { return m_deviceIndex; }
        void setDeviceIndex(int index)
        {
            if (m_deviceIndex == index) {
                return;
            }
            m_deviceIndex = index;
            {
                QSignalBlocker blocker(widget->deviceCombo);
                const int comboIndex = widget->deviceCombo->findData(m_deviceIndex);
                if (comboIndex >= 0) {
                    widget->deviceCombo->setCurrentIndex(comboIndex);
                }
            }
            openCurrentDevice();
            emit deviceIndexChanged(m_deviceIndex);
        }

        int getBitrate() const { return m_bitrate; }
        void setBitrate(int bitrate)
        {
            if (m_bitrate == bitrate) {
                return;
            }
            m_bitrate = bitrate;
            {
                QSignalBlocker blocker(widget->bitrateCombo);
                const int comboIndex = widget->bitrateCombo->findData(m_bitrate);
                if (comboIndex >= 0) {
                    widget->bitrateCombo->setCurrentIndex(comboIndex);
                }
            }
            openCurrentDevice();
            emit bitrateChanged(m_bitrate);
        }

        QString getCanId() const { return m_canId; }
        void setCanId(const QString &canId)
        {
            m_canId = canId;
            QSignalBlocker blocker(widget->canIdEdit);
            widget->canIdEdit->setText(m_canId);
            emit canIdChanged(m_canId);
        }

        QString getData() const { return m_data; }
        void setData(const QString &data)
        {
            m_data = data;
            QSignalBlocker blocker(widget->dataEdit);
            widget->dataEdit->setText(m_data);
            emit dataChanged(m_data);
        }

        bool getConnected() const { return m_connected; }
        void setConnected(bool connected)
        {
            Q_UNUSED(connected)
        }

        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/deviceIndex"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/bitrate"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/canId"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/data"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/connected"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0: return QStringLiteral("ID");
                case 1: return QStringLiteral("DATA");
                case 2: return QStringLiteral("TRIGGER");
                default: break;
                }
                break;
            case PortType::Out:
                switch (portIndex) {
                case 0: return QStringLiteral("FRAME");
                default: break;
                }
                break;
            default:
                break;
            }
            return QString();
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
            if (!m_inData) {
                return;
            }

            switch (portIndex) {
            case 0: {
                const QVariantMap map = m_inData->asMap();
                if (map.contains(QStringLiteral("id"))) {
                    setCanId(map.value(QStringLiteral("id")).toString());
                } else {
                    setCanId(m_inData->asString());
                }
                if (map.contains(QStringLiteral("data"))) {
                    setData(map.value(QStringLiteral("data")).toString());
                }
                if (map.contains(QStringLiteral("extended"))) {
                    widget->extendedCheck->setChecked(map.value(QStringLiteral("extended")).toBool());
                }
                sendMessage();
                break;
            }
            case 1:
                setData(m_inData->asString());
                sendMessage();
                break;
            case 2:
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
            values[QStringLiteral("DeviceIndex")] = m_deviceIndex;
            values[QStringLiteral("Bitrate")] = m_bitrate;
            values[QStringLiteral("Channel")] = widget->channelSpin->value();
            values[QStringLiteral("Mode")] = widget->modeCombo->currentIndex();
            values[QStringLiteral("CanId")] = m_canId;
            values[QStringLiteral("Data")] = m_data;
            values[QStringLiteral("Extended")] = widget->extendedCheck->isChecked();
            values[QStringLiteral("Rtr")] = widget->rtrCheck->isChecked();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p[QStringLiteral("values")];
            if (!v.isUndefined() && v.isObject()) {
                widget->refreshDevices();
                setDeviceIndex(v[QStringLiteral("DeviceIndex")].toInt());
                setBitrate(v[QStringLiteral("Bitrate")].toInt());
                widget->channelSpin->setValue(v[QStringLiteral("Channel")].toInt());
                widget->modeCombo->setCurrentIndex(v[QStringLiteral("Mode")].toInt());
                setCanId(v[QStringLiteral("CanId")].toString());
                setData(v[QStringLiteral("Data")].toString());
                widget->extendedCheck->setChecked(v[QStringLiteral("Extended")].toBool());
                widget->rtrCheck->setChecked(v[QStringLiteral("Rtr")].toBool());
                openCurrentDevice();
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

            bool ok = false;
            QString idText = m_canId.trimmed();
            if (idText.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
                idText = idText.mid(2);
            }
            const quint32 canId = idText.toUInt(&ok, 16);
            if (!ok) {
                AbstractDelegateModel::stateFeedBack("/send", false);
                return;
            }

            QByteArray payload = QByteArray::fromHex(QString(m_data).remove(QLatin1Char(' ')).toLatin1());
            if (payload.size() > 8) {
                payload = payload.left(8);
            }

            emit sendCanFrame(canId, payload, widget->extendedCheck->isChecked(), widget->rtrCheck->isChecked());
            AbstractDelegateModel::stateFeedBack("/send", false);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("deviceIndex")) {
                setDeviceIndex(ev.payload.toInt());
            } else if (localPath == QLatin1String("bitrate")) {
                setBitrate(ev.payload.toInt());
            } else if (localPath == QLatin1String("canId")) {
                setCanId(ev.payload.toString());
            } else if (localPath == QLatin1String("data")) {
                setData(ev.payload.toString());
            } else if (localPath == QLatin1String("connected")) {
                setConnected(ev.payload.toBool());
            } else if (localPath == QLatin1String("send")) {
                sendMessage();
            }
        }

    signals:
        void deviceIndexChanged(int deviceIndex);
        void bitrateChanged(int bitrate);
        void canIdChanged(QString canId);
        void dataChanged(QString data);
        void connectedChanged(bool connected);

        void openCanBus(int deviceIndex, int channel, int bitrate, int modeFlags);
        void closeCanBus();
        void sendCanFrame(quint32 canId, const QByteArray &data, bool extended, bool rtr);

    private:
        int modeFlagsFromUi() const
        {
            switch (widget->modeCombo->currentData().toInt()) {
            case 1:
                return 0x01; // CANDLE_MODE_LISTEN_ONLY
            case 2:
                return 0x02; // CANDLE_MODE_LOOP_BACK
            default:
                return 0x00; // CANDLE_MODE_NORMAL
            }
        }

        void openCurrentDevice()
        {
            if (m_deviceIndex < 0) {
                emit closeCanBus();
                return;
            }
            emit openCanBus(m_deviceIndex,
                            widget->channelSpin->value(),
                            m_bitrate,
                            modeFlagsFromUi());
        }

        CanBusInterface *widget = new CanBusInterface();
        CanBus *client = new CanBus();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;

        int m_deviceIndex = -1;
        int m_bitrate = 500000;
        QString m_canId = QStringLiteral("0x123");
        QString m_data;
        bool m_connected = false;
    };
}
