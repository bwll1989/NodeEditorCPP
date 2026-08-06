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
#include "ModBusMasterCoilControllerInterface.hpp"
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
 * @brief 通用 Modbus TCP 主机：可配置线圈数量与起始地址
 *
 * - 输入端口：写入期望线圈状态（无输出端口）
 * - FC01 批量读 / FC05 单线圈写（多路不一致时按指令间隔串行写）
 * - 连接后以节点期望为准同步；空闲时轮询读回
 */
class ModBusMasterCoilControllerDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)
    Q_PROPERTY(int coilCount READ getCoilCount WRITE setCoilCount NOTIFY coilCountChanged)
    Q_PROPERTY(int addressBase READ getAddressBase WRITE setAddressBase NOTIFY addressBaseChanged)
    Q_PROPERTY(int coilStartAddress READ getCoilStartAddress WRITE setCoilStartAddress NOTIFY coilStartAddressChanged)

public:
    static constexpr int kMinCoilCount = ModBusMasterCoilControllerInterface::kMinCoilCount;
    static constexpr int kMaxCoilCount = ModBusMasterCoilControllerInterface::kMaxCoilCount;
    static constexpr int kDefaultCoilCount = 8;

    ModBusMasterCoilControllerDataModel();
    ~ModBusMasterCoilControllerDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString &host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    bool isConnected() const { return _connected; }
    void setConnected(bool connected);

    int getCoilCount() const { return _coilCount; }
    void setCoilCount(int count);

    int getAddressBase() const { return _addressBase; }
    void setAddressBase(int base);

    /** 界面/属性中的起始地址（随 0/1 基变化）；协议层使用 PDU = 该值 - addressBase */
    int getCoilStartAddress() const { return static_cast<int>(_coilStartAddress) + _addressBase; }
    void setCoilStartAddress(int address);

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
    void coilCountChanged(int count);
    void addressBaseChanged(int base);
    void coilStartAddressChanged(int address);

private:
    enum class ModbusCommandKind {
        ReadCoils,
        WriteSingleCoil,
    };

    struct ModbusCommand {
        QByteArray payload;
        quint16 transactionId = 0;
        ModbusCommandKind kind = ModbusCommandKind::ReadCoils;
        int coilIndex = -1; // WriteSingleCoil 时有效
    };

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void recMsg(QByteArray msg, QString ip, int port);
    void onResponseTimeout();
    void onCommandGapTimeout();

private:
    ModBusMasterCoilControllerInterface *_interface = nullptr;
    TcpClient *_tcpClient = nullptr;
    QTimer *_readTimer = nullptr;
    QTimer *_responseTimer = nullptr;
    QTimer *_commandGapTimer = nullptr;

    QList<ModbusCommand> _commandQueue;
    bool _awaitingResponse = false;
    ModbusCommandKind _activeKind = ModbusCommandKind::ReadCoils;
    quint16 _activeTransactionId = 0;
    int _activeCoilIndex = -1;

    int _coilCount = kDefaultCoilCount;
    int _addressBase = 0; // 0=PDU，1=显示地址（命令中仍用 PDU）
    quint16 _coilStartAddress = 0; // 始终存 PDU（0 基）

    QVector<bool> _desiredStates;
    QVector<bool> _deviceStates;

    quint16 _transactionId = 0;

    QString _host = "127.0.0.1";
    int _port = 502;
    int _serverId = 1;
    bool _connected = false;
    bool _oscReady = false;

    void reconfigureCoils(int count, quint16 pduStart, bool notifyPorts);
    int displayAddress(int index) const;
    void resizeCoilState(int count);
    void rebuildCoilUiBindings();
    void subscribeCoilOsc();
    void unsubscribeCoilOsc();

    void setCoil(int index, bool state);
    void readAllCoils();
    void writeCoil(int index);
    void syncDesiredToDevice(bool onlyMismatch);

    void processModbusResponse(const QByteArray &response);
    void handleSingleFrame(const QByteArray &frame);
    void finishActiveCommand(bool success);
    void scheduleNextCommand();

    QByteArray generateReadCoilsCommand(quint16 startAddress, quint16 quantity);
    QByteArray generateWriteSingleCoilCommand(quint16 address, bool value);

    quint16 transactionIdFromCommand(const QByteArray &command) const;
    void enqueueModbusCommand(const QByteArray &command, ModbusCommandKind kind, int coilIndex = -1);
    void pumpCommandQueue();
    void clearCommandQueue();
    void dropPendingWrites();
};

} // namespace Nodes
