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
    , _transactionId(0)
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
    
    // 注册OSC控制
    NodeDelegateModel::registerOSCControl("/status", _interface->_statusLabel);
    for (int i = 0; i < 8; ++i) {
        NodeDelegateModel::registerOSCControl("/DO" + QString::number(i), _interface->_outputCheckBoxes[i]);
        NodeDelegateModel::registerOSCControl("/DI" + QString::number(i), _interface->_inputLabels[i]);
    }
    
    // 连接TCP客户端信号
    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("default").toByteArray(), dataMap["host"].toString(), 0);
        }
    });
    
    // 连接TCP客户端连接状态信号
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        _interface->setConnectionStatus(isReady);
        if (isReady) {
            // 连接成功后，读取所有输入和输出状态
            readAllData();
            // 启动定时读取
            _readTimer->start(1000); // 每秒读取一次
        } else {
            _readTimer->stop();
        }
    });
    
    // 连接输出状态改变信号
    connect(_interface, &USR_IO808Interface::outputChanged, this, [this](int index, bool state) {
        if (index >= 0 && index < 8) {
            setOutput(index, state);
        }
    });
    
    // 连接主机和端口变更信号
    connect(_interface, &USR_IO808Interface::hostChanged, this, [this](const QString &host) {
        _tcpClient->disconnectFromServer();
        _tcpClient->connectToServer(host, _interface->getPort());
    });
    
    connect(_interface, &USR_IO808Interface::portChanged, this, [this](int port) {
        _tcpClient->disconnectFromServer();
        _tcpClient->connectToServer(_interface->getHost(), port);
    });
    
    // 连接Read All按钮
    connect(_interface->_readAll, &QPushButton::clicked, this, [this]() {
        readAllData();
    });
    
    // 设置定时读取
    connect(_readTimer, &QTimer::timeout, this, [this]() {
        readAllInputs(); // 定时读取输入状态
    });
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
    if (_outputStates[port] != newState) {
        _outputStates[port] = newState;
        _interface->_outputCheckBoxes[port]->setChecked(newState);
        // setOutput(port, newState);
    }
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
    modelJson1["host"] = _interface->getHost();
    modelJson1["port"] = _interface->getPort();
    modelJson1["serverId"] = _serverId;
    
    modelJson["values"] = modelJson1;
    return modelJson;
}

void USR_IO808DataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject()) {
        QJsonObject values = v.toObject();
        
        // 加载主机和端口信息
        if (values.contains("host")) {
            QString host = values["host"].toString();
            _interface->setHost(host);
        }
        
        if (values.contains("port")) {
            int port = values["port"].toInt();
            _interface->setPort(port);
        }
        
        if (values.contains("serverId")) {
            _serverId = values["serverId"].toInt();
        }
        
        // 加载输出状态
        if (values.contains("outputStates")) {
            QJsonArray outputStates = values["outputStates"].toArray();
            for (int i = 0; i < outputStates.size() && i < 8; ++i) {
                _outputStates[i] = outputStates[i].toBool();
                _interface->setOutputState(i, _outputStates[i]);
            }
        }
        
        // 连接到服务器
        if (!_interface->getHost().isEmpty() && _interface->getPort() > 0) {
            _tcpClient->connectToServer(_interface->getHost(), _interface->getPort());
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
    // 处理接收到的Modbus TCP响应
    processModbusResponse(msg);
}

void USR_IO808DataModel::readAllInputs()
{
    // 读取DI状态（地址0x0020~0x0027）
    QByteArray command = generateReadDiscreteInputsCommand(0x0020, 8);
    sendModbusCommand(command);
}

void USR_IO808DataModel::readAllOutputs()
{
    // 读取DO状态（地址0x0000~0x0007）
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
    if (index < 0 || index >= 8) {
        return;
    }
    
    _outputStates[index] = state;
    
    // 发送写单个线圈命令（地址0x0000 + index）
    QByteArray command = generateWriteSingleCoilCommand(0x0000 + index, state);
    sendModbusCommand(command);
    
    // qDebug() << QString("设置输出 DO%1 为 %2").arg(index).arg(state ? "开" : "关");
}

void USR_IO808DataModel::processModbusResponse(const QByteArray &response)
{
    if (response.size() < 8) {
        // qDebug() << "Modbus响应数据长度不足";
        return;
    }
    
    // 解析Modbus TCP响应头
    quint16 transactionId = (static_cast<quint8>(response[0]) << 8) | static_cast<quint8>(response[1]);
    quint16 protocolId = (static_cast<quint8>(response[2]) << 8) | static_cast<quint8>(response[3]);
    quint16 length = (static_cast<quint8>(response[4]) << 8) | static_cast<quint8>(response[5]);
    quint8 unitId = static_cast<quint8>(response[6]);
    quint8 functionCode = static_cast<quint8>(response[7]);
    
    if (protocolId != 0) {
        qDebug() << "无效的Modbus TCP协议标识符";
        return;
    }
    
    switch (functionCode) {
    case 0x01: // 读取线圈状态响应
        if (response.size() >= 10) {
            quint8 byteCount = static_cast<quint8>(response[8]);
            if (response.size() >= 9 + byteCount) {
                quint8 coilData = static_cast<quint8>(response[9]);
                // 更新DO状态
                for (int i = 0; i < 8; ++i) {
                    bool state = (coilData & (1 << i)) != 0;
                    _outputStates[i] = state;
                    _interface->setOutputState(i, state);
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
                    }
                }

            }
        }
        break;
        
    case 0x05: // 写单个线圈响应
        // qDebug() << "写单个线圈操作完成";
        break;
        
    case 0x0F: // 写多个线圈响应
        // qDebug() << "写多个线圈操作完成";
        break;
        
    default:
        if (functionCode & 0x80) {
            // 错误响应
            if (response.size() >= 9) {
                quint8 exceptionCode = static_cast<quint8>(response[8]);
                // qDebug() << QString("Modbus异常响应，异常码: 0x%1").arg(exceptionCode, 2, 16, QChar('0'));
            }
        } else {
            qDebug() << QString("未处理的功能码: 0x%1").arg(functionCode, 2, 16, QChar('0'));
        }
        break;
    }
}

QByteArray USR_IO808DataModel::generateReadCoilsCommand(quint16 startAddress, quint16 quantity)
{
    QByteArray command;
    
    // Modbus TCP头
    command.append(static_cast<char>(_transactionId >> 8));   // 事务标识符高字节
    command.append(static_cast<char>(_transactionId & 0xFF)); // 事务标识符低字节
    command.append(static_cast<char>(0x00));                  // 协议标识符高字节
    command.append(static_cast<char>(0x00));                  // 协议标识符低字节
    command.append(static_cast<char>(0x00));                  // 长度高字节
    command.append(static_cast<char>(0x06));                  // 长度低字节（6字节）
    command.append(static_cast<char>(_serverId));             // 单元标识符
    
    // PDU
    command.append(static_cast<char>(0x01));                  // 功能码：读取线圈
    command.append(static_cast<char>(startAddress >> 8));     // 起始地址高字节
    command.append(static_cast<char>(startAddress & 0xFF));   // 起始地址低字节
    command.append(static_cast<char>(quantity >> 8));         // 数量高字节
    command.append(static_cast<char>(quantity & 0xFF));       // 数量低字节
    
    _transactionId++;
    return command;
}

QByteArray USR_IO808DataModel::generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity)
{
    QByteArray command;
    
    // Modbus TCP头
    command.append(static_cast<char>(_transactionId >> 8));   // 事务标识符高字节
    command.append(static_cast<char>(_transactionId & 0xFF)); // 事务标识符低字节
    command.append(static_cast<char>(0x00));                  // 协议标识符高字节
    command.append(static_cast<char>(0x00));                  // 协议标识符低字节
    command.append(static_cast<char>(0x00));                  // 长度高字节
    command.append(static_cast<char>(0x06));                  // 长度低字节（6字节）
    command.append(static_cast<char>(_serverId));             // 单元标识符
    
    // PDU
    command.append(static_cast<char>(0x02));                  // 功能码：读取离散输入
    command.append(static_cast<char>(startAddress >> 8));     // 起始地址高字节
    command.append(static_cast<char>(startAddress & 0xFF));   // 起始地址低字节
    command.append(static_cast<char>(quantity >> 8));         // 数量高字节
    command.append(static_cast<char>(quantity & 0xFF));       // 数量低字节
    
    _transactionId++;
    return command;
}

QByteArray USR_IO808DataModel::generateWriteSingleCoilCommand(quint16 address, bool value)
{
    QByteArray command;
    
    // Modbus TCP头
    command.append(static_cast<char>(_transactionId >> 8));   // 事务标识符高字节
    command.append(static_cast<char>(_transactionId & 0xFF)); // 事务标识符低字节
    command.append(static_cast<char>(0x00));                  // 协议标识符高字节
    command.append(static_cast<char>(0x00));                  // 协议标识符低字节
    command.append(static_cast<char>(0x00));                  // 长度高字节
    command.append(static_cast<char>(0x06));                  // 长度低字节（6字节）
    command.append(static_cast<char>(_serverId));             // 单元标识符
    
    // PDU
    command.append(static_cast<char>(0x05));                  // 功能码：写单个线圈
    command.append(static_cast<char>(address >> 8));          // 线圈地址高字节
    command.append(static_cast<char>(address & 0xFF));        // 线圈地址低字节
    command.append(static_cast<char>(value ? 0xFF : 0x00));   // 线圈值高字节
    command.append(static_cast<char>(0x00));                  // 线圈值低字节
    
    _transactionId++;
    return command;
}

QByteArray USR_IO808DataModel::generateWriteMultipleCoilsCommand(quint16 startAddress, const QVector<bool> &values, quint16 quantity)
{
    QByteArray command;
    
    // 计算字节数
    quint8 byteCount = (quantity + 7) / 8;
    
    // Modbus TCP头
    command.append(static_cast<char>(_transactionId >> 8));   // 事务标识符高字节
    command.append(static_cast<char>(_transactionId & 0xFF)); // 事务标识符低字节
    command.append(static_cast<char>(0x00));                  // 协议标识符高字节
    command.append(static_cast<char>(0x00));                  // 协议标识符低字节
    command.append(static_cast<char>(0x00));                  // 长度高字节
    command.append(static_cast<char>(7 + byteCount));         // 长度低字节
    command.append(static_cast<char>(_serverId));             // 单元标识符
    
    // PDU
    command.append(static_cast<char>(0x0F));                  // 功能码：写多个线圈
    command.append(static_cast<char>(startAddress >> 8));     // 起始地址高字节
    command.append(static_cast<char>(startAddress & 0xFF));   // 起始地址低字节
    command.append(static_cast<char>(quantity >> 8));         // 数量高字节
    command.append(static_cast<char>(quantity & 0xFF));       // 数量低字节
    command.append(static_cast<char>(byteCount));             // 字节数
    
    // 线圈值
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

/**
 * @brief 发送Modbus命令
 * @param command Modbus命令字节数组
 */
void USR_IO808DataModel::sendModbusCommand(const QByteArray &command)
{
    if (_tcpClient) {
        // 将QByteArray转换为十六进制字符串，因为TcpClient::sendMessage需要QString参数
        QString hexCommand = command.toHex();
        // qDebug() << "发送Modbus命令: " << command.toHex(' '); // 带空格的十六进制显示，便于调试
        // 发送命令，格式参数设为0（16进制）
        _tcpClient->sendMessage(hexCommand, 0);
    } else {
        qDebug() << "TCP Server 未连接，无法发送Modbus命令";
    }
}

void USR_IO808DataModel::stateFeedBack(const QString& oscAddress,QVariant value) {

    OSCMessage message;
    message.host = AppConstants::EXTRA_FEEDBACK_HOST;
    message.port = AppConstants::EXTRA_FEEDBACK_PORT;
    message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
    message.value = value;
    OSCSender::instance()->sendOSCMessageWithQueue(message);
}
} // namespace Nodes