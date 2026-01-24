#include "USR-IO808DataModel.hpp"
#include "../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJSEngine>
#include <QDateTime>
#include <QCoreApplication>
#include <QDebug>

#include "ConstantDefines.h"
#include "OSCMessage.h"
#include "OSCSender/OSCSender.h"

namespace Nodes {

USR_IO808DataModel::USR_IO808DataModel()
    : _interface(new USR_IO808Interface())
    , _tcpClient(new TcpClient("127.0.0.1", 8080))  // Modbus TCP默认端口502
    , _readTimer(new QTimer(this))
    , _writeQueueTimer(new QTimer(this))
    , _transactionId(0)
    , _host("127.0.0.1")
    , _port(8080)
    , _serverId(1)
{
    InPortCount = 8;
    OutPortCount = 8;
    PortEditable = false;
    CaptionVisible = true;
    Caption = "USR-IO808";
    WidgetEmbeddable = false;
    Resizable = false;
    
    // 初始化状态数组
    for (int i = 0; i < 8; ++i) {
        _inputStates[i] = false;
        _outputStates[i] = false;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>();
    }
    AbstractDelegateModel::registerExternalControl("/connect", _interface->_statusLabel);
    // UI Connections
    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });

    connect(_interface->_portEdit, &QSpinBox::valueChanged, this, [this](int val) {
        setPort(val);
    });

    connect(_interface->_serverId, &QSpinBox::valueChanged, this, [this](int val) {
        setServerId(val);
    });

    for (int i = 0; i < 8; ++i) {
        AbstractDelegateModel::registerExternalControl("/DO" + QString::number(i), _interface->_outputCheckBoxes[i]);
        AbstractDelegateModel::registerExternalControl("/DI" + QString::number(i), _interface->_inputLabels[i]);
        connect(_interface->_outputCheckBoxes[i], &QCheckBox::clicked, this, [this, i](bool checked) {
            setOutput(i, checked);
        });
    }

    // Read All Button
    connect(_interface->_readAll, &QPushButton::clicked, this, &USR_IO808DataModel::readAllData);
    
    // TCP Client Connections
    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("default").toByteArray(), dataMap["host"].toString(), 0);
        }
    });
    
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        _interface->setConnectionStatus(isReady);
        AbstractDelegateModel::stateFeedBack("/connect",isReady);
        _interface->_readAll->setEnabled(isReady);
        for(int i=0; i<8; i++) {
            _interface->_outputCheckBoxes[i]->setEnabled(isReady);
        }

        if (isReady) {
            readAllData();
            _readTimer->start(1000); 
        } else {
            _readTimer->stop();
        }
    });

    connect(_readTimer, &QTimer::timeout, this, &USR_IO808DataModel::readAllInputs);

    // Write Queue Timer Setup
    _writeQueueTimer->setInterval(1000); // 1s interval
    connect(_writeQueueTimer, &QTimer::timeout, this, &USR_IO808DataModel::processWriteQueue);

    // Initial sync
    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_serverId->setValue(_serverId);
}

USR_IO808DataModel::~USR_IO808DataModel()
{
    if (_tcpClient) {
        _tcpClient->disconnectFromServer();
        delete _tcpClient;
    }
    if (_readTimer) {
        _readTimer->stop();
    }
    if (_writeQueueTimer) {
        _writeQueueTimer->stop();
    }
}

void USR_IO808DataModel::setHost(const QString& host) {
    if (_host == host) return;
    _host = host;
    
    QSignalBlocker blocker(_interface->_hostEdit);
    _interface->_hostEdit->setText(_host);
    
    emit hostChanged(_host);
    AbstractDelegateModel::stateFeedBack("/host", _host);

    // Reconnect
    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void USR_IO808DataModel::setPort(int port) {
    if (_port == port) return;
    _port = port;
    
    QSignalBlocker blocker(_interface->_portEdit);
    _interface->_portEdit->setValue(_port);
    
    emit portChanged(_port);
    AbstractDelegateModel::stateFeedBack("/port", _port);

    // Reconnect
    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void USR_IO808DataModel::setServerId(int serverId) {
    if (_serverId == serverId) return;
    _serverId = serverId;
    
    QSignalBlocker blocker(_interface->_serverId);
    _interface->_serverId->setValue(_serverId);
    
    emit serverIdChanged(_serverId);
    AbstractDelegateModel::stateFeedBack("/serverId", _serverId);
}

void USR_IO808DataModel::onGlobalEvent(const GlobalEvent& ev) {
    if (ev.kind == GlobalEventKind::Command) {
        QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
        if (localPath == "host") setHost(ev.payload.toString());
        else if (localPath == "port") setPort(ev.payload.toInt());
        else if (localPath == "serverId") setServerId(ev.payload.toInt());
        else if (localPath == "readAll") readAllData();
        else if (localPath.startsWith("DO")) {
            bool ok;
            int index = localPath.mid(2).toInt(&ok);
            if (ok && index >= 0 && index < 8) {
                setOutput(index, ev.payload.toBool());
            }
        }
    }
}

void USR_IO808DataModel::afterModelReady() {
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/host"), this,SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/port"), this,SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/serverId"), this,SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/readAll"), this,SLOT(onGlobalEvent(GlobalEvent)));
    
    for (int i = 0; i < 8; ++i) {
        GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress(QString("/DO%1").arg(i)), this,SLOT(onGlobalEvent(GlobalEvent)));
    }
}

NodeDataType USR_IO808DataModel::dataType(PortType portType, PortIndex portIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> USR_IO808DataModel::outData(PortIndex port)
{
    if (port >= 0 && port < 8) {
        return _outputData[port];
    }
    return nullptr;
}

void USR_IO808DataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    
    if (!varData || port < 0 || port >= 8) {
        return;
    }
    
    bool newState = varData->value().toBool();
    setOutput(port, newState);
}

QString USR_IO808DataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

QJsonObject USR_IO808DataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject modelJson1;
    
    // 保存输出状态
    QJsonArray outputStates;
    for (int i = 0; i < 8; ++i) {
        outputStates.append(_outputStates[i]);
    }
    modelJson1["outputStates"] = outputStates;
    modelJson1["host"] = _host;
    modelJson1["port"] = _port;
    modelJson1["serverId"] = _serverId;
    
    modelJson["values"] = modelJson1;
    return modelJson;
}

void USR_IO808DataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject()) {
        QJsonObject values = v.toObject();
        
        if (values.contains("host")) setHost(values["host"].toString());
        if (values.contains("port")) setPort(values["port"].toInt());
        if (values.contains("serverId")) setServerId(values["serverId"].toInt());
        
        // 加载输出状态
        if (values.contains("outputStates")) {
            QJsonArray outputStates = values["outputStates"].toArray();
            for (int i = 0; i < outputStates.size() && i < 8; ++i) {
                setOutput(i, outputStates[i].toBool());
            }
        }
    }
}

ConnectionPolicy USR_IO808DataModel::portConnectionPolicy(PortType portType, PortIndex index) const {
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

void USR_IO808DataModel::recMsg(QByteArray msg, QString ip, int port)
{
    processModbusResponse(msg);
}

void USR_IO808DataModel::readAllInputs()
{
    QByteArray command = generateReadDiscreteInputsCommand(0x0020, 8);
    sendModbusCommand(command);
}

void USR_IO808DataModel::readAllOutputs()
{
    QByteArray command = generateReadCoilsCommand(0x0000, 8);
    sendModbusCommand(command);
}

void USR_IO808DataModel::readAllData()
{
    readAllInputs();
    QTimer::singleShot(100, this, [this]() {
        readAllOutputs();
    });
}

void USR_IO808DataModel::setOutput(int index, bool state)
{
    if (index < 0 || index >= 8) return;
    
    _outputStates[index] = state;
    
    QSignalBlocker blocker(_interface->_outputCheckBoxes[index]);
    _interface->_outputCheckBoxes[index]->setChecked(state);

    AbstractDelegateModel::stateFeedBack(QString("/DO%1").arg(index), state);
    
    // 添加到队列
    _writeQueue.enqueue({index, state});
    
    // 如果定时器未运行，立即触发
    if (!_writeQueueTimer->isActive()) {
        _writeQueueTimer->start(0);
    }
}

void USR_IO808DataModel::processWriteQueue()
{
    if (_writeQueue.isEmpty()) {
        _writeQueueTimer->stop();
        return;
    }

    // 取出最早的一个指令
    WriteCommand cmd = _writeQueue.dequeue();
    QByteArray command = generateWriteSingleCoilCommand(cmd.index, cmd.state);
    sendModbusCommand(command);

    // 设置下一次触发为1秒后
    _writeQueueTimer->start(1000);
}

void USR_IO808DataModel::processModbusResponse(const QByteArray &response)
{
    if (response.size() < 8) return;
    
    // 解析Modbus TCP响应头
    quint16 transactionId = (static_cast<quint8>(response[0]) << 8) | static_cast<quint8>(response[1]);
    quint16 protocolId = (static_cast<quint8>(response[2]) << 8) | static_cast<quint8>(response[3]);
    quint16 length = (static_cast<quint8>(response[4]) << 8) | static_cast<quint8>(response[5]);
    quint8 unitId = static_cast<quint8>(response[6]);
    quint8 functionCode = static_cast<quint8>(response[7]);
    
    if (protocolId != 0) return;
    
    switch (functionCode) {
    case 0x01: // 读取线圈状态响应
        if (response.size() >= 10) {
            quint8 byteCount = static_cast<quint8>(response[8]);
            if (response.size() >= 9 + byteCount) {
                quint8 coilData = static_cast<quint8>(response[9]);
                // 更新DO状态
                for (int i = 0; i < 8; ++i) {
                    bool state = (coilData & (1 << i)) != 0;
                    if (_outputStates[i] != state) {
                        _outputStates[i] = state;
                        QSignalBlocker blocker(_interface->_outputCheckBoxes[i]);
                        _interface->_outputCheckBoxes[i]->setChecked(state);
                        AbstractDelegateModel::stateFeedBack(QString("/DO%1").arg(i), state);
                    }
                }
            }
        }
        break;
        
    case 0x02: // 读取离散输入状态响应
        if (response.size() >= 10) {
            quint8 byteCount = static_cast<quint8>(response[8]);
            if (response.size() >= 9 + byteCount) {
                quint8 inputData = static_cast<quint8>(response[9]);
                // 更新DI状态
                for (int i = 0; i < 8; ++i) {
                    bool state = (inputData & (1 << i)) != 0;
                    if (_inputStates[i] != state) {
                        _inputStates[i] = state;
                        _interface->setInputState(i, state);
                        updateOutputData(i, state);
                        AbstractDelegateModel::stateFeedBack(QString("/DI%1").arg(i), state);
                    }
                }

            }
        }
        break;
        
    case 0x05: // 写单个线圈响应
        break;
        
    case 0x0F: // 写多个线圈响应
        break;
        
    default:
        break;
    }
}

QByteArray USR_IO808DataModel::generateReadCoilsCommand(quint16 startAddress, quint16 quantity)
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

QByteArray USR_IO808DataModel::generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity)
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

QByteArray USR_IO808DataModel::generateWriteSingleCoilCommand(quint16 address, bool value)
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
    command.append(static_cast<char>(value ? 0xFF : 0x00));
    command.append(static_cast<char>(0x00));
    _transactionId++;
    return command;
}

QByteArray USR_IO808DataModel::generateWriteMultipleCoilsCommand(quint16 startAddress, const QVector<bool> &values, quint16 quantity)
{
    QByteArray command;
    quint8 byteCount = (quantity + 7) / 8;
    
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

void USR_IO808DataModel::updateOutputData(int port, bool value)
{
    if (port >= 0 && port < 8) {
        _outputData[port] = std::make_shared<NodeDataTypes::VariableData>(value);
        Q_EMIT dataUpdated(port);
    }
}

void USR_IO808DataModel::sendModbusCommand(const QByteArray &command)
{
    if (_tcpClient ) {
        QString hexCommand = command.toHex();
        _tcpClient->sendMessage(hexCommand, 0);
    }
}

} // namespace Nodes
