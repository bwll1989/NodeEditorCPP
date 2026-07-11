#include "USR-IO424DataModel.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace Nodes {

namespace {
constexpr int kPollIntervalMs = 100;  // 10Hz，厂家要求指令间隔不低于 100ms
constexpr int kResponseTimeoutMs = 800;
}

USR_IO424DataModel::USR_IO424DataModel()
    : _interface(new USR_IO424Interface())
    , _tcpClient(new TcpClient("127.0.0.1", 8080))
    , _readTimer(new QTimer(this))
    , _responseTimer(new QTimer(this))
    , _transactionId(0)
    , _host("127.0.0.1")
    , _port(8080)
    , _serverId(1)
{
    InPortCount = kChannelCount;
    OutPortCount = kChannelCount;
    PortEditable = false;
    CaptionVisible = true;
    Caption = "USR-IO424";
    WidgetEmbeddable = false;
    Resizable = false;

    for (int i = 0; i < kChannelCount; ++i) {
        _inputStates[i] = false;
        _outputStates[i] = false;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>();
    }

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "host";
        b.control = _interface->_hostEdit;
        AbstractDelegateModel::registerExternalBinding("/host", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "port";
        b.control = _interface->_portEdit;
        AbstractDelegateModel::registerExternalBinding("/port", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "serverId";
        b.control = _interface->_serverId;
        AbstractDelegateModel::registerExternalBinding("/serverId", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/connect", this, b);
    }

    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });

    connect(_interface->_portEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setPort(val);
    });

    connect(_interface->_serverId, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setServerId(val);
    });

    for (int i = 0; i < kChannelCount; ++i) {
        {
            NodeDelegateModel::ExternalBinding b;
            b.control = _interface->_outputCheckBoxes[i];
            AbstractDelegateModel::registerExternalBinding("/DO" + QString::number(i), nullptr, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.control = _interface->_inputLabels[i];
            AbstractDelegateModel::registerExternalBinding("/DI" + QString::number(i), nullptr, b);
        }
        connect(_interface->_outputCheckBoxes[i], &QCheckBox::clicked, this, [this, i](bool checked) {
            setOutput(i, checked);
        });
    }

    connect(_interface->_readAll, &QPushButton::clicked, this, [this]() { readAllData(); });

    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("default").toByteArray(), dataMap["host"].toString(), 0);
        }
    });

    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        setConnected(isReady);
    });

    _readTimer->setInterval(kPollIntervalMs);
    connect(_readTimer, &QTimer::timeout, this, [this]() {
        if (!_connected || _awaitingResponse || !_commandQueue.isEmpty()) {
            return;
        }
        readAllData();
    });

    _responseTimer->setSingleShot(true);
    _responseTimer->setInterval(kResponseTimeoutMs);
    connect(_responseTimer, &QTimer::timeout, this, &USR_IO424DataModel::onResponseTimeout);

    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_serverId->setValue(_serverId);
}

USR_IO424DataModel::~USR_IO424DataModel()
{
    if (_tcpClient) {
        _tcpClient->disconnectFromServer();
        delete _tcpClient;
    }
    if (_readTimer) {
        _readTimer->stop();
    }
    if (_responseTimer) {
        _responseTimer->stop();
    }
}

void USR_IO424DataModel::setHost(const QString& host)
{
    if (_host == host) return;
    _host = host;

    QSignalBlocker blocker(_interface->_hostEdit);
    _interface->_hostEdit->setText(_host);

    emit hostChanged(_host);

    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void USR_IO424DataModel::setPort(int port)
{
    if (_port == port) return;
    _port = port;

    QSignalBlocker blocker(_interface->_portEdit);
    _interface->_portEdit->setValue(_port);

    emit portChanged(_port);

    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void USR_IO424DataModel::setServerId(int serverId)
{
    if (_serverId == serverId) return;
    _serverId = serverId;

    QSignalBlocker blocker(_interface->_serverId);
    _interface->_serverId->setValue(_serverId);

    emit serverIdChanged(_serverId);
}

void USR_IO424DataModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    } else if (localPath == "serverId") {
        setServerId(ev.payload.toInt());
    } else if (localPath.startsWith("DO")) {
        bool ok;
        int index = localPath.mid(2).toInt(&ok);
        if (ok && index >= 0 && index < kChannelCount) {
            setOutput(index, ev.payload.toBool());
        }
    }
}

void USR_IO424DataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/serverId"), this, SLOT(onGlobalEvent(GlobalEvent)));

    for (int i = 0; i < kChannelCount; ++i) {
        GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress(QString("/DO%1").arg(i)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }
}

NodeDataType USR_IO424DataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> USR_IO424DataModel::outData(PortIndex port)
{
    if (port >= 0 && port < kChannelCount) {
        return _outputData[port];
    }
    return nullptr;
}

void USR_IO424DataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);

    if (!varData || port < 0 || port >= kChannelCount) {
        return;
    }

    setOutput(port, varData->value().toBool());
}

QString USR_IO424DataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        return QString("DO %1").arg(portIndex);
    case PortType::Out:
        return QString("DI %1").arg(portIndex);
    default:
        break;
    }
    return "";
}

QJsonObject USR_IO424DataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values["host"] = _host;
    values["port"] = _port;
    values["serverId"] = _serverId;

    QJsonArray doStates;
    for (int i = 0; i < kChannelCount; ++i) {
        doStates.append(_outputStates[i]);
    }
    values["doStates"] = doStates;

    modelJson["values"] = values;
    return modelJson;
}

void USR_IO424DataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject()) {
        QJsonObject values = v.toObject();
        if (values.contains("host")) setHost(values["host"].toString());
        if (values.contains("port")) setPort(values["port"].toInt());
        if (values.contains("serverId")) setServerId(values["serverId"].toInt());

        if (values.contains("doStates") && values["doStates"].isArray()) {
            const QJsonArray doStates = values["doStates"].toArray();
            for (int i = 0; i < kChannelCount && i < doStates.size(); ++i) {
                setOutput(i, doStates[i].toBool());
            }
        }
    }
}

ConnectionPolicy USR_IO424DataModel::portConnectionPolicy(PortType portType, PortIndex index) const
{
    Q_UNUSED(index);
    switch (portType) {
    case PortType::In:
    case PortType::Out:
        return ConnectionPolicy::Many;
    case PortType::None:
        break;
    }
    return ConnectionPolicy::One;
}

void USR_IO424DataModel::recMsg(QByteArray msg, QString ip, int port)
{
    Q_UNUSED(ip);
    Q_UNUSED(port);
    processModbusResponse(msg);
}

quint16 USR_IO424DataModel::transactionIdFromCommand(const QByteArray &command) const
{
    if (command.size() < 2) {
        return 0;
    }
    return (static_cast<quint8>(command[0]) << 8) | static_cast<quint8>(command[1]);
}

void USR_IO424DataModel::enqueueModbusCommand(const QByteArray &command, ModbusCommandKind kind)
{
    ModbusCommand item;
    item.payload = command;
    item.transactionId = transactionIdFromCommand(command);
    item.kind = kind;

    _commandQueue.append(item);
    pumpCommandQueue();
}

void USR_IO424DataModel::pumpCommandQueue()
{
    if (_awaitingResponse || !_connected || _commandQueue.isEmpty()) {
        return;
    }

    const ModbusCommand cmd = _commandQueue.takeFirst();
    _activeTransactionId = cmd.transactionId;
    _activeKind = cmd.kind;
    _awaitingResponse = true;

    _tcpClient->sendMessage(cmd.payload.toHex(), 0);
    _responseTimer->start();
}

void USR_IO424DataModel::clearCommandQueue()
{
    _commandQueue.clear();
    _awaitingResponse = false;
    _responseTimer->stop();
}

void USR_IO424DataModel::readAllInputs()
{
    QByteArray command = generateReadDiscreteInputsCommand(kDiAddressBase, kChannelCount);
    enqueueModbusCommand(command, ModbusCommandKind::ReadDiscreteInputs);
}

void USR_IO424DataModel::readAllOutputs()
{
    QByteArray command = generateReadCoilsCommand(kDoAddressBase, kChannelCount);
    enqueueModbusCommand(command, ModbusCommandKind::ReadCoils);
}

void USR_IO424DataModel::readAllData()
{
    if (!_connected) {
        return;
    }

    readAllInputs();
    readAllOutputs();
}

void USR_IO424DataModel::setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }

    _connected = connected;
    emit connectedChanged(_connected);

    if (_interface) {
        _interface->setConnectionStatus(_connected);
    }
    if (_connected) {
        _readTimer->start();
    } else {
        _readTimer->stop();
        clearCommandQueue();
    }
}

void USR_IO424DataModel::setOutput(int index, bool state)
{
    if (index < 0 || index >= kChannelCount) return;
    if (_outputStates[index] == state) return;

    _outputStates[index] = state;

    QSignalBlocker blocker(_interface->_outputCheckBoxes[index]);
    _interface->_outputCheckBoxes[index]->setChecked(state);

    AbstractDelegateModel::stateFeedBack(QString("/DO%1").arg(index), state);
}

void USR_IO424DataModel::writeAllOutputs()
{
    if (!_connected) {
        return;
    }

    QVector<bool> values;
    values.reserve(kChannelCount);
    for (int i = 0; i < kChannelCount; ++i) {
        values.append(_outputStates[i]);
    }

    QByteArray command = generateWriteMultipleCoilsCommand(kDoAddressBase, values, kChannelCount);
    enqueueModbusCommand(command, ModbusCommandKind::WriteMultipleCoils);
}

void USR_IO424DataModel::finishActiveCommand(bool success)
{
    Q_UNUSED(success);

    _responseTimer->stop();
    _awaitingResponse = false;
    pumpCommandQueue();
}

void USR_IO424DataModel::onResponseTimeout()
{
    if (!_awaitingResponse) {
        return;
    }

    _awaitingResponse = false;
    _responseTimer->stop();
    pumpCommandQueue();
}

void USR_IO424DataModel::processModbusResponse(const QByteArray &response)
{
    int offset = 0;
    while (offset + 7 <= response.size()) {
        const int length = (static_cast<quint8>(response[offset + 4]) << 8)
                         | static_cast<quint8>(response[offset + 5]);
        const int frameSize = 6 + length;
        if (frameSize < 8 || offset + frameSize > response.size()) {
            break;
        }

        handleSingleFrame(response.mid(offset, frameSize));
        offset += frameSize;
    }
}

void USR_IO424DataModel::handleSingleFrame(const QByteArray &frame)
{
    if (frame.size() < 8) {
        return;
    }

    const quint16 transactionId = (static_cast<quint8>(frame[0]) << 8) | static_cast<quint8>(frame[1]);
    const quint16 protocolId = (static_cast<quint8>(frame[2]) << 8) | static_cast<quint8>(frame[3]);
    const quint8 functionCode = static_cast<quint8>(frame[7]);

    if (protocolId != 0) {
        return;
    }

    if (!_awaitingResponse || transactionId != _activeTransactionId) {
        return;
    }

    if (functionCode & 0x80) {
        finishActiveCommand(false);
        return;
    }

    switch (functionCode) {
    case 0x01:
        if (frame.size() >= 10) {
            const quint8 byteCount = static_cast<quint8>(frame[8]);
            if (frame.size() >= 9 + byteCount) {
                const quint8 coilData = static_cast<quint8>(frame[9]);
                bool mismatch = false;
                for (int i = 0; i < kChannelCount; ++i) {
                    const bool deviceState = (coilData & (1 << i)) != 0;
                    if (_outputStates[i] != deviceState) {
                        mismatch = true;
                        break;
                    }
                }
                if (mismatch) {
                    writeAllOutputs();
                }
            }
        }
        finishActiveCommand(true);
        break;

    case 0x02:
        if (frame.size() >= 10) {
            const quint8 byteCount = static_cast<quint8>(frame[8]);
            if (frame.size() >= 9 + byteCount) {
                const quint8 inputData = static_cast<quint8>(frame[9]);
                for (int i = 0; i < kChannelCount; ++i) {
                    const bool state = (inputData & (1 << i)) != 0;
                    if (_inputStates[i] != state) {
                        _inputStates[i] = state;
                        _interface->setInputState(i, state);
                        updateOutputData(i, state);
                        AbstractDelegateModel::stateFeedBack(QString("/DI%1").arg(i), state);
                    }
                }
            }
        }
        finishActiveCommand(true);
        break;

    case 0x0F:
        finishActiveCommand(true);
        break;

    default:
        finishActiveCommand(true);
        break;
    }
}

QByteArray USR_IO424DataModel::generateReadCoilsCommand(quint16 startAddress, quint16 quantity)
{
    QByteArray command;
    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x06));
    command.append(static_cast<char>(_serverId));
    command.append(static_cast<char>(0x01));
    command.append(static_cast<char>(startAddress >> 8));
    command.append(static_cast<char>(startAddress & 0xFF));
    command.append(static_cast<char>(quantity >> 8));
    command.append(static_cast<char>(quantity & 0xFF));
    _transactionId++;
    return command;
}

QByteArray USR_IO424DataModel::generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity)
{
    QByteArray command;
    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x06));
    command.append(static_cast<char>(_serverId));
    command.append(static_cast<char>(0x02));
    command.append(static_cast<char>(startAddress >> 8));
    command.append(static_cast<char>(startAddress & 0xFF));
    command.append(static_cast<char>(quantity >> 8));
    command.append(static_cast<char>(quantity & 0xFF));
    _transactionId++;
    return command;
}

QByteArray USR_IO424DataModel::generateWriteMultipleCoilsCommand(quint16 startAddress, const QVector<bool> &values, quint16 quantity)
{
    QByteArray command;
    const quint8 byteCount = (quantity + 7) / 8;

    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(7 + byteCount));
    command.append(static_cast<char>(_serverId));
    command.append(static_cast<char>(0x0F));
    command.append(static_cast<char>(startAddress >> 8));
    command.append(static_cast<char>(startAddress & 0xFF));
    command.append(static_cast<char>(quantity >> 8));
    command.append(static_cast<char>(quantity & 0xFF));
    command.append(static_cast<char>(byteCount));

    for (int i = 0; i < byteCount; ++i) {
        quint8 byteValue = 0;
        for (int j = 0; j < 8 && (i * 8 + j) < values.size(); ++j) {
            if (values[i * 8 + j]) {
                byteValue |= (1 << j);
            }
        }
        command.append(static_cast<char>(byteValue));
    }
    _transactionId++;
    return command;
}

void USR_IO424DataModel::updateOutputData(int port, bool value)
{
    if (port >= 0 && port < kChannelCount) {
        _outputData[port] = std::make_shared<NodeDataTypes::VariableData>(value);
        Q_EMIT dataUpdated(port);
    }
}

} // namespace Nodes
