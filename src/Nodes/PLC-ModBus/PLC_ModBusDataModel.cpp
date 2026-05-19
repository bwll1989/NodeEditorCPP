#include "PLC_ModBusDataModel.hpp"
#include "BeckhoffMapping.hpp"
#include "SiemensMapping.hpp"
#include "../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJSEngine>
#include <QDebug>
#include "OSCSender/OSCSender.h"

namespace Nodes {

PLC_ModBusDataModel::PLC_ModBusDataModel()
    : _interface(new PLC_ModBusInterface())
    , _tcpClient(new TcpClient("127.0.0.1", 502))  // Modbus TCP默认端口502
    , _tickTimer(new QTimer(this))
    , _transactionId(0)
    , _host("127.0.0.1")
    , _port(502)
    , _serverId(1)
{
    InPortCount = 8;
    OutPortCount = 8;
    PortEditable = true;
    CaptionVisible = true;
    Caption = "PLC_ModBus";
    WidgetEmbeddable = false;
    Resizable = false;
    
    // 初始化状态数组
    for (int i = 0; i < 32; ++i) {
        _controlOutputStates[i] = false;
        _statusStates[i] = false;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>();
        _pulseSeq[i] = 0;
    }
    _tick = 0;
    _pollCounter = 0;

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/connect", this, b);
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
        b.member = "plcType";
        b.control = _interface->_plcTypeCombo;
        AbstractDelegateModel::registerExternalBinding("/plcType", this, b);
    }

    // UI Connections
    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });

    connect(_interface->_portEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setPort(val);
    });

    connect(_interface->_serverId, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setServerId(val);
    });

    // 读取PLC类型
    setPlcType(_plcType);

    // 根据PLC类型变换，更改映射关系及显示（统一走属性 Setter）
    connect(_interface, &PLC_ModBusInterface::plcTypeChanged, this, &PLC_ModBusDataModel::setPlcType);

    for (int i = 0; i < 32; ++i) {
        {
            NodeDelegateModel::ExternalBinding ui;
            ui.control = _interface->_controlButtons[i];
            AbstractDelegateModel::registerExternalBinding("/Z" + QString::number(i), nullptr, ui);
        }
        connect(_interface->_controlButtons[i], &QPushButton::toggled, this, [this, i](bool checked) {
            setControlOutput(i, checked);
        });
        {
            NodeDelegateModel::ExternalBinding ui;
            ui.control = _interface->_statusCheckBoxes[i];
            AbstractDelegateModel::registerExternalBinding("/S" + QString::number(i), nullptr, ui);
        }
    }
    
    // TCP Client Connections
    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("default").toByteArray(), dataMap["host"].toString(), 0);
        }
    });
    
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        setConnected(isReady);
        for(int i=0; i<32; i++) {
            _interface->_controlButtons[i]->setEnabled(isReady);
        }

        if (isReady) {
            _pendingReadRequests.clear();
            readAllData();
            if (!_tickTimer->isActive()) {
                _tickTimer->start();
            }
        } else {
            _writeQueue.clear();
            _deferredResets.clear();
            _pendingReadRequests.clear();
            _pollCounter = 0;
            if (_tickTimer->isActive()) {
                _tickTimer->stop();
            }
        }
    });

    _tickTimer->setInterval(50);
    connect(_tickTimer, &QTimer::timeout, this, &PLC_ModBusDataModel::processWriteQueue);

    // Initial sync
    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_serverId->setValue(_serverId);
}

PLC_ModBusDataModel::~PLC_ModBusDataModel()
{
    if (_tcpClient) {
        _tcpClient->disconnectFromServer();
        delete _tcpClient;
    }
    if (_tickTimer) {
        _tickTimer->stop();
    }
}

void PLC_ModBusDataModel::setHost(const QString& host) {
    if (_host == host) return;
    _host = host;
    
    QSignalBlocker blocker(_interface->_hostEdit);
    _interface->_hostEdit->setText(_host);
    
    emit hostChanged(_host);

    // Reconnect
    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void PLC_ModBusDataModel::setPort(int port) {
    if (_port == port) return;
    _port = port;
    
    QSignalBlocker blocker(_interface->_portEdit);
    _interface->_portEdit->setValue(_port);
    
    emit portChanged(_port);

    // Reconnect
    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void PLC_ModBusDataModel::setServerId(int serverId) {
    if (_serverId == serverId) return;
    _serverId = serverId;

    if (_interface && _interface->_serverId) {
        QSignalBlocker blocker(_interface->_serverId);
        _interface->_serverId->setValue(_serverId);
    }

    emit serverIdChanged(_serverId);
}

/**
 * @brief 设置 PLC 类型并更新寄存器映射与标签
 */
void PLC_ModBusDataModel::setPlcType(int plcType)
{
    if (_plcType == plcType && _mapping) return;

    updateMapping(plcType);

    if (_interface && _interface->_plcTypeCombo) {
        QSignalBlocker b(_interface->_plcTypeCombo);
        _interface->_plcTypeCombo->setCurrentIndex(_plcType);
    }

    if (_interface) {
        _interface->setZLabels(makeZLabels());
        _interface->setSLabels(makeSLabels());
    }

    Q_EMIT plcTypeChanged(_plcType);
}

/**
 * @brief 设置连接状态并同步到界面与外部反馈
 */
void PLC_ModBusDataModel::setConnected(bool connected)
{
    if (_isConnected == connected) return;
    _isConnected = connected;

    if (_interface) {
        _interface->setConnectionStatus(connected);
    }

    Q_EMIT connectedChanged(connected);

}

void PLC_ModBusDataModel::onGlobalEvent(const GlobalEvent& ev) {
    if (ev.kind != GlobalEventKind::Command) return;

    QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == "host") setHost(ev.payload.toString());
    else if (localPath == "port") setPort(ev.payload.toInt());
    else if (localPath == "serverId") setServerId(ev.payload.toInt());
    else if (localPath == "plcType") setPlcType(ev.payload.toInt());
    else if (localPath.startsWith("Z")) {
        bool ok;
        int index = localPath.mid(1).toInt(&ok);
        if (ok && index >= 0 && index < 32) {
            setControlOutput(index, ev.payload.toBool());
        }
    }
}

void PLC_ModBusDataModel::afterModelReady() {
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/host"), this,SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/port"), this,SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/serverId"), this,SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/plcType"), this,SLOT(onGlobalEvent(GlobalEvent)));
    for (int i = 0; i < 32; ++i) {
        GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress(QString("/Z%1").arg(i)), this,SLOT(onGlobalEvent(GlobalEvent)));
    }
}

NodeDataType PLC_ModBusDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> PLC_ModBusDataModel::outData(PortIndex port)
{
    if (port >= 0 && port < 32) {
        return _outputData[port];
    }
    return nullptr;
}

void PLC_ModBusDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);

    if (!varData || port < 0 || port >= 32) {
        return;
    }

    bool newState = varData->value().toBool();
    setControlOutput(port, newState);
}

QString PLC_ModBusDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        return QString("Z %1").arg(portIndex);
    case PortType::Out:
        return QString("S %1").arg(portIndex);
    default:
        break;
    }
    return "";
}

QJsonObject PLC_ModBusDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject modelJson1;

    modelJson1["host"] = _host;
    modelJson1["port"] = _port;
    modelJson1["serverId"] = _serverId;
    modelJson1["plcType"] = _plcType;

    modelJson["values"] = modelJson1;
    return modelJson;
}

void PLC_ModBusDataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject()) {
        QJsonObject values = v.toObject();

        if (values.contains("host")) setHost(values["host"].toString());
        if (values.contains("port")) setPort(values["port"].toInt());
        if (values.contains("serverId")) setServerId(values["serverId"].toInt());

        if (values.contains("plcType")) {
            const int plcType = values["plcType"].toInt();
            // 统一走属性 Setter，确保映射/标签/反馈一致
            setPlcType(plcType);
        }

        if (_mapping) {
            _registerBase = _mapping->defaultBaseAddress();
        }

        if (_interface) {
            _interface->setZLabels(makeZLabels());
            _interface->setSLabels(makeSLabels()); // 更新状态标签
        }

    }
}

ConnectionPolicy PLC_ModBusDataModel::portConnectionPolicy(PortType portType, PortIndex index) const {
    auto result = ConnectionPolicy::One;
    switch (portType) {
        case PortType::In:
            result = ConnectionPolicy::Many;
            break;
        case PortType::Out:
            result = ConnectionPolicy::Many;
            break;
        case PortType::None:
            break;
    }

    return result;
}

void PLC_ModBusDataModel::recMsg(QByteArray msg, QString ip, int port)
{
    processModbusResponse(msg);
}

void PLC_ModBusDataModel::readAllRegisters()
{
    // 仅读取状态寄存器
    const quint16 statusQty = _mapping ? static_cast<quint16>(_mapping->statusRegisterCount()) : static_cast<quint16>(2);
    const quint16 statusStart = toProtocolAddress(_mapping ? _mapping->statusRegisterBase() : 0);
    if (statusQty > 0) {
        const quint16 statusTid = _transactionId;
        QByteArray statusCommand = generateReadHoldingRegistersCommand(statusStart, statusQty);
        _pendingReadRequests[statusTid] = true; // 状态寄存器请求
        sendModbusCommand(statusCommand);
    }
}

void PLC_ModBusDataModel::readAllData()
{
    readAllRegisters();
}

void PLC_ModBusDataModel::enqueueWriteReg(int regOffset, quint16 value, bool highPriority)
{
    for (int i = _writeQueue.size() - 1; i >= 0; --i) {
        if (_writeQueue.at(i).regOffset == regOffset) {
            _writeQueue.removeAt(i);
        }
    }

    if (highPriority) {
        _writeQueue.prepend({ regOffset, value });
    } else {
        _writeQueue.append({ regOffset, value });
    }

    if (_isConnected && _tickTimer && !_tickTimer->isActive()) {
        _tickTimer->start();
    }
}

void PLC_ModBusDataModel::enqueuePulseReset(int regOffset, quint16 value)
{
    for (int i = _writeQueue.size() - 1; i >= 0; --i) {
        if (_writeQueue.at(i).regOffset == regOffset && _writeQueue.at(i).value == 0) {
            _writeQueue.removeAt(i);
        }
    }

    int last = -1;
    for (int i = _writeQueue.size() - 1; i >= 0; --i) {
        if (_writeQueue.at(i).regOffset == regOffset) {
            last = i;
            break;
        }
    }

    const int insertPos = (last >= 0) ? (last + 1) : _writeQueue.size();
    _writeQueue.insert(insertPos, { regOffset, value });

    if (_isConnected && _tickTimer && !_tickTimer->isActive()) {
        _tickTimer->start();
    }
}

void PLC_ModBusDataModel::setControlOutput(int index, bool state)
{
    if (index < 0 || index >= 32) return;

    if (!state) {
        _pulseSeq[index]++;
    }

    _controlOutputStates[index] = state;

    // 更新UI
    _interface->setControlOutputState(index, state);
    emit controlOutputChanged(index, state);

    AbstractDelegateModel::stateFeedBack(QString("/Z%1").arg(index), state);

    const int regOffset = _mapping ? _mapping->controlRegisterOffsetForIndex(index) : (index / 16);
    const quint16 val = buildRegisterValue(regOffset);

    enqueueWriteReg(regOffset, val, true);

    if (state) {
        const quint32 token = ++_pulseSeq[index];
        const DeferredReset dr{ _tick + 10, index, token }; // 10 ticks * 50ms = 500ms
        _deferredResets.append(dr);
    }
}

void PLC_ModBusDataModel::setStatus(int index, bool state)
{
    if (index < 0 || index >= 32) return;
    if (_statusStates[index] == state) return;

    _statusStates[index] = state;

    AbstractDelegateModel::stateFeedBack(QString("/S%1").arg(index), state);
    updateOutputData(index, state);
    _interface->setStatusDisplay(index, state);
    emit statusDisplayChanged(index, state);
}

void PLC_ModBusDataModel::processWriteQueue()
{
    ++_tick;

    for (int i = _deferredResets.size() - 1; i >= 0; --i) {
        const auto dr = _deferredResets.at(i);
        if (dr.dueTick > _tick) continue;
        _deferredResets.removeAt(i);

        if (dr.index < 0 || dr.index >= 32) continue;
        if (_pulseSeq[dr.index] != dr.token) continue;

        _controlOutputStates[dr.index] = false;
        // 更新UI
        _interface->setControlOutputState(dr.index, false);
        emit controlOutputChanged(dr.index, false);

        AbstractDelegateModel::stateFeedBack(QString("/Z%1").arg(dr.index), false);

        const int ro = _mapping ? _mapping->controlRegisterOffsetForIndex(dr.index) : (dr.index / 16);
        const quint16 v0 = buildRegisterValue(ro);
        enqueuePulseReset(ro, v0);
    }

    if (!_isConnected) return;

    if (!_writeQueue.isEmpty()) {
        const WriteRegCommand cmd = _writeQueue.takeFirst();
        const quint16 addr = toProtocolAddress(static_cast<quint16>(_registerBase + cmd.regOffset));
        QByteArray command = generateWriteSingleRegisterCommand(addr, cmd.value);
        sendModbusCommand(command);
        return;
    }

    // 每20个tick（1秒）轮询一次所有寄存器
    _pollCounter++;
    if (_pollCounter >= 2) {
        _pollCounter = 0;
        readAllRegisters();
    }
}

void PLC_ModBusDataModel::processModbusResponse(const QByteArray &response)
{
    if (response.size() < 8) return;
    
    // 解析Modbus TCP响应头
    quint16 transactionId = (static_cast<quint8>(response[0]) << 8) | static_cast<quint8>(response[1]);
    quint16 protocolId = (static_cast<quint8>(response[2]) << 8) | static_cast<quint8>(response[3]);
    quint16 length = (static_cast<quint8>(response[4]) << 8) | static_cast<quint8>(response[5]);
    quint8 unitId = static_cast<quint8>(response[6]);
    quint8 functionCode = static_cast<quint8>(response[7]);
    
    if (protocolId != 0) return;
    
    // 检查功能码和数据长度
    if (functionCode == 0x03 && response.size() >= 9) { // 读取保持寄存器响应
        quint8 byteCount = static_cast<quint8>(response[8]);
        if (response.size() >= 9 + byteCount) {
            QVector<quint16> regs;
            regs.reserve(byteCount / 2);
            for (int i = 0; i < byteCount / 2; ++i) {
                const int off = 9 + i * 2;
                if (off + 1 >= response.size()) break;
                const quint16 v = (static_cast<quint8>(response[off]) << 8) | static_cast<quint8>(response[off + 1]);
                regs.push_back(v);
            }

            if (_mapping && _pendingReadRequests.contains(transactionId)) {
                bool isStatus = _pendingReadRequests.take(transactionId);
                
                if (isStatus) { // 仅处理状态寄存器响应
                    bool nextStates[32];
                    for (int i = 0; i < 32; ++i) nextStates[i] = _statusStates[i];
                    _mapping->applyStatusReadRegisters(regs, nextStates, 32);

                    for (int i = 0; i < 32; ++i) {
                        setStatus(i, nextStates[i]);
                    }
                }
            }
        }
    } else if (functionCode == 0x06) { // 写单个保持寄存器响应
        // 写入响应不需要额外处理，因为写入操作是主动发起的
    } else {
        // 其他功能码或错误响应
    }
}

QByteArray PLC_ModBusDataModel::generateReadHoldingRegistersCommand(quint16 startAddress, quint16 quantity)
{
    QByteArray command;
    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x06));
    command.append(static_cast<char>(_serverId));
    command.append(static_cast<char>(0x03));
    command.append(static_cast<char>(startAddress >> 8));
    command.append(static_cast<char>(startAddress & 0xFF));
    command.append(static_cast<char>(quantity >> 8));
    command.append(static_cast<char>(quantity & 0xFF));
    _transactionId++;
    return command;
}

QByteArray PLC_ModBusDataModel::generateWriteSingleRegisterCommand(quint16 address, quint16 value)
{
    QByteArray command;
    command.append(static_cast<char>(_transactionId >> 8));
    command.append(static_cast<char>(_transactionId & 0xFF));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x06));
    command.append(static_cast<char>(_serverId));
    command.append(static_cast<char>(0x06));
    command.append(static_cast<char>(address >> 8));
    command.append(static_cast<char>(address & 0xFF));
    command.append(static_cast<char>(value >> 8));
    command.append(static_cast<char>(value & 0xFF));
    _transactionId++;
    return command;
}

void PLC_ModBusDataModel::updateMapping(int plcType)
{
    _plcType = plcType;
    if (_plcType == 0) {
        _mapping = std::make_unique<PlcModBus::BeckhoffMapping>();
    } else {
        _mapping = std::make_unique<PlcModBus::SiemensMapping>();
    }
    if (_mapping) {
        _registerBase = _mapping->defaultBaseAddress();
        _interface->setZLabels(makeZLabels());
        _interface->setSLabels(makeSLabels()); // 更新状态标签
    }
}

QStringList PLC_ModBusDataModel::makeZLabels() const
{
    if (!_mapping) return {};
    return _mapping->makeZLabels(_registerBase);
}

QStringList PLC_ModBusDataModel::makeSLabels() const
{
    if (!_mapping) return {};
    return _mapping->makeSLabels(_mapping->statusRegisterBase());
}

quint16 PLC_ModBusDataModel::buildRegisterValue(int regOffset) const
{
    if (!_mapping) return 0;
    return _mapping->buildControlRegisterValue(regOffset, _controlOutputStates, 32);
}

quint16 PLC_ModBusDataModel::buildStatusRegisterValue(int regOffset) const
{
    // 状态寄存器是只读的，不需要构建写入值
    return 0;
}

void PLC_ModBusDataModel::updateOutputData(int port, bool value)
{
    if (port >= 0 && port < 32) {
        _outputData[port] = std::make_shared<NodeDataTypes::VariableData>(value);
        Q_EMIT dataUpdated(port);
    }
}

void PLC_ModBusDataModel::sendModbusCommand(const QByteArray &command)
{
    if (_tcpClient ) {
        QString hexCommand = command.toHex();
        _tcpClient->sendMessage(hexCommand, 0);
    }
}
//处理0基或1基
quint16 PLC_ModBusDataModel::toProtocolAddress(quint16 uiAddress) const
{
    if (_mapping) {
        return _mapping->toProtocolAddress(uiAddress);
    }
    return uiAddress > 0 ? static_cast<quint16>(uiAddress - 1) : static_cast<quint16>(0);
}

} // namespace Nodes
