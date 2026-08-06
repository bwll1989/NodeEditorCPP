#include "ModBusSlaveCoilDataModel.hpp"

#include <QComboBox>
#include <QJsonObject>
#include <QSignalBlocker>

namespace Nodes {

namespace {
// ---------------------------------------------------------------------------
// Modbus 功能码 / 异常码（本从机仅响应线圈相关请求）
// ---------------------------------------------------------------------------
constexpr quint8 kFcReadCoils = 0x01;            // 读线圈
constexpr quint8 kFcWriteSingleCoil = 0x05;      // 写单线圈
constexpr quint8 kFcWriteMultipleCoils = 0x0F;   // 写多线圈
constexpr quint8 kExIllegalFunction = 0x01;      // 非法功能
constexpr quint8 kExIllegalDataAddress = 0x02;   // 非法数据地址
constexpr quint8 kExIllegalDataValue = 0x03;     // 非法数据值
}

// =============================================================================
// 构造 / 析构
// =============================================================================

ModBusSlaveCoilDataModel::ModBusSlaveCoilDataModel()
    : _interface(new ModBusSlaveCoilInterface())
{
    // 注意：_host/_port 按声明顺序在 _tcpServer 之后初始化，
    // 不能在成员初始化列表里用它们构造 TcpServer，否则会读到未初始化内存并崩溃。
    InPortCount = 3; // HOST / PORT / ID
    OutPortCount = static_cast<unsigned int>(kDefaultCoilCount);
    PortEditable = false;
    CaptionVisible = true;
    Caption = "ModBus Slave Coil";
    WidgetEmbeddable = false;
    Resizable = true;

    resizeCoilState(kDefaultCoilCount);

    // 外部控制（OSC / 属性面板）与界面控件绑定
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
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "coilCount";
        b.control = _interface->_coilCountEdit;
        AbstractDelegateModel::registerExternalBinding("/coilCount", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "addressBase";
        b.control = _interface->_addressBaseCombo;
        AbstractDelegateModel::registerExternalBinding("/addressBase", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "coilStartAddress";
        b.control = _interface->_coilStartEdit;
        AbstractDelegateModel::registerExternalBinding("/coilStartAddress", this, b);
    }

    // 界面 → 模型
    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });
    connect(_interface->_portEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setPort(val);
    });
    connect(_interface->_serverId, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setServerId(val);
    });
    connect(_interface->_coilCountEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setCoilCount(val);
    });
    connect(_interface->_addressBaseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        setAddressBase(_interface->_addressBaseCombo->currentData().toInt());
    });
    connect(_interface->_coilStartEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setCoilStartAddress(val);
    });
    // 纯被控：不连接界面勾选 → 写表（勾选框已只读）

    // 初始界面值（起始地址按当前基址显示）
    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_serverId->setValue(_serverId);
    _interface->_coilCountEdit->setValue(_coilCount);
    {
        QSignalBlocker b(_interface->_addressBaseCombo);
        _interface->_addressBaseCombo->setCurrentIndex(_addressBase == 0 ? 0 : 1);
    }
    _interface->_coilStartEdit->setValue(getCoilStartAddress());

    rebuildCoilUiBindings();

    // TcpServer 在成员都就绪后再创建，避免初始化顺序问题
    _tcpServer = new TcpServer(_host, _port);
    connect(_tcpServer, &TcpServer::isReady, this, &ModBusSlaveCoilDataModel::onServerReady);
    connect(_tcpServer, &TcpServer::recMsg, this, &ModBusSlaveCoilDataModel::onRecMsg);
}

ModBusSlaveCoilDataModel::~ModBusSlaveCoilDataModel()
{
    if (_tcpServer) {
        _tcpServer->cleanup();
        delete _tcpServer;
        _tcpServer = nullptr;
    }
}

// =============================================================================
// 连接参数 / 线圈配置
// =============================================================================

void ModBusSlaveCoilDataModel::setHost(const QString &host)
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

void ModBusSlaveCoilDataModel::setPort(int port)
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

void ModBusSlaveCoilDataModel::setServerId(int serverId)
{
    if (_serverId == serverId) {
        return;
    }
    _serverId = serverId;
    QSignalBlocker blocker(_interface->_serverId);
    _interface->_serverId->setValue(_serverId);
    emit serverIdChanged(_serverId);
    // Unit ID 变更不影响监听；后续请求按新 ID 过滤
}

void ModBusSlaveCoilDataModel::setCoilCount(int count)
{
    // 改数量会增删输出端口，需通知图模型
    reconfigureCoils(count, _coilStartAddress, true);
}

/**
 * @brief 切换地址基址（0=PDU，1=显示地址）
 *
 * 只改变「界面/属性显示」；内部 _coilStartAddress 始终存 PDU。
 * 切换时刷新起始地址编辑框显示值与勾选框标签，不改变协议匹配区间。
 */
void ModBusSlaveCoilDataModel::setAddressBase(int base)
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
        QSignalBlocker b(_interface->_coilStartEdit);
        _interface->updateStartAddressLabel(_addressBase);
        // 显示值 = PDU + 基址
        _interface->_coilStartEdit->setValue(getCoilStartAddress());
    }
    emit addressBaseChanged(_addressBase);
    emit coilStartAddressChanged(getCoilStartAddress());
    rebuildCoilUiBindings();
    Q_EMIT embeddedWidgetSizeUpdated();
}

/**
 * @brief 设置起始地址（入参为当前基址下的显示地址）
 *
 * 换算：PDU = max(0, 显示地址 - addressBase)
 */
void ModBusSlaveCoilDataModel::setCoilStartAddress(int address)
{
    const int pdu = qMax(0, address - _addressBase);
    const quint16 start = static_cast<quint16>(qMin(pdu, 65535));
    if (start == _coilStartAddress) {
        QSignalBlocker b(_interface->_coilStartEdit);
        _interface->_coilStartEdit->setValue(getCoilStartAddress());
        return;
    }
    // 只改起始地址不改端口数量，notifyPorts=false
    reconfigureCoils(_coilCount, start, false);
}

/** 端口/勾选框上显示的地址 = PDU起始 + 索引 + 基址 */
int ModBusSlaveCoilDataModel::displayAddress(int index) const
{
    return static_cast<int>(_coilStartAddress) + index + _addressBase;
}

/**
 * @brief 重建线圈数量与/或起始 PDU
 * @param count      线圈路数（钳制到 1～64）
 * @param pduStart   起始 PDU 地址
 * @param notifyPorts 为 true 时通过 portsInserted/Deleted 通知图模型（避免连线错乱）
 */
void ModBusSlaveCoilDataModel::reconfigureCoils(int count, quint16 pduStart, bool notifyPorts)
{
    count = qBound(kMinCoilCount, count, kMaxCoilCount);
    const bool countChanged = (count != _coilCount);
    const bool startChanged = (pduStart != _coilStartAddress);

    if (!countChanged && !startChanged) {
        return;
    }

    // 端口数变化时无需退订线圈 OSC：纯被控不接受 /Cn Command

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
            // load 等场景：直接改计数，避免触发连线清理副作用
            OutPortCount = newCount;
        }
        InPortCount = 3;

        _coilCount = count;
        resizeCoilState(count);
        emit coilCountChanged(_coilCount);
    }

    if (startChanged) {
        _coilStartAddress = pduStart;
        // 对外属性仍报告「显示地址」
        emit coilStartAddressChanged(getCoilStartAddress());
    }

    {
        QSignalBlocker b1(_interface->_coilCountEdit);
        QSignalBlocker b2(_interface->_coilStartEdit);
        _interface->_coilCountEdit->setValue(_coilCount);
        _interface->_coilStartEdit->setValue(getCoilStartAddress());
    }

    rebuildCoilUiBindings();
    Q_EMIT embeddedWidgetSizeUpdated();
}

/** 扩容时新线圈默认 OFF；缩容时多余状态随 resize 丢弃 */
void ModBusSlaveCoilDataModel::resizeCoilState(int count)
{
    const int oldSize = _coils.size();
    _coils.resize(count);
    _outputData.resize(count);

    for (int i = oldSize; i < count; ++i) {
        _coils[i] = false;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>(false);
    }
}

/**
 * @brief 重建只读勾选框，并按索引注册 /C0…/CN-1 外部绑定（仅 UI 同步，不可 Command 写）
 */
void ModBusSlaveCoilDataModel::rebuildCoilUiBindings()
{
    for (int i = 0; i < kMaxCoilCount; ++i) {
        unregisterExternalBinding(QString("/C%1").arg(i));
    }

    _interface->rebuildCoilWidgets(_coilCount, static_cast<int>(_coilStartAddress), _addressBase);

    for (int i = 0; i < _coilCount; ++i) {
        _interface->setCoilState(i, _coils[i]);
        NodeDelegateModel::ExternalBinding b;
        b.control = _interface->coilCheckBoxes().value(i);
        AbstractDelegateModel::registerExternalBinding(QString("/C%1").arg(i), nullptr, b);
    }
}

void ModBusSlaveCoilDataModel::setListening(bool listening)
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

/** 改监听地址/端口后异步重启 TcpServer，并清空分片缓冲 */
void ModBusSlaveCoilDataModel::restartServer()
{
    _rxBuffers.clear();
    if (_tcpServer) {
        QMetaObject::invokeMethod(_tcpServer, "setHost", Qt::QueuedConnection,
                                  Q_ARG(QString, _host), Q_ARG(int, _port));
    }
}

void ModBusSlaveCoilDataModel::onServerReady(bool ready)
{
    setListening(ready);
}

// =============================================================================
// OSC / 端口 / 持久化
// =============================================================================

/** 处理外部 Command：仅配置项；/Cn 写表已禁用（纯被控） */
void ModBusSlaveCoilDataModel::onGlobalEvent(const GlobalEvent &ev)
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
    } else if (localPath == "coilCount") {
        setCoilCount(ev.payload.toInt());
    } else if (localPath == "addressBase") {
        setAddressBase(ev.payload.toInt());
    } else if (localPath == "coilStartAddress") {
        setCoilStartAddress(ev.payload.toInt());
    }
    // /Cn Command 故意忽略：状态只由主站 FC05/0F 写入
}

/** 模型就绪后订阅配置类 OSC；推送线圈初始状态反馈（只读镜像） */
void ModBusSlaveCoilDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();

    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/serverId"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/coilCount"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/addressBase"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/coilStartAddress"), this, SLOT(onGlobalEvent(GlobalEvent)));

    for (int i = 0; i < _coilCount; ++i) {
        AbstractDelegateModel::stateFeedBack(QString("/C%1").arg(i), _coils[i]);
    }
}

NodeDataType ModBusSlaveCoilDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> ModBusSlaveCoilDataModel::outData(PortIndex port)
{
    if (port >= 0 && port < _outputData.size()) {
        return _outputData[port];
    }
    return nullptr;
}

/** 输入端口：0=HOST，1=PORT，2=从站 ID */
void ModBusSlaveCoilDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
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

QString ModBusSlaveCoilDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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
        if (portIndex >= 0 && portIndex < _coilCount) {
            // 例：C2 [0003] —— 索引 + 当前基址下的显示地址
            const int absAddr = displayAddress(static_cast<int>(portIndex));
            return QString("C%1 [%2]").arg(portIndex).arg(absAddr, 4, 10, QChar('0'));
        }
        break;
    default:
        break;
    }
    return {};
}

QJsonObject ModBusSlaveCoilDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values["host"] = _host;
    values["port"] = _port;
    values["serverId"] = _serverId;
    values["coilCount"] = _coilCount;
    values["addressBase"] = _addressBase;
    // 起始地址按 PDU 存盘，与基址解耦，兼容旧工程
    values["coilStartAddress"] = static_cast<int>(_coilStartAddress);

    modelJson["values"] = values;
    return modelJson;
}

void ModBusSlaveCoilDataModel::load(QJsonObject const &p)
{
    const QJsonValue v = p["values"];
    if (!v.isObject()) {
        return;
    }
    const QJsonObject values = v.toObject();

    int count = _coilCount;
    quint16 start = _coilStartAddress;
    if (values.contains("coilCount")) {
        count = values["coilCount"].toInt(_coilCount);
    }
    // 先恢复基址，再 reconfigure，以便界面显示正确
    if (values.contains("addressBase")) {
        setAddressBase(values["addressBase"].toInt(_addressBase));
    }
    if (values.contains("coilStartAddress")) {
        // 文件中为 PDU
        start = static_cast<quint16>(values["coilStartAddress"].toInt(_coilStartAddress));
    }
    reconfigureCoils(count, start, false);

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

ConnectionPolicy ModBusSlaveCoilDataModel::portConnectionPolicy(PortType portType, PortIndex index) const
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

// =============================================================================
// 线圈状态同步（仅主站写入 → 只读 UI / 输出端口 / OSC 反馈）
// =============================================================================

/**
 * @brief 应用主站写线圈结果
 *
 * 更新状态表、只读勾选、输出端口，并 stateFeedBack（外部可监视，不可 Command 回写）。
 */
void ModBusSlaveCoilDataModel::applyMasterCoilWrite(int index, bool state)
{
    if (index < 0 || index >= _coilCount) {
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

    AbstractDelegateModel::stateFeedBack(QString("/C%1").arg(index), state);
}

void ModBusSlaveCoilDataModel::updateOutputPort(int index, bool value)
{
    if (index < 0 || index >= _outputData.size()) {
        return;
    }
    _outputData[index] = std::make_shared<NodeDataTypes::VariableData>(value);
    Q_EMIT dataUpdated(index);
}

/** PDU 地址是否落在 [起始, 起始+数量) */
bool ModBusSlaveCoilDataModel::isCoilAddressValid(quint16 address) const
{
    return address >= _coilStartAddress
        && address < static_cast<quint16>(_coilStartAddress + _coilCount);
}

/** PDU 地址 → 本地索引（调用前应已校验合法） */
int ModBusSlaveCoilDataModel::addressToIndex(quint16 address) const
{
    return static_cast<int>(address - _coilStartAddress);
}

// =============================================================================
// Modbus TCP 收包与分帧
// =============================================================================

void ModBusSlaveCoilDataModel::onRecMsg(const QVariantMap &msg)
{
    const QString peerHost = msg.value("host").toString();
    const QByteArray chunk = msg.value("default").toByteArray();
    if (peerHost.isEmpty() || chunk.isEmpty()) {
        return;
    }
    processClientData(peerHost, chunk);
}

/**
 * @brief 按对端 IP 缓存 TCP 粘包/半包，按 MBAP 长度字段拆出完整 ADU 再处理
 *
 * MBAP：Transaction(2) + Protocol(2) + Length(2) + Unit(1) + PDU…
 * frameSize = 6 + Length（Length 含 Unit + PDU）
 */
void ModBusSlaveCoilDataModel::processClientData(const QString &peerHost, const QByteArray &chunk)
{
    QByteArray &buffer = _rxBuffers[peerHost];
    buffer.append(chunk);

    int offset = 0;
    while (offset + 7 <= buffer.size()) {
        const int length = (static_cast<quint8>(buffer[offset + 4]) << 8)
                         | static_cast<quint8>(buffer[offset + 5]);
        const int frameSize = 6 + length;
        // Length 至少要能容纳 UnitId + FunctionCode
        if (length < 2 || frameSize < 8 || offset + frameSize > buffer.size()) {
            break;
        }

        handleRequestFrame(peerHost, buffer.mid(offset, frameSize));
        offset += frameSize;
    }

    if (offset > 0) {
        buffer.remove(0, offset);
    }
    // 异常堆积防护：过大则清空，避免内存膨胀
    if (buffer.size() > 4096) {
        buffer.clear();
    }
}

/**
 * @brief 处理一帧完整 Modbus TCP 请求并回包
 *
 * 支持：0x01 读线圈 / 0x05 写单线圈 / 0x0F 写多线圈。
 * Unit ID 不匹配则静默忽略（不回包）；其它错误回标准异常。
 */
void ModBusSlaveCoilDataModel::handleRequestFrame(const QString &peerHost, const QByteArray &frame)
{
    Q_UNUSED(peerHost);
    if (frame.size() < 8) {
        return;
    }

    const quint16 transactionId = (static_cast<quint8>(frame[0]) << 8) | static_cast<quint8>(frame[1]);
    const quint16 protocolId = (static_cast<quint8>(frame[2]) << 8) | static_cast<quint8>(frame[3]);
    const quint8 unitId = static_cast<quint8>(frame[6]);
    const quint8 functionCode = static_cast<quint8>(frame[7]);

    // Modbus TCP 协议标识必须为 0
    if (protocolId != 0) {
        return;
    }
    // 非本从站 ID：忽略（一端口多从站场景下由其它节点/设备处理）
    if (unitId != static_cast<quint8>(_serverId)) {
        return;
    }

    QByteArray response;
    switch (functionCode) {
    case kFcReadCoils: {
        // 请求 PDU：起始地址(2) + 数量(2)
        if (frame.size() < 12) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        const quint16 startAddress = (static_cast<quint8>(frame[8]) << 8) | static_cast<quint8>(frame[9]);
        const quint16 quantity = (static_cast<quint8>(frame[10]) << 8) | static_cast<quint8>(frame[11]);
        if (quantity < 1 || quantity > static_cast<quint16>(_coilCount)) {
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
        // 请求 PDU：地址(2) + 值(2)，值仅允许 0xFF00 / 0x0000
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
        applyMasterCoilWrite(addressToIndex(address), value == 0xFF00);
        response = buildWriteSingleCoilResponse(transactionId, unitId, address, value);
        break;
    }
    case kFcWriteMultipleCoils: {
        // 请求 PDU：起始(2) + 数量(2) + 字节数(1) + 线圈位图
        if (frame.size() < 13) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        const quint16 startAddress = (static_cast<quint8>(frame[8]) << 8) | static_cast<quint8>(frame[9]);
        const quint16 quantity = (static_cast<quint8>(frame[10]) << 8) | static_cast<quint8>(frame[11]);
        const quint8 byteCount = static_cast<quint8>(frame[12]);
        const int expectedBytes = (quantity + 7) / 8;
        if (quantity < 1 || quantity > static_cast<quint16>(_coilCount)
            || byteCount != expectedBytes || frame.size() < 13 + byteCount) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataValue);
            break;
        }
        if (!isCoilAddressValid(startAddress)
            || !isCoilAddressValid(static_cast<quint16>(startAddress + quantity - 1))) {
            response = buildException(transactionId, unitId, functionCode, kExIllegalDataAddress);
            break;
        }
        // 位序：字节内 bit0 对应数量方向上的第 1 个线圈
        for (int i = 0; i < quantity; ++i) {
            const int byteIndex = i / 8;
            const int bitIndex = i % 8;
            const bool on = (static_cast<quint8>(frame[13 + byteIndex]) & (1 << bitIndex)) != 0;
            applyMasterCoilWrite(addressToIndex(static_cast<quint16>(startAddress + i)), on);
        }
        response = buildWriteMultipleCoilsResponse(transactionId, unitId, startAddress, quantity);
        break;
    }
    default:
        response = buildException(transactionId, unitId, functionCode, kExIllegalFunction);
        break;
    }

    if (!response.isEmpty() && _tcpServer) {
        _tcpServer->sendByteArray(response);
    }
}

// =============================================================================
// 应答帧构造（MBAP + PDU）
// =============================================================================

/** 异常响应：功能码最高位置 1，后跟异常码 */
QByteArray ModBusSlaveCoilDataModel::buildException(quint16 transactionId, quint8 unitId, quint8 functionCode, quint8 exceptionCode) const
{
    QByteArray frame;
    frame.reserve(9);
    frame.append(static_cast<char>(transactionId >> 8));
    frame.append(static_cast<char>(transactionId & 0xFF));
    frame.append(char(0x00)); // Protocol ID
    frame.append(char(0x00));
    frame.append(char(0x00)); // Length = 3（Unit + FC|0x80 + Ex）
    frame.append(char(0x03));
    frame.append(static_cast<char>(unitId));
    frame.append(static_cast<char>(functionCode | 0x80));
    frame.append(static_cast<char>(exceptionCode));
    return frame;
}

/** 0x01 读线圈应答：字节数 + 位图（低位先对应靠前的线圈） */
QByteArray ModBusSlaveCoilDataModel::buildReadCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const
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
            if (index >= 0 && index < _coils.size() && _coils[index]) {
                value |= static_cast<quint8>(1 << bit);
            }
        }
        pdu.append(static_cast<char>(value));
    }

    QByteArray frame;
    // Length = Unit(1) + PDU
    const quint16 length = static_cast<quint16>(1 + pdu.size());
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

/** 0x05 写单线圈应答：回显地址与值 */
QByteArray ModBusSlaveCoilDataModel::buildWriteSingleCoilResponse(quint16 transactionId, quint8 unitId, quint16 address, quint16 value) const
{
    QByteArray frame;
    frame.reserve(12);
    frame.append(static_cast<char>(transactionId >> 8));
    frame.append(static_cast<char>(transactionId & 0xFF));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x00));
    frame.append(char(0x06)); // Unit + FC + Addr + Value
    frame.append(static_cast<char>(unitId));
    frame.append(static_cast<char>(kFcWriteSingleCoil));
    frame.append(static_cast<char>(address >> 8));
    frame.append(static_cast<char>(address & 0xFF));
    frame.append(static_cast<char>(value >> 8));
    frame.append(static_cast<char>(value & 0xFF));
    return frame;
}

/** 0x0F 写多线圈应答：回显起始地址与数量 */
QByteArray ModBusSlaveCoilDataModel::buildWriteMultipleCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const
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
