#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <QtCore/QQueue>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QMap>

#include <memory>
#include <QStringList>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>
#include <QtQml/QJSValueIterator> 
#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "PLC_ModBusInterface.hpp"
#include "IPlcRegisterMapping.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;
namespace Nodes {

/**
 * @brief PLC Modbus TCP 节点数据模型
 *
 * 通过 Modbus TCP 的保持寄存器(0x03/0x06) 实现 32 路 Z 控制。
 *
 * 地址说明：界面展示通常为 1 基地址（如 Siemens 40001、Beckhoff 12789），
 * 实际 Modbus PDU 的起始地址在部分设备/网关上按 0 基解释，因此需要在发送前进行地址转换。
 */
class PLC_ModBusDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(int plcType READ plcType WRITE setPlcType NOTIFY plcTypeChanged)
    Q_PROPERTY(bool connected READ connected WRITE setConnected NOTIFY connectedChanged)

public:
    PLC_ModBusDataModel();
    ~PLC_ModBusDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString& host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    /**
     * @brief 获取 PLC 类型索引（0=Beckhoff, 1=Siemens）
     */
    int plcType() const { return _plcType; }

    /**
     * @brief 设置 PLC 类型并更新寄存器映射与标签
     */
    void setPlcType(int plcType);

    /**
     * @brief 获取连接状态
     */
    bool connected() const { return _isConnected; }

    /**
     * @brief 设置连接状态并同步到界面与外部反馈
     */
    void setConnected(bool connected);

    void afterModelReady() override;

public:
    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override ;
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;
    QWidget *embeddedWidget() override { return _interface; }

signals:
    void hostChanged(QString host);
    void portChanged(int port);
    void serverIdChanged(int serverId);
    void plcTypeChanged(int plcType);
    void connectedChanged(bool connected);
    void controlOutputChanged(int index, bool state); // 控制输出状态改变信号
    void statusDisplayChanged(int index, bool state); // 状态显示改变信号

private slots:
    void onGlobalEvent(const GlobalEvent& ev);
    /**
     * @brief 接收TCP消息处理槽函数
     * @param msg 接收到的消息数据
     * @param ip 发送方IP地址
     * @param port 发送方端口
     */
    void recMsg(QByteArray msg, QString ip, int port);
    
    void readAllRegisters(); // 修改为读取所有寄存器，包括控制和状态
    
    /**
     * @brief 设置单个控制输出状态
     * @param index 输出索引(0-31)
     * @param state 输出状态
     */
    void setControlOutput(int index, bool state);

    /**
     * @brief 设置单个状态显示
     * @param index 状态索引(0-31)
     * @param state 状态值
     */
    void setStatus(int index, bool state);
    
    /**
     * @brief 读取所有数据（输入和输出）
     */
    void readAllData();

private slots:
    void processWriteQueue();

private:
    PLC_ModBusInterface *_interface;
    TcpClient *_tcpClient;
    QTimer *_tickTimer;
    int _tick = 0;
    int _pollCounter = 0;

    struct WriteRegCommand {
        int regOffset;
        quint16 value; // 快照值
    };

    QList<WriteRegCommand> _writeQueue; // 寄存器写命令队列（携带快照值）
    quint32 _pulseSeq[32];

    struct DeferredReset {
        int dueTick;
        int index;
        quint32 token;
    };
    QList<DeferredReset> _deferredResets;

    void enqueueWriteReg(int regOffset, quint16 value, bool highPriority);
    void enqueuePulseReset(int regOffset, quint16 value);

    // 控制输出状态（32 路）
    bool _controlOutputStates[32];
    // 状态显示状态（32 路）
    bool _statusStates[32];
    
    // Modbus协议相关
    quint16 _transactionId; // 事务标识符
    bool _isConnected = false;
    bool _writePending = false;
    QMap<quint16, bool> _pendingReadRequests; // Transaction ID -> isStatusRequest (true if status, false if control)
    
    // Properties
    QString _host = "127.0.0.1";
    int _port = 502;
    int _serverId = 1;
    int _plcType = 0;
    quint16 _registerBase = 12789;

    std::unique_ptr<PlcModBus::IPlcRegisterMapping> _mapping;

    void updateMapping(int plcType);
    QStringList makeZLabels() const;
    QStringList makeSLabels() const; // 新增状态标签生成函数

    // 输出数据缓存（32 路）
    std::shared_ptr<NodeDataTypes::VariableData> _outputData[32];

    /**
     * @brief 处理Modbus TCP响应
     * @param response 响应数据
     */
    void processModbusResponse(const QByteArray &response);

    QByteArray generateReadHoldingRegistersCommand(quint16 startAddress, quint16 quantity);
    QByteArray generateWriteSingleRegisterCommand(quint16 address, quint16 value);
    quint16 buildRegisterValue(int regOffset) const;
    quint16 buildStatusRegisterValue(int regOffset) const; // 新增状态寄存器值构建函数
    
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
    quint16 toProtocolAddress(quint16 uiAddress) const;
};

} // namespace Nodes