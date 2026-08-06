#include "FTC040ADataModel.hpp"

#include <QJsonObject>
#include <QSignalBlocker>

namespace Nodes {

namespace {
constexpr quint8 kFcReadCoils = 0x01;
constexpr quint8 kFcWriteSingleCoil = 0x05;
constexpr quint8 kFcWriteMultipleCoils = 0x0F;
constexpr quint8 kExIllegalFunction = 0x01;
constexpr quint8 kExIllegalDataAddress = 0x02;
constexpr quint8 kExIllegalDataValue = 0x03;
}

FTC040ADataModel::FTC040ADataModel()
    : _interface(new FTC040AInterface())
{
    // 注意：_host/_port 按声明顺序在 _tcpServer 之后初始化，
    // 不能在成员初始化列表里用它们构造 TcpServer，否则会读到未初始化内存并崩溃。
    InPortCount = 3;
    OutPortCount = kCoilCount;
    PortEditable = false;
    CaptionVisible = true;
    Caption = "FT-C040A";
    WidgetEmbeddable = false;
    Resizable = false;

    for (int i = 0; i < kCoilCount; ++i) {
        _coils[i] = false;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>(false);
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
        b.member = "listening";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/listening", this, b);
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

    for (int i = 0; i < kCoilCount; ++i) {
        const int addr = i + static_cast<int>(kCoilBaseAddress);
        const QString label = coilAddressLabel(addr);
        {
            NodeDelegateModel::ExternalBinding b;
            b.control = _interface->_coilCheckBoxes[i];
            AbstractDelegateModel::registerExternalBinding("/" + label, nullptr, b);
        }
        connect(_interface->_coilCheckBoxes[i], &QCheckBox::clicked, this, [this, i](bool checked) {
            setCoil(i, checked);
        });
    }

    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_serverId->setValue(_serverId);

    _tcpServer = new TcpServer(_host, _port);
    connect(_tcpServer, &TcpServer::isReady, this, &FTC040ADataModel::onServerReady);
    connect(_tcpServer, &TcpServer::recMsg, this, &FTC040ADataModel::onRecMsg);
}

FTC040ADataModel::~FTC040ADataModel()
{
    if (_tcpServer) {
        _tcpServer->cleanup();
        delete _tcpServer;
        _tcpServer = nullptr;
    }
}

void FTC040ADataModel::setHost(const QString &host)
{
    if (_host == host) {
        return;
    }
    _host = host;
    QSignalBlocker blocker(_interface->_hostEdit);
    _interface->_hostEdit->setText(_host);
    emit hostChanged(_host);
    restartServer();
}

void FTC040ADataModel::setPort(int port)
{
    if (_port == port) {
        return;
    }
    _port = port;
    QSignalBlocker blocker(_interface->_portEdit);
    _interface->_portEdit->setValue(_port);
    emit portChanged(_port);
    restartServer();
}

void FTC040ADataModel::setServerId(int serverId)
{
    if (_serverId == serverId) {
        return;
    }
    _serverId = serverId;
    QSignalBlocker blocker(_interface->_serverId);
    _interface->_serverId->setValue(_serverId);
    emit serverIdChanged(_serverId);
}

void FTC040ADataModel::setListening(bool listening)
{
    if (_listening == listening) {
        return;
    }
    _listening = listening;
    if (_interface) {
        _interface->setListeningStatus(_listening);
    }
    emit listeningChanged(_listening);
}

void FTC040ADataModel::restartServer()
{
    _rxBuffers.clear();
    if (_tcpServer) {
        QMetaObject::invokeMethod(_tcpServer, "setHost", Qt::QueuedConnection,
                                  Q_ARG(QString, _host), Q_ARG(int, _port));
    }
}

void FTC040ADataModel::onServerReady(bool ready)
{
    setListening(ready);
}

void FTC040ADataModel::onGlobalEvent(const GlobalEvent &ev)
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
    } else {
        bool ok = false;
        const int addr = localPath.toInt(&ok); // "0001" → 1
        if (ok && isCoilAddressValid(static_cast<quint16>(addr))) {
            setCoil(addressToIndex(static_cast<quint16>(addr)), ev.payload.toBool());
        }
    }
}

void FTC040ADataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/serverId"), this, SLOT(onGlobalEvent(GlobalEvent)));
    for (int i = 0; i < kCoilCount; ++i) {
        const int addr = i + static_cast<int>(kCoilBaseAddress);
        bus->subscribe(makeFullOscAddress("/" + coilAddressLabel(addr)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }
}

NodeDataType FTC040ADataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> FTC040ADataModel::outData(PortIndex port)
{
    if (port >= 0 && port < kCoilCount) {
        return _outputData[port];
    }
    return nullptr;
}

void FTC040ADataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!varData) {
        return;
    }
    switch (port) {
    case 0:
        setHost(varData->value().toString());
        break;
    case 1:
        setPort(varData->value().toInt());
        break;
    case 2:
        setServerId(varData->value().toInt());
        break;
    default:
        break;
    }
}

QString FTC040ADataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        switch (portIndex) {
        case 0:
            return "HOST";
        case 1:
            return "PORT";
        case 2:
            return "ID";
        default:
            break;
        }
        break;
    case PortType::Out:
        if (portIndex >= 0 && portIndex < kCoilCount) {
            return coilAddressLabel(portIndex + static_cast<int>(kCoilBaseAddress));
        }
        break;
    default:
        break;
    }
    return {};
}

QJsonObject FTC040ADataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values["host"] = _host;
    values["port"] = _port;
    values["serverId"] = _serverId;
    modelJson["values"] = values;
    return modelJson;
}

void FTC040ADataModel::load(QJsonObject const &p)
{
    const QJsonValue v = p["values"];
    if (!v.isObject()) {
        return;
    }
    const QJsonObject values = v.toObject();
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

ConnectionPolicy FTC040ADataModel::portConnectionPolicy(PortType portType, PortIndex index) const
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

void FTC040ADataModel::setCoil(int index, bool state, bool fromModbus)
{
    Q_UNUSED(fromModbus);
    if (index < 0 || index >= kCoilCount) {
        return;
    }
    if (_coils[index] == state) {
        return;
    }

    _coils[index] = state;
    if (_interface) {
        _interface->setCoilState(index, state);
    }
    updateOutputPort(index, state);

    const int addr = index + static_cast<int>(kCoilBaseAddress);
    AbstractDelegateModel::stateFeedBack("/" + coilAddressLabel(addr), state);
}

void FTC040ADataModel::updateOutputPort(int index, bool value)
{
    if (index < 0 || index >= kCoilCount) {
        return;
    }
    _outputData[index] = std::make_shared<NodeDataTypes::VariableData>(value);
    Q_EMIT dataUpdated(index);
}

bool FTC040ADataModel::isCoilAddressValid(quint16 address)
{
    return address >= kCoilBaseAddress
        && address < kCoilBaseAddress + static_cast<quint16>(kCoilCount);
}

int FTC040ADataModel::addressToIndex(quint16 address)
{
    return static_cast<int>(address - kCoilBaseAddress);
}

QString FTC040ADataModel::coilAddressLabel(int address)
{
    return QString("%1").arg(address, 4, 10, QChar('0'));
}

void FTC040ADataModel::onRecMsg(const QVariantMap &msg)
{
    const QString peerHost = msg.value("host").toString();
    const QByteArray chunk = msg.value("default").toByteArray();
    if (peerHost.isEmpty() || chunk.isEmpty()) {
        return;
    }
    processClientData(peerHost, chunk);
}

void FTC040ADataModel::processClientData(const QString &peerHost, const QByteArray &chunk)
{
    QByteArray &buffer = _rxBuffers[peerHost];
    buffer.append(chunk);

    int offset = 0;
    while (offset + 7 <= buffer.size()) {
        const int length = (static_cast<quint8>(buffer[offset + 4]) << 8)
                         | static_cast<quint8>(buffer[offset + 5]);
        const int frameSize = 6 + length;
        if (length < 2 || frameSize < 8 || offset + frameSize > buffer.size()) {
            break;
        }

        handleRequestFrame(peerHost, buffer.mid(offset, frameSize));
        offset += frameSize;
    }

    if (offset > 0) {
        buffer.remove(0, offset);
    }
    // 防止异常数据无限增长
    if (buffer.size() > 4096) {
        buffer.clear();
    }
}

void FTC040ADataModel::handleRequestFrame(const QString &peerHost, const QByteArray &frame)
{
    Q_UNUSED(peerHost);
    if (frame.size() < 8) {
        return;
    }

    const quint16 transactionId = (static_cast<quint8>(frame[0]) << 8) | static_cast<quint8>(frame[1]);
    const quint16 protocolId = (static_cast<quint8>(frame[2]) << 8) | static_cast<quint8>(frame[3]);
    const quint8 unitId = static_cast<quint8>(frame[6]);
    const quint8 functionCode = static_cast<quint8>(frame[7]);

    if (protocolId != 0) {
        return;
    }
    if (unitId != static_cast<quint8>(_serverId)) {
        return;
    }

    QByteArray response;
    switch (functionCode) {
    case kFcReadCoils: {
        if (frame.size() < 12) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        const quint16 startAddress = (static_cast<quint8>(frame[8]) << 8) | static_cast<quint8>(frame[9]);
        const quint16 quantity = (static_cast<quint8>(frame[10]) << 8) | static_cast<quint8>(frame[11]);
        if (quantity < 1 || quantity > kCoilCount) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        if (!isCoilAddressValid(startAddress)
            || !isCoilAddressValid(static_cast<quint16>(startAddress + quantity - 1))) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataAddress);
            break;
        }
        response = buildReadCoilsResponse(transactionId, unitId, startAddress, quantity);
        break;
    }
    case kFcWriteSingleCoil: {
        if (frame.size() < 12) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        const quint16 address = (static_cast<quint8>(frame[8]) << 8) | static_cast<quint8>(frame[9]);
        const quint16 value = (static_cast<quint8>(frame[10]) << 8) | static_cast<quint8>(frame[11]);
        if (!isCoilAddressValid(address)) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataAddress);
            break;
        }
        if (value != 0xFF00 && value != 0x0000) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        setCoil(addressToIndex(address), value == 0xFF00, true);
        response = buildWriteSingleCoilResponse(transactionId, unitId, address, value);
        break;
    }
    case kFcWriteMultipleCoils: {
        if (frame.size() < 13) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        const quint16 startAddress = (static_cast<quint8>(frame[8]) << 8) | static_cast<quint8>(frame[9]);
        const quint16 quantity = (static_cast<quint8>(frame[10]) << 8) | static_cast<quint8>(frame[11]);
        const quint8 byteCount = static_cast<quint8>(frame[12]);
        const int expectedBytes = (quantity + 7) / 8;
        if (quantity < 1 || quantity > kCoilCount || byteCount != expectedBytes || frame.size() < 13 + byteCount) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        if (!isCoilAddressValid(startAddress)
            || !isCoilAddressValid(static_cast<quint16>(startAddress + quantity - 1))) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataAddress);
            break;
        }
        for (int i = 0; i < quantity; ++i) {
            const int byteIndex = i / 8;
            const int bitIndex = i % 8;
            const bool on = (static_cast<quint8>(frame[13 + byteIndex]) & (1 << bitIndex)) != 0;
            setCoil(addressToIndex(static_cast<quint16>(startAddress + i)), on, true);
        }
        response = buildWriteMultipleCoilsResponse(transactionId, unitId, startAddress, quantity);
        break;
    }
    default:
        response = buildException(transactionId, unitId, functionCode, kExIllegalFunction);
        break;
    }

    if (!response.isEmpty() && _tcpServer) {
        // 单主站场景广播即可，避免 IPv4/IPv6 映射地址字符串不一致导致回包失败
        _tcpServer->sendByteArray(response);
    }
}

QByteArray FTC040ADataModel::buildException(quint16 transactionId, quint8 unitId, quint8 functionCode, quint8 exceptionCode) const
{
    QByteArray frame;
    frame.reserve(9);
    frame.append(static_cast<char>(transactionId >> 8));
    frame.append(static_cast<char>(transactionId & 0xFF));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x03));
    frame.append(static_cast<char>(unitId));
    frame.append(static_cast<char>(functionCode | 0x80));
    frame.append(static_cast<char>(exceptionCode));
    return frame;
}

QByteArray FTC040ADataModel::buildReadCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const
{
    const quint8 byteCount = static_cast<quint8>((quantity + 7) / 8);
    QByteArray pdu;
    pdu.append(static_cast<char>(kFcReadCoils));
    pdu.append(static_cast<char>(byteCount));

    for (int b = 0; b < byteCount; ++b) {
        quint8 value = 0;
        for (int bit = 0; bit < 8; ++bit) {
            const int offset = b * 8 + bit;
            if (offset >= quantity) {
                break;
            }
            const int index = addressToIndex(static_cast<quint16>(startAddress + offset));
            if (_coils[index]) {
                value |= static_cast<quint8>(1 << bit);
            }
        }
        pdu.append(static_cast<char>(value));
    }

    QByteArray frame;
    const quint16 length = static_cast<quint16>(1 + pdu.size()); // unitId + PDU
    frame.append(static_cast<char>(transactionId >> 8));
    frame.append(static_cast<char>(transactionId & 0xFF));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(static_cast<char>(length >> 8));
    frame.append(static_cast<char>(length & 0xFF));
    frame.append(static_cast<char>(unitId));
    frame.append(pdu);
    return frame;
}

QByteArray FTC040ADataModel::buildWriteSingleCoilResponse(quint16 transactionId, quint8 unitId, quint16 address, quint16 value) const
{
    QByteArray frame;
    frame.reserve(12);
    frame.append(static_cast<char>(transactionId >> 8));
    frame.append(static_cast<char>(transactionId & 0xFF));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x06));
    frame.append(static_cast<char>(unitId));
    frame.append(static_cast<char>(kFcWriteSingleCoil));
    frame.append(static_cast<char>(address >> 8));
    frame.append(static_cast<char>(address & 0xFF));
    frame.append(static_cast<char>(value >> 8));
    frame.append(static_cast<char>(value & 0xFF));
    return frame;
}

QByteArray FTC040ADataModel::buildWriteMultipleCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const
{
    QByteArray frame;
    frame.reserve(12);
    frame.append(static_cast<char>(transactionId >> 8));
    frame.append(static_cast<char>(transactionId & 0xFF));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x06));
    frame.append(static_cast<char>(unitId));
    frame.append(static_cast<char>(kFcWriteMultipleCoils));
    frame.append(static_cast<char>(startAddress >> 8));
    frame.append(static_cast<char>(startAddress & 0xFF));
    frame.append(static_cast<char>(quantity >> 8));
    frame.append(static_cast<char>(quantity & 0xFF));
    return frame;
}

} // namespace Nodes
