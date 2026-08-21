#include "ModBusMasterCoilControllerDataModel.hpp"

#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>

namespace Nodes {

namespace {
// ---------------------------------------------------------------------------
// 时序参数
// ---------------------------------------------------------------------------
constexpr int kPollIntervalMs = 100;      // 空闲时读轮询周期（FC 0x01）
constexpr int kCommandIntervalMs = 5;     // 上一帧应答后，发下一帧前的间隔
constexpr int kResponseTimeoutMs = 800;   // 单帧应答超时
}

// =============================================================================
// 构造 / 析构
// =============================================================================

ModBusMasterCoilControllerDataModel::ModBusMasterCoilControllerDataModel()
    : _interface(new ModBusMasterCoilControllerInterface())
    , _tcpClient(new TcpClient("127.0.0.1", 502))
    , _readTimer(new QTimer(this))
    , _responseTimer(new QTimer(this))
    , _commandGapTimer(new QTimer(this))
{
    // 仅输入端口：写入期望线圈状态；无输出端口
    InPortCount = static_cast<unsigned int>(kDefaultCoilCount);
    OutPortCount = 0;
    PortEditable = false;
    CaptionVisible = true;
    Caption = "ModBus Coil Controller";
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
        b.member = "connected";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/connect", this, b);
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
    connect(_interface, &ModBusMasterCoilControllerInterface::coilChanged, this,
            [this](int index, bool state) { setCoil(index, state); });

    // TcpClient 收包 / 连接状态
    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("default").toByteArray(), dataMap["host"].toString(), 0);
        }
    });
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        setConnected(isReady);
    });

    // 空闲轮询：队列空且无在途应答、无指令间隔等待时，发 0x01 读整段
    _readTimer->setInterval(kPollIntervalMs);
    connect(_readTimer, &QTimer::timeout, this, [this]() {
        if (!_connected || _awaitingResponse || !_commandQueue.isEmpty()
            || _commandGapTimer->isActive()) {
            return;
        }
        readAllCoils();
    });

    // 单帧应答超时
    _responseTimer->setSingleShot(true);
    _responseTimer->setInterval(kResponseTimeoutMs);
    connect(_responseTimer, &QTimer::timeout, this, &ModBusMasterCoilControllerDataModel::onResponseTimeout);

    // 指令间隔：应答结束后再隔约 5ms 发下一帧
    _commandGapTimer->setSingleShot(true);
    _commandGapTimer->setInterval(kCommandIntervalMs);
    connect(_commandGapTimer, &QTimer::timeout, this, &ModBusMasterCoilControllerDataModel::onCommandGapTimeout);

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
}

ModBusMasterCoilControllerDataModel::~ModBusMasterCoilControllerDataModel()
{
    unsubscribeCoilOsc();
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
    if (_commandGapTimer) {
        _commandGapTimer->stop();
    }
}

// =============================================================================
// 连接参数 / 线圈配置
// =============================================================================

void ModBusMasterCoilControllerDataModel::setHost(const QString &host)
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

void ModBusMasterCoilControllerDataModel::setPort(int port)
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

void ModBusMasterCoilControllerDataModel::setServerId(int serverId)
{
    if (_serverId == serverId) {
        return;
    }
    _serverId = serverId;

    QSignalBlocker blocker(_interface->_serverId);
    _interface->_serverId->setValue(_serverId);
    emit serverIdChanged(_serverId);
    // Unit ID 变更后，后续入队命令会带上新 ID
}

void ModBusMasterCoilControllerDataModel::setCoilCount(int count)
{
    // 改数量会增删输入端口，需通知图模型
    reconfigureCoils(count, _coilStartAddress, true);
}

/**
 * @brief 切换地址基址（0=PDU，1=显示地址）
 *
 * 只改变界面/属性显示；内部 _coilStartAddress 始终存 PDU。
 * 协议读写地址不变，仅刷新标签与起始地址编辑框显示值。
 */
void ModBusMasterCoilControllerDataModel::setAddressBase(int base)
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
void ModBusMasterCoilControllerDataModel::setCoilStartAddress(int address)
{
    // 界面值为当前基址下的显示地址，换算为 PDU
    const int pdu = qMax(0, address - _addressBase);
    const quint16 start = static_cast<quint16>(qMin(pdu, 65535));
    if (start == _coilStartAddress) {
        // 仍同步一次界面（例如基址切换后）
        QSignalBlocker b(_interface->_coilStartEdit);
        _interface->_coilStartEdit->setValue(getCoilStartAddress());
        return;
    }
    reconfigureCoils(_coilCount, start, false);
}

/** 端口/勾选框上显示的地址 = PDU起始 + 索引 + 基址 */
int ModBusMasterCoilControllerDataModel::displayAddress(int index) const
{
    return static_cast<int>(_coilStartAddress) + index + _addressBase;
}

/**
 * @brief 重建线圈数量与/或起始 PDU
 * @param count       线圈路数（钳制到 1～64）
 * @param pduStart    起始 PDU 地址
 * @param notifyPorts 为 true 时通过 portsInserted/Deleted 通知图模型
 *
 * 已连接时清空命令队列，并以节点期望为准重新整段写回设备。
 */
void ModBusMasterCoilControllerDataModel::reconfigureCoils(int count, quint16 pduStart, bool notifyPorts)
{
    count = qBound(kMinCoilCount, count, kMaxCoilCount);
    const bool countChanged = (count != _coilCount);
    const bool startChanged = (pduStart != _coilStartAddress);

    if (!countChanged && !startChanged) {
        return;
    }

    unsubscribeCoilOsc();

    if (countChanged) {
        const unsigned int oldCount = InPortCount;
        const unsigned int newCount = static_cast<unsigned int>(count);

        if (notifyPorts && newCount != oldCount) {
            if (newCount > oldCount) {
                Q_EMIT portsAboutToBeInserted(PortType::In, oldCount, newCount - 1);
                InPortCount = newCount;
                Q_EMIT portsInserted();
            } else if (newCount < oldCount) {
                Q_EMIT portsAboutToBeDeleted(PortType::In, newCount, oldCount - 1);
                InPortCount = newCount;
                Q_EMIT portsDeleted();
            }
        } else {
            // load 等场景：直接改计数，避免触发连线清理副作用
            InPortCount = newCount;
        }
        OutPortCount = 0;

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
    subscribeCoilOsc();
    Q_EMIT embeddedWidgetSizeUpdated();

    clearCommandQueue();
    if (_connected) {
        // 配置变更后重新以节点期望同步到设备
        syncDesiredToDevice(false);
    }
}

/** 扩容时新线圈期望/设备缓存默认 OFF；缩容随 resize 丢弃 */
void ModBusMasterCoilControllerDataModel::resizeCoilState(int count)
{
    const int oldSize = _desiredStates.size();
    _desiredStates.resize(count);
    _deviceStates.resize(count);

    for (int i = oldSize; i < count; ++i) {
        _desiredStates[i] = false;
        _deviceStates[i] = false;
    }
}

/**
 * @brief 重建勾选框，并按索引重新注册 /C0…/CN-1 外部绑定
 *
 * rebuildCoilWidgets 入参：数量、PDU 起始、地址基址（用于标签显示）。
 */
void ModBusMasterCoilControllerDataModel::rebuildCoilUiBindings()
{
    // 注销旧勾选框绑定
    for (int i = 0; i < kMaxCoilCount; ++i) {
        unregisterExternalBinding(QString("/C%1").arg(i));
    }

    _interface->rebuildCoilWidgets(_coilCount, static_cast<int>(_coilStartAddress), _addressBase);

    for (int i = 0; i < _coilCount; ++i) {
        _interface->setCoilState(i, _desiredStates[i]);

        NodeDelegateModel::ExternalBinding b;
        b.control = _interface->coilCheckBoxes().value(i);
        // 勾选框绑定仅做 UI 同步，member 为空
        AbstractDelegateModel::registerExternalBinding(QString("/C%1").arg(i), nullptr, b);
    }

    _interface->setConnectionStatus(_connected);
}

void ModBusMasterCoilControllerDataModel::subscribeCoilOsc()
{
    if (!_oscReady) {
        return;
    }
    for (int i = 0; i < _coilCount; ++i) {
        GlobalEventBus::instance()->subscribe(
            AbstractDelegateModel::makeFullOscAddress(QString("/C%1").arg(i)),
            this,
            SLOT(onGlobalEvent(GlobalEvent)));
    }
}

void ModBusMasterCoilControllerDataModel::unsubscribeCoilOsc()
{
    if (!_oscReady) {
        return;
    }
    // 按最大路数退订，避免缩容后残留订阅
    for (int i = 0; i < kMaxCoilCount; ++i) {
        GlobalEventBus::instance()->unsubscribe(
            AbstractDelegateModel::makeFullOscAddress(QString("/C%1").arg(i)),
            this);
    }
}

// =============================================================================
// OSC / 端口 / 持久化
// =============================================================================

/** 模型就绪后再订阅 OSC（此时节点完整地址已可用） */
void ModBusMasterCoilControllerDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    _oscReady = true;

    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/serverId"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/coilCount"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/addressBase"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/coilStartAddress"), this, SLOT(onGlobalEvent(GlobalEvent)));

    subscribeCoilOsc();

    // load 常早于 onModelReady；此处补推 /Cn，外部控制才能拿到初始线圈状态
    for (int i = 0; i < _coilCount; ++i) {
        AbstractDelegateModel::stateFeedBack(QString("/C%1").arg(i), _desiredStates[i]);
    }
}

/** 处理外部 Command：配置项或 /Cn 写期望状态 */
void ModBusMasterCoilControllerDataModel::onGlobalEvent(const GlobalEvent &ev)
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
    } else if (localPath.startsWith("C")) {
        bool ok = false;
        const int index = localPath.mid(1).toInt(&ok);
        if (ok) {
            setCoil(index, ev.payload.toBool());
        }
    }
}

NodeDataType ModBusMasterCoilControllerDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> ModBusMasterCoilControllerDataModel::outData(PortIndex port)
{
    Q_UNUSED(port);
    return nullptr; // 本节点无输出端口
}

/** 输入端口 Cn：写入期望状态，已连接则立即 0x05 写设备 */
void ModBusMasterCoilControllerDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!varData || port < 0 || port >= _coilCount) {
        return;
    }
    setCoil(port, varData->asBool());
}

QString ModBusMasterCoilControllerDataModel::portCaption(QtNodes::PortType portType,
                                                        QtNodes::PortIndex portIndex) const
{
    if (portType != PortType::In) {
        return "";
    }
    // 例：C2 [0003] —— 索引 + 当前基址下的显示地址
    const int absAddr = displayAddress(static_cast<int>(portIndex));
    return QString("C%1 [%2]").arg(portIndex).arg(absAddr, 4, 10, QChar('0'));
}

QJsonObject ModBusMasterCoilControllerDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;

    values["host"] = _host;
    values["port"] = _port;
    values["serverId"] = _serverId;
    values["coilCount"] = _coilCount;
    values["addressBase"] = _addressBase;
    values["coilStartAddress"] = static_cast<int>(_coilStartAddress); // PDU，与基址解耦

    // 持久化节点期望线圈状态，便于工程重新打开后保持
    QJsonArray coilStates;
    for (int i = 0; i < _coilCount; ++i) {
        coilStates.append(_desiredStates[i]);
    }
    values["coilStates"] = coilStates;

    modelJson["values"] = values;
    return modelJson;
}

void ModBusMasterCoilControllerDataModel::load(QJsonObject const &p)
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
        // 存的是 PDU
        start = static_cast<quint16>(values["coilStartAddress"].toInt(_coilStartAddress));
    }

    // 加载时端口数量已由图 JSON 设置，不再二次通知
    reconfigureCoils(count, start, false);

    // 恢复期望线圈状态（在 reconfigure 之后，避免被 resize 覆盖）
    // 走 setCoil：更新 UI、OSC 状态反馈；已连接时入队 0x05
    if (values.contains("coilStates") && values["coilStates"].isArray()) {
        const QJsonArray coilStates = values["coilStates"].toArray();
        for (int i = 0; i < _coilCount && i < coilStates.size(); ++i) {
            setCoil(i, coilStates[i].toBool());
        }
    }

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

ConnectionPolicy ModBusMasterCoilControllerDataModel::portConnectionPolicy(PortType portType,
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

// =============================================================================
// 命令队列（串行：同时只等一帧应答；帧间间隔约 5ms）
// =============================================================================

void ModBusMasterCoilControllerDataModel::recMsg(QByteArray msg, QString ip, int port)
{
    Q_UNUSED(ip);
    Q_UNUSED(port);
    processModbusResponse(msg);
}

quint16 ModBusMasterCoilControllerDataModel::transactionIdFromCommand(const QByteArray &command) const
{
    if (command.size() < 2) {
        return 0;
    }
    return (static_cast<quint8>(command[0]) << 8) | static_cast<quint8>(command[1]);
}

/**
 * @brief 丢弃队列中尚未发出的写命令，保留读等其它命令
 *
 * 重新规划差异写时调用，避免堆积过时目标。
 */
void ModBusMasterCoilControllerDataModel::dropPendingWrites()
{
    QList<ModbusCommand> kept;
    kept.reserve(_commandQueue.size());
    for (const ModbusCommand &cmd : _commandQueue) {
        if (cmd.kind != ModbusCommandKind::WriteSingleCoil) {
            kept.append(cmd);
        }
    }
    _commandQueue.swap(kept);
}

void ModBusMasterCoilControllerDataModel::enqueueModbusCommand(const QByteArray &command,
                                                             ModbusCommandKind kind,
                                                             int coilIndex)
{
    ModbusCommand item;
    item.payload = command;
    item.transactionId = transactionIdFromCommand(command);
    item.kind = kind;
    item.coilIndex = coilIndex; // WriteSingleCoil 时用于写成功后更新设备缓存
    _commandQueue.append(item);
    pumpCommandQueue();
}

/**
 * @brief 若空闲则取出队首命令发送
 *
 * 条件：已连接、无在途应答、队列非空、指令间隔定时器未在跑。
 * 发送后启动应答超时定时器。
 */
void ModBusMasterCoilControllerDataModel::pumpCommandQueue()
{
    if (_awaitingResponse || !_connected || _commandQueue.isEmpty()
        || _commandGapTimer->isActive()) {
        return;
    }

    const ModbusCommand cmd = _commandQueue.takeFirst();
    _activeTransactionId = cmd.transactionId;
    _activeKind = cmd.kind;
    _activeCoilIndex = cmd.coilIndex;
    _awaitingResponse = true;

    // TcpClient 按十六进制字符串发送原始 ADU
    _tcpClient->sendMessage(cmd.payload.toHex(), 0);
    _responseTimer->start();
}

/**
 * @brief 上一帧结束后，间隔 kCommandIntervalMs 再发下一帧
 *
 * 首条命令入队时可直接 pump；后续由 finish/timeout → scheduleNextCommand 驱动。
 */
void ModBusMasterCoilControllerDataModel::scheduleNextCommand()
{
    if (!_connected || _commandQueue.isEmpty() || _awaitingResponse) {
        return;
    }
    _commandGapTimer->start();
}

void ModBusMasterCoilControllerDataModel::onCommandGapTimeout()
{
    pumpCommandQueue();
}

void ModBusMasterCoilControllerDataModel::clearCommandQueue()
{
    _commandQueue.clear();
    _awaitingResponse = false;
    _responseTimer->stop();
    _commandGapTimer->stop();
}

// =============================================================================
// 同步：以节点期望为准（0x01 读 / 0x05 写）
// =============================================================================

/** 入队 FC 0x01：从起始 PDU 起读 _coilCount 路 */
void ModBusMasterCoilControllerDataModel::readAllCoils()
{
    if (!_connected || _coilCount <= 0) {
        return;
    }
    const QByteArray command = generateReadCoilsCommand(
        _coilStartAddress, static_cast<quint16>(_coilCount));
    enqueueModbusCommand(command, ModbusCommandKind::ReadCoils);
}

/** 入队 FC 0x05：写单路线圈为当前期望值 */
void ModBusMasterCoilControllerDataModel::writeCoil(int index)
{
    if (!_connected || index < 0 || index >= _coilCount) {
        return;
    }
    const quint16 address = static_cast<quint16>(_coilStartAddress + index);
    const QByteArray command = generateWriteSingleCoilCommand(address, _desiredStates[index]);
    enqueueModbusCommand(command, ModbusCommandKind::WriteSingleCoil, index);
}

/**
 * @brief 将节点期望同步到设备（逐点 0x05）
 * @param onlyMismatch true：只写「期望 ≠ 设备缓存」的路；false：写全部
 *
 * 先 dropPendingWrites，再批量入队；串行发送且帧间间隔约 5ms。
 */
void ModBusMasterCoilControllerDataModel::syncDesiredToDevice(bool onlyMismatch)
{
    if (!_connected || _coilCount <= 0) {
        return;
    }

    dropPendingWrites();

    for (int i = 0; i < _coilCount; ++i) {
        if (onlyMismatch && _desiredStates[i] == _deviceStates[i]) {
            continue;
        }
        const quint16 address = static_cast<quint16>(_coilStartAddress + i);
        const QByteArray command = generateWriteSingleCoilCommand(address, _desiredStates[i]);
        ModbusCommand item;
        item.payload = command;
        item.transactionId = transactionIdFromCommand(command);
        item.kind = ModbusCommandKind::WriteSingleCoil;
        item.coilIndex = i;
        _commandQueue.append(item);
    }

    pumpCommandQueue();
}

/**
 * @brief TCP 连接状态变化
 *
 * 连上：启动读轮询，并立即以节点期望整段写回设备。
 * 断开：停轮询、清空队列。
 */
void ModBusMasterCoilControllerDataModel::setConnected(bool connected)
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
        // 以节点期望为准，立即写回设备
        syncDesiredToDevice(false);
    } else {
        _readTimer->stop();
        clearCommandQueue();
    }
}

/**
 * @brief 更新本地期望状态（界面 / 端口 / OSC）
 *
 * 已连接时立即对该路入队 0x05，不必等下次 100ms 轮询。
 */
void ModBusMasterCoilControllerDataModel::setCoil(int index, bool state)
{
    if (index < 0 || index >= _coilCount) {
        return;
    }
    if (_desiredStates[index] == state) {
        return;
    }

    _desiredStates[index] = state;
    _interface->setCoilState(index, state);
    AbstractDelegateModel::stateFeedBack(QString("/C%1").arg(index), state);

    if (_connected) {
        writeCoil(index);
    }
}

/** 当前命令结束：停超时定时器，按指令间隔调度下一帧 */
void ModBusMasterCoilControllerDataModel::finishActiveCommand(bool success)
{
    Q_UNUSED(success);
    _responseTimer->stop();
    _awaitingResponse = false;
    scheduleNextCommand();
}

void ModBusMasterCoilControllerDataModel::onResponseTimeout()
{
    if (!_awaitingResponse) {
        return;
    }
    // 超时则放弃等待，继续队列后续命令
    _awaitingResponse = false;
    _responseTimer->stop();
    scheduleNextCommand();
}

/**
 * @brief 按 MBAP Length 拆分可能粘在一起的多帧应答
 *
 * frameSize = 6 + Length（Length 含 Unit + PDU）
 */
void ModBusMasterCoilControllerDataModel::processModbusResponse(const QByteArray &response)
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

/**
 * @brief 处理单帧应答
 *
 * - 必须匹配当前在途 transactionId
 * - 功能码最高位为 1：异常，结束本命令
 * - 0x01：更新设备缓存；若与期望不一致，只写差异点
 * - 0x05：乐观更新对应路设备缓存，减少重复写
 */
void ModBusMasterCoilControllerDataModel::handleSingleFrame(const QByteArray &frame)
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
    // 非当前等待的事务：丢弃（可能是迟到/错序包）
    if (!_awaitingResponse || transactionId != _activeTransactionId) {
        return;
    }
    if (functionCode & 0x80) {
        finishActiveCommand(false);
        return;
    }

    switch (functionCode) {
    case 0x01: {
        // 读线圈应答：字节数 + 位图（低位对应靠前的线圈）
        if (frame.size() >= 9) {
            const quint8 byteCount = static_cast<quint8>(frame[8]);
            if (frame.size() >= 9 + byteCount) {
                bool mismatch = false;
                for (int i = 0; i < _coilCount; ++i) {
                    const int byteIndex = i / 8;
                    const int bitIndex = i % 8;
                    if (byteIndex >= byteCount) {
                        break;
                    }
                    const quint8 coilByte = static_cast<quint8>(frame[9 + byteIndex]);
                    const bool deviceState = (coilByte & (1 << bitIndex)) != 0;
                    _deviceStates[i] = deviceState;
                    if (_desiredStates[i] != deviceState) {
                        mismatch = true;
                    }
                }
                if (mismatch) {
                    // 以节点为准，只写不一致的线圈（指令间隔 5ms）
                    syncDesiredToDevice(true);
                }
            }
        }
        finishActiveCommand(true);
        break;
    }
    case 0x05: {
        // 写成功：乐观认为设备已等于期望，避免下一轮读前重复写
        if (_activeCoilIndex >= 0 && _activeCoilIndex < _coilCount) {
            _deviceStates[_activeCoilIndex] = _desiredStates[_activeCoilIndex];
        }
        finishActiveCommand(true);
        break;
    }
    default:
        finishActiveCommand(true);
        break;
    }
}

// =============================================================================
// 请求帧构造（MBAP + PDU）
// =============================================================================

/** FC 0x01 Read Coils：起始 PDU + 数量 */
QByteArray ModBusMasterCoilControllerDataModel::generateReadCoilsCommand(quint16 startAddress,
                                                                        quint16 quantity)
{
    QByteArray command;
    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00)); // Protocol ID
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00)); // Length = 6
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

/** FC 0x05 Write Single Coil：地址 + 0xFF00(ON) / 0x0000(OFF) */
QByteArray ModBusMasterCoilControllerDataModel::generateWriteSingleCoilCommand(quint16 address,
                                                                              bool value)
{
    QByteArray command;
    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x06));
    command.append(static_cast<char>(_serverId));
    command.append(static_cast<char>(0x05));
    command.append(static_cast<char>(address >> 8));
    command.append(static_cast<char>(address & 0xFF));
    if (value) {
        command.append(static_cast<char>(0xFF));
        command.append(static_cast<char>(0x00));
    } else {
        command.append(static_cast<char>(0x00));
        command.append(static_cast<char>(0x00));
    }
    _transactionId++;
    return command;
}

} // namespace Nodes
