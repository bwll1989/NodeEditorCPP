#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <QList>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>
#include <QtQml/QJSValueIterator> 
#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "USR-IO808Interface.hpp"
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
 * @brief Modbus TCP主机节点数据模型
 * 
 * 实现基于Modbus TCP协议的主机节点，支持：
 * - DO部分：8个线圈读写，寄存器地址范围0x0000~0x0007
 * - DI部分：8个离散量输入只读，寄存器地址范围0x0020~0x0027
 * - 命令串行队列：上一帧响应后再发下一帧
 * - DO 以 10Hz 轮询同步：先读设备 DO，与 _outputStates 不一致时才写 8 路 DO
 * - _outputStates 仅由用户/输入端口维护，读 DO 不反向覆盖缓存
 */
class USR_IO808DataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)

public:
    USR_IO808DataModel();
    ~USR_IO808DataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString& host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    bool isConnected() const { return _connected; }
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
    void connectedChanged(bool connected);

private:
    enum class ModbusCommandKind {
        ReadDiscreteInputs,
        ReadCoils,
        WriteMultipleCoils,
    };

    struct ModbusCommand {
        QByteArray payload;
        quint16 transactionId = 0;
        ModbusCommandKind kind = ModbusCommandKind::ReadDiscreteInputs;
    };

private slots:
    void onGlobalEvent(const GlobalEvent& ev);
    void recMsg(QByteArray msg, QString ip, int port);
    void readAllInputs();
    void readAllOutputs();
    void setOutput(int index, bool state);
    void readAllData();

private:
    USR_IO808Interface *_interface;
    TcpClient *_tcpClient;
    QTimer *_readTimer;
    QTimer *_responseTimer;

    QList<ModbusCommand> _commandQueue;
    bool _awaitingResponse = false;
    ModbusCommandKind _activeKind = ModbusCommandKind::ReadDiscreteInputs;
    quint16 _activeTransactionId = 0;

    bool _inputStates[8];
    bool _outputStates[8];

    quint16 _transactionId;

    QString _host = "127.0.0.1";
    int _port = 502;
    int _serverId = 1;
    bool _connected = false;

    std::shared_ptr<NodeDataTypes::VariableData> _outputData[8];

    void processModbusResponse(const QByteArray &response);
    void handleSingleFrame(const QByteArray &frame);
    void finishActiveCommand(bool success);

    QByteArray generateReadCoilsCommand(quint16 startAddress, quint16 quantity);
    QByteArray generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity);
    QByteArray generateWriteMultipleCoilsCommand(quint16 startAddress, const QVector<bool> &values, quint16 quantity);

    quint16 transactionIdFromCommand(const QByteArray &command) const;

    void updateOutputData(int port, bool value);
    void enqueueModbusCommand(const QByteArray &command, ModbusCommandKind kind);
    void pumpCommandQueue();
    void clearCommandQueue();

    void writeAllOutputs();
    void onResponseTimeout();
};

} // namespace Nodes
