#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <memory>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "ModBusMasterDiscreteInputInterface.hpp"
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
 * @brief 通用 Modbus TCP 主机：可配置离散输入数量与起始地址
 *
 * - 仅输出端口：回读 DI 状态
 * - FC02 批量读离散输入
 * - 命令串行队列 + 定时轮询
 */
class ModBusMasterDiscreteInputDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)
    Q_PROPERTY(int inputCount READ getInputCount WRITE setInputCount NOTIFY inputCountChanged)
    Q_PROPERTY(int addressBase READ getAddressBase WRITE setAddressBase NOTIFY addressBaseChanged)
    Q_PROPERTY(int inputStartAddress READ getInputStartAddress WRITE setInputStartAddress NOTIFY inputStartAddressChanged)

public:
    static constexpr int kMinInputCount = ModBusMasterDiscreteInputInterface::kMinInputCount;
    static constexpr int kMaxInputCount = ModBusMasterDiscreteInputInterface::kMaxInputCount;
    static constexpr int kDefaultInputCount = 8;

    ModBusMasterDiscreteInputDataModel();
    ~ModBusMasterDiscreteInputDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString &host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    bool isConnected() const { return _connected; }
    void setConnected(bool connected);

    int getInputCount() const { return _inputCount; }
    void setInputCount(int count);

    int getAddressBase() const { return _addressBase; }
    void setAddressBase(int base);

    int getInputStartAddress() const { return static_cast<int>(_inputStartAddress) + _addressBase; }
    void setInputStartAddress(int address);

    void afterModelReady() override;

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;
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
    void inputCountChanged(int count);
    void addressBaseChanged(int base);
    void inputStartAddressChanged(int address);

private:
    struct ModbusCommand {
        QByteArray payload;
        quint16 transactionId = 0;
    };

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void recMsg(QByteArray msg, QString ip, int port);
    void onResponseTimeout();

private:
    ModBusMasterDiscreteInputInterface *_interface = nullptr;
    TcpClient *_tcpClient = nullptr;
    QTimer *_readTimer = nullptr;
    QTimer *_responseTimer = nullptr;

    QList<ModbusCommand> _commandQueue;
    bool _awaitingResponse = false;
    quint16 _activeTransactionId = 0;

    int _inputCount = kDefaultInputCount;
    int _addressBase = 0;
    quint16 _inputStartAddress = 0; // PDU
    QVector<bool> _inputStates;
    QVector<std::shared_ptr<NodeDataTypes::VariableData>> _outputData;

    quint16 _transactionId = 0;

    QString _host = "127.0.0.1";
    int _port = 502;
    int _serverId = 1;
    bool _connected = false;

    void reconfigureInputs(int count, quint16 pduStart, bool notifyPorts);
    int displayAddress(int index) const;
    void resizeInputState(int count);
    void rebuildInputUiBindings();

    void readAllInputs();
    void updateOutputData(int port, bool value);

    void processModbusResponse(const QByteArray &response);
    void handleSingleFrame(const QByteArray &frame);
    void finishActiveCommand(bool success);

    QByteArray generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity);

    quint16 transactionIdFromCommand(const QByteArray &command) const;
    void enqueueModbusCommand(const QByteArray &command);
    void pumpCommandQueue();
    void clearCommandQueue();
};

} // namespace Nodes
