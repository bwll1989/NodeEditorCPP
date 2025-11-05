#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>
#include <QtQml/QJSValueIterator> 
#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "../../DataTypes/VariableData.h"
#include "USR-IO808Interface.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;
namespace Nodes {

/**
 * @brief Modbus TCP主机节点数据模型
 * 
 * 实现基于Modbus TCP协议的主机节点，支持：
 * - DO部分：8个线圈读写，寄存器地址范围0x0000~0x0007
 * - DI部分：8个离散量输入只读，寄存器地址范围0x0020~0x0027
 * - Read All功能：同步从机数据
 */
class USR_IO808DataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    USR_IO808DataModel();
    ~USR_IO808DataModel() override;

public:
    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override ;
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;
    QWidget *embeddedWidget() override { return _interface; }

private slots:
    /**
     * @brief 接收TCP消息处理槽函数
     * @param msg 接收到的消息数据
     * @param ip 发送方IP地址
     * @param port 发送方端口
     */
    void recMsg(QByteArray msg, QString ip, int port);
    
    /**
     * @brief 读取所有输入状态
     */
    void readAllInputs();
    
    /**
     * @brief 读取所有输出状态
     */
    void readAllOutputs();
    
    /**
     * @brief 设置单个输出状态
     * @param index 输出索引(0-7)
     * @param state 输出状态
     */
    void setOutput(int index, bool state);
    
    /**
     * @brief 读取所有数据（输入和输出）
     */
    void readAllData();

    void stateFeedBack(const QString& oscAddress,QVariant value) override;
private:
    USR_IO808Interface *_interface;
    TcpClient *_tcpClient;
    QTimer *_readTimer;  // 定时读取定时器

    // 设备状态
    bool _inputStates[8];   // DI状态数组
    bool _outputStates[8];  // DO状态数组
    
    // Modbus协议相关
    quint16 _transactionId; // 事务标识符
    quint8 _serverId;       // 服务器ID
    
    // 输出数据缓存
    std::shared_ptr<NodeDataTypes::VariableData> _outputData[8];

    /**
     * @brief 处理Modbus TCP响应
     * @param response 响应数据
     */
    void processModbusResponse(const QByteArray &response);

    /**
     * @brief 生成Modbus TCP读取线圈命令
     * @param startAddress 起始地址
     * @param quantity 数量
     * @return Modbus TCP命令字节数组
     */
    QByteArray generateReadCoilsCommand(quint16 startAddress, quint16 quantity);
    
    /**
     * @brief 生成Modbus TCP读取离散输入命令
     * @param startAddress 起始地址
     * @param quantity 数量
     * @return Modbus TCP命令字节数组
     */
    QByteArray generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity);
    
    /**
     * @brief 生成Modbus TCP写单个线圈命令
     * @param address 线圈地址
     * @param value 线圈值
     * @return Modbus TCP命令字节数组
     */
    QByteArray generateWriteSingleCoilCommand(quint16 address, bool value);
    
    /**
     * @brief 生成Modbus TCP写多个线圈命令
     * @param startAddress 起始地址
     * @param values 线圈值数组
     * @param quantity 数量
     * @return Modbus TCP命令字节数组
     */
    QByteArray generateWriteMultipleCoilsCommand(quint16 startAddress, const QVector<bool> &values, quint16 quantity);
    
    /**
     * @brief 计算CRC16校验码（如果需要）
     * @param data 数据
     * @return CRC16校验码
     */
    quint16 calculateCRC16(const QByteArray &data);
    
    /**
     * @brief 更新输出端口数据
     * @param port 端口索引
     * @param value 数据值
     */
    void updateOutputData(int port, bool value);
    
    /**
     * @brief 发送Modbus命令
     * @param command 命令数据
     */
    void sendModbusCommand(const QByteArray &command);
};

} // namespace Nodes