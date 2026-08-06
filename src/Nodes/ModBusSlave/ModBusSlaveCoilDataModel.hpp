#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <memory>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "../../Common/Devices/TcpServer/TcpServer.h"
#include "NodeDataList.hpp"
#include "ModBusSlaveCoilInterface.hpp"
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
 * @brief 通用 Modbus TCP 从机（线圈）—— 纯被控
 *
 * - 监听 TCP，响应主站读/写线圈（FC01 / FC05 / FC0F）
 * - 状态仅由主站写入更新；界面勾选只读显示；输出端口/OSC 反馈同步给本图
 * - 本地界面与 OSC Command 不得改写线圈表（避免反向拉动主站读到的状态）
 */
class ModBusSlaveCoilDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(bool listening READ isListening WRITE setListening NOTIFY listeningChanged)
    Q_PROPERTY(int coilCount READ getCoilCount WRITE setCoilCount NOTIFY coilCountChanged)
    Q_PROPERTY(int addressBase READ getAddressBase WRITE setAddressBase NOTIFY addressBaseChanged)
    Q_PROPERTY(int coilStartAddress READ getCoilStartAddress WRITE setCoilStartAddress NOTIFY coilStartAddressChanged)

public:
    static constexpr int kMinCoilCount = ModBusSlaveCoilInterface::kMinCoilCount;
    static constexpr int kMaxCoilCount = ModBusSlaveCoilInterface::kMaxCoilCount;
    static constexpr int kDefaultCoilCount = 16;
    static constexpr quint16 kDefaultCoilStartAddress = 1;

    ModBusSlaveCoilDataModel();
    ~ModBusSlaveCoilDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString &host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    bool isListening() const { return _listening; }
    void setListening(bool listening);

    int getCoilCount() const { return _coilCount; }
    void setCoilCount(int count);

    int getAddressBase() const { return _addressBase; }
    void setAddressBase(int base);

    /** 界面/属性中的起始地址（随 0/1 基变化）；协议匹配使用 PDU = 该值 - addressBase */
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
    void listeningChanged(bool listening);
    void coilCountChanged(int count);
    void addressBaseChanged(int base);
    void coilStartAddressChanged(int address);

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void onServerReady(bool ready);
    void onRecMsg(const QVariantMap &msg);

private:
    ModBusSlaveCoilInterface *_interface = nullptr;

    QString _host = "0.0.0.0";
    int _port = 50001;
    int _serverId = 1;
    bool _listening = false;

    int _coilCount = kDefaultCoilCount;
    int _addressBase = 0; // 0=PDU，1=显示地址（协议匹配仍用 PDU）
    quint16 _coilStartAddress = kDefaultCoilStartAddress; // PDU

    // 必须在 _host/_port 之后声明，避免构造时用未初始化参数创建
    TcpServer *_tcpServer = nullptr;

    QVector<bool> _coils;
    QVector<std::shared_ptr<NodeDataTypes::VariableData>> _outputData;
    QHash<QString, QByteArray> _rxBuffers;

    void reconfigureCoils(int count, quint16 pduStart, bool notifyPorts);
    int displayAddress(int index) const;
    void resizeCoilState(int count);
    void rebuildCoilUiBindings();

    void restartServer();
    /** 仅由主站写请求调用：更新状态表、只读 UI、输出端口与 OSC 反馈 */
    void applyMasterCoilWrite(int index, bool state);
    void updateOutputPort(int index, bool value);

    void processClientData(const QString &peerHost, const QByteArray &chunk);
    void handleRequestFrame(const QString &peerHost, const QByteArray &frame);
    QByteArray buildException(quint16 transactionId, quint8 unitId, quint8 functionCode, quint8 exceptionCode) const;
    QByteArray buildReadCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const;
    QByteArray buildWriteSingleCoilResponse(quint16 transactionId, quint8 unitId, quint16 address, quint16 value) const;
    QByteArray buildWriteMultipleCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const;

    bool isCoilAddressValid(quint16 address) const;
    int addressToIndex(quint16 address) const;
};

} // namespace Nodes
