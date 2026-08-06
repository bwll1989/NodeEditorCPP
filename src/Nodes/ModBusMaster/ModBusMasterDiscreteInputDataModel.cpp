#include "ModBusMasterDiscreteInputDataModel.hpp"

#include <QComboBox>
#include <QJsonObject>
#include <QSignalBlocker>

namespace Nodes {

namespace {
constexpr int kPollIntervalMs = 100;
constexpr int kResponseTimeoutMs = 800;
}

ModBusMasterDiscreteInputDataModel::ModBusMasterDiscreteInputDataModel()
    : _interface(new ModBusMasterDiscreteInputInterface())
    , _tcpClient(new TcpClient("127.0.0.1", 502))
    , _readTimer(new QTimer(this))
    , _responseTimer(new QTimer(this))
{
    InPortCount = 0;
    OutPortCount = static_cast<unsigned int>(kDefaultInputCount);
    PortEditable = false;
    CaptionVisible = true;
    Caption = "ModBus Discrete Input";
    WidgetEmbeddable = false;
    Resizable = true;

    resizeInputState(kDefaultInputCount);

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
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "inputCount";
        b.control = _interface->_inputCountEdit;
        AbstractDelegateModel::registerExternalBinding("/inputCount", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "addressBase";
        b.control = _interface->_addressBaseCombo;
        AbstractDelegateModel::registerExternalBinding("/addressBase", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "inputStartAddress";
        b.control = _interface->_inputStartEdit;
        AbstractDelegateModel::registerExternalBinding("/inputStartAddress", this, b);
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
    connect(_interface->_inputCountEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setInputCount(val);
    });
    connect(_interface->_addressBaseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        setAddressBase(_interface->_addressBaseCombo->currentData().toInt());
    });
    connect(_interface->_inputStartEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setInputStartAddress(val);
    });

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
        readAllInputs();
    });

    _responseTimer->setSingleShot(true);
    _responseTimer->setInterval(kResponseTimeoutMs);
    connect(_responseTimer, &QTimer::timeout, this, &ModBusMasterDiscreteInputDataModel::onResponseTimeout);

    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_serverId->setValue(_serverId);
    _interface->_inputCountEdit->setValue(_inputCount);
    {
        QSignalBlocker b(_interface->_addressBaseCombo);
        _interface->_addressBaseCombo->setCurrentIndex(_addressBase == 0 ? 0 : 1);
    }
    _interface->_inputStartEdit->setValue(getInputStartAddress());

    rebuildInputUiBindings();
}

ModBusMasterDiscreteInputDataModel::~ModBusMasterDiscreteInputDataModel()
{
    if (_tcpClient) {
        _tcpClient->disconnectFromServer();
        delete _tcpClient;
        _tcpClient = nullptr;
    }
    if (_readTimer) {
        _readTimer->stop();
    }
    if (_responseTimer) {
        _responseTimer->stop();
    }
}

void ModBusMasterDiscreteInputDataModel::setHost(const QString &host)
{
    if (_host == host) {
        return;
    }
    _host = host;

    QSignalBlocker blocker(_interface->_hostEdit);
    _interface->_hostEdit->setText(_host);
    emit hostChanged(_host);

    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void ModBusMasterDiscreteInputDataModel::setPort(int port)
{
    if (_port == port) {
        return;
    }
    _port = port;

    QSignalBlocker blocker(_interface->_portEdit);
    _interface->_portEdit->setValue(_port);
    emit portChanged(_port);

    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void ModBusMasterDiscreteInputDataModel::setServerId(int serverId)
{
    if (_serverId == serverId) {
        return;
    }
    _serverId = serverId;

    QSignalBlocker blocker(_interface->_serverId);
    _interface->_serverId->setValue(_serverId);
    emit serverIdChanged(_serverId);
}

void ModBusMasterDiscreteInputDataModel::setInputCount(int count)
{
    reconfigureInputs(count, _inputStartAddress, true);
}

void ModBusMasterDiscreteInputDataModel::setAddressBase(int base)
{
    base = (base <= 0) ? 0 : 1;
    if (base == _addressBase) {
        return;
    }
    _addressBase = base;
    {
        QSignalBlocker b(_interface->_addressBaseCombo);
        _interface->_addressBaseCombo->setCurrentIndex(_addressBase == 0 ? 0 : 1);
    }
    {
        QSignalBlocker b(_interface->_inputStartEdit);
        _interface->updateStartAddressLabel(_addressBase);
        _interface->_inputStartEdit->setValue(getInputStartAddress());
    }
    emit addressBaseChanged(_addressBase);
    emit inputStartAddressChanged(getInputStartAddress());
    rebuildInputUiBindings();
    Q_EMIT embeddedWidgetSizeUpdated();
}

void ModBusMasterDiscreteInputDataModel::setInputStartAddress(int address)
{
    const int pdu = qMax(0, address - _addressBase);
    const quint16 start = static_cast<quint16>(qMin(pdu, 65535));
    if (start == _inputStartAddress) {
        QSignalBlocker b(_interface->_inputStartEdit);
        _interface->_inputStartEdit->setValue(getInputStartAddress());
        return;
    }
    reconfigureInputs(_inputCount, start, false);
}

int ModBusMasterDiscreteInputDataModel::displayAddress(int index) const
{
    return static_cast<int>(_inputStartAddress) + index + _addressBase;
}

void ModBusMasterDiscreteInputDataModel::reconfigureInputs(int count, quint16 pduStart, bool notifyPorts)
{
    count = qBound(kMinInputCount, count, kMaxInputCount);
    const bool countChanged = (count != _inputCount);
    const bool startChanged = (pduStart != _inputStartAddress);

    if (!countChanged && !startChanged) {
        return;
    }

    if (countChanged) {
        const unsigned int oldCount = OutPortCount;
        const unsigned int newCount = static_cast<unsigned int>(count);

        if (notifyPorts && newCount != oldCount) {
            if (newCount > oldCount) {
                Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, newCount - 1);
                OutPortCount = newCount;
                Q_EMIT portsInserted();
            } else if (newCount < oldCount) {
                Q_EMIT portsAboutToBeDeleted(PortType::Out, newCount, oldCount - 1);
                OutPortCount = newCount;
                Q_EMIT portsDeleted();
            }
        } else {
            OutPortCount = newCount;
        }
        InPortCount = 0;

        _inputCount = count;
        resizeInputState(count);
        emit inputCountChanged(_inputCount);
    }

    if (startChanged) {
        _inputStartAddress = pduStart;
        emit inputStartAddressChanged(getInputStartAddress());
    }

    {
        QSignalBlocker b1(_interface->_inputCountEdit);
        QSignalBlocker b2(_interface->_inputStartEdit);
        _interface->_inputCountEdit->setValue(_inputCount);
        _interface->_inputStartEdit->setValue(getInputStartAddress());
    }

    rebuildInputUiBindings();
    Q_EMIT embeddedWidgetSizeUpdated();

    clearCommandQueue();
    if (_connected) {
        readAllInputs();
    }
}

void ModBusMasterDiscreteInputDataModel::resizeInputState(int count)
{
    const int oldSize = _inputStates.size();
    _inputStates.resize(count);
    _outputData.resize(count);

    for (int i = oldSize; i < count; ++i) {
        _inputStates[i] = false;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>(false);
    }
}

void ModBusMasterDiscreteInputDataModel::rebuildInputUiBindings()
{
    for (int i = 0; i < kMaxInputCount; ++i) {
        unregisterExternalBinding(QString("/DI%1").arg(i));
    }

    _interface->rebuildInputWidgets(_inputCount, static_cast<int>(_inputStartAddress), _addressBase);

    for (int i = 0; i < _inputCount; ++i) {
        _interface->setInputState(i, _inputStates[i]);

        NodeDelegateModel::ExternalBinding b;
        b.control = _interface->inputCheckBoxes().value(i);
        AbstractDelegateModel::registerExternalBinding(QString("/DI%1").arg(i), nullptr, b);
    }

    _interface->setConnectionStatus(_connected);
}

void ModBusMasterDiscreteInputDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();

    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/serverId"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/inputCount"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/addressBase"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/inputStartAddress"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

void ModBusMasterDiscreteInputDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    } else if (localPath == "serverId") {
        setServerId(ev.payload.toInt());
    } else if (localPath == "inputCount") {
        setInputCount(ev.payload.toInt());
    } else if (localPath == "addressBase") {
        setAddressBase(ev.payload.toInt());
    } else if (localPath == "inputStartAddress") {
        setInputStartAddress(ev.payload.toInt());
    }
}

NodeDataType ModBusMasterDiscreteInputDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> ModBusMasterDiscreteInputDataModel::outData(PortIndex port)
{
    if (port >= 0 && port < _outputData.size()) {
        return _outputData[port];
    }
    return nullptr;
}

void ModBusMasterDiscreteInputDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    Q_UNUSED(data);
    Q_UNUSED(port);
}

QString ModBusMasterDiscreteInputDataModel::portCaption(QtNodes::PortType portType,
                                                       QtNodes::PortIndex portIndex) const
{
    if (portType != PortType::Out) {
        return "";
    }
    const int absAddr = displayAddress(static_cast<int>(portIndex));
    return QString("DI%1 [%2]").arg(portIndex).arg(absAddr, 4, 10, QChar('0'));
}

QJsonObject ModBusMasterDiscreteInputDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;

    values["host"] = _host;
    values["port"] = _port;
    values["serverId"] = _serverId;
    values["inputCount"] = _inputCount;
    values["addressBase"] = _addressBase;
    values["inputStartAddress"] = static_cast<int>(_inputStartAddress);
    // DI 状态只来自远端 FC02 读回，不持久化

    modelJson["values"] = values;
    return modelJson;
}

void ModBusMasterDiscreteInputDataModel::load(QJsonObject const &p)
{
    const QJsonValue v = p["values"];
    if (!v.isObject()) {
        return;
    }

    const QJsonObject values = v.toObject();

    int count = _inputCount;
    quint16 start = _inputStartAddress;
    if (values.contains("inputCount")) {
        count = values["inputCount"].toInt(_inputCount);
    }
    if (values.contains("addressBase")) {
        setAddressBase(values["addressBase"].toInt(_addressBase));
    }
    if (values.contains("inputStartAddress")) {
        start = static_cast<quint16>(values["inputStartAddress"].toInt(_inputStartAddress));
    }

    reconfigureInputs(count, start, false);

    if (values.contains("host")) {
        setHost(values["host"].toString());
    }
    if (values.contains("port")) {
        setPort(values["port"].toInt());
    }
    if (values.contains("serverId")) {
        setServerId(values["serverId"].toInt());
    }
}

ConnectionPolicy ModBusMasterDiscreteInputDataModel::portConnectionPolicy(PortType portType,
                                                                         PortIndex index) const
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

void ModBusMasterDiscreteInputDataModel::recMsg(QByteArray msg, QString ip, int port)
{
    Q_UNUSED(ip);
    Q_UNUSED(port);
    processModbusResponse(msg);
}

quint16 ModBusMasterDiscreteInputDataModel::transactionIdFromCommand(const QByteArray &command) const
{
    if (command.size() < 2) {
        return 0;
    }
    return (static_cast<quint8>(command[0]) << 8) | static_cast<quint8>(command[1]);
}

void ModBusMasterDiscreteInputDataModel::enqueueModbusCommand(const QByteArray &command)
{
    ModbusCommand item;
    item.payload = command;
    item.transactionId = transactionIdFromCommand(command);
    _commandQueue.append(item);
    pumpCommandQueue();
}

void ModBusMasterDiscreteInputDataModel::pumpCommandQueue()
{
    if (_awaitingResponse || !_connected || _commandQueue.isEmpty()) {
        return;
    }

    const ModbusCommand cmd = _commandQueue.takeFirst();
    _activeTransactionId = cmd.transactionId;
    _awaitingResponse = true;

    _tcpClient->sendMessage(cmd.payload.toHex(), 0);
    _responseTimer->start();
}

void ModBusMasterDiscreteInputDataModel::clearCommandQueue()
{
    _commandQueue.clear();
    _awaitingResponse = false;
    _responseTimer->stop();
}

void ModBusMasterDiscreteInputDataModel::readAllInputs()
{
    if (!_connected || _inputCount <= 0) {
        return;
    }
    const QByteArray command = generateReadDiscreteInputsCommand(
        _inputStartAddress, static_cast<quint16>(_inputCount));
    enqueueModbusCommand(command);
}

void ModBusMasterDiscreteInputDataModel::setConnected(bool connected)
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
        readAllInputs();
    } else {
        _readTimer->stop();
        clearCommandQueue();
    }
}

void ModBusMasterDiscreteInputDataModel::finishActiveCommand(bool success)
{
    Q_UNUSED(success);
    _responseTimer->stop();
    _awaitingResponse = false;
    pumpCommandQueue();
}

void ModBusMasterDiscreteInputDataModel::onResponseTimeout()
{
    if (!_awaitingResponse) {
        return;
    }
    _awaitingResponse = false;
    _responseTimer->stop();
    pumpCommandQueue();
}

void ModBusMasterDiscreteInputDataModel::processModbusResponse(const QByteArray &response)
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

void ModBusMasterDiscreteInputDataModel::handleSingleFrame(const QByteArray &frame)
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

    if (functionCode == 0x02) {
        if (frame.size() >= 9) {
            const quint8 byteCount = static_cast<quint8>(frame[8]);
            if (frame.size() >= 9 + byteCount) {
                for (int i = 0; i < _inputCount; ++i) {
                    const int byteIndex = i / 8;
                    const int bitIndex = i % 8;
                    if (byteIndex >= byteCount) {
                        break;
                    }
                    const quint8 inputByte = static_cast<quint8>(frame[9 + byteIndex]);
                    const bool state = (inputByte & (1 << bitIndex)) != 0;
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
        return;
    }

    finishActiveCommand(true);
}

QByteArray ModBusMasterDiscreteInputDataModel::generateReadDiscreteInputsCommand(quint16 startAddress,
                                                                                quint16 quantity)
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

void ModBusMasterDiscreteInputDataModel::updateOutputData(int port, bool value)
{
    if (port < 0 || port >= _outputData.size()) {
        return;
    }
    _outputData[port] = std::make_shared<NodeDataTypes::VariableData>(value);
    Q_EMIT dataUpdated(port);
}

} // namespace Nodes
