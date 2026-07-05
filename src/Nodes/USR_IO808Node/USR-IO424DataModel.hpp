#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "USR-IO424Interface.hpp"
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
 * @brief USR-IO424 Modbus TCP 节点
 *
 * - DO：4 路线圈，地址 0x0000~0x0003（功能码 01/05/0F）
 * - DI：4 路离散输入，地址 0x0020~0x0023（功能码 02/03，本节点使用 02）
 */
class USR_IO424DataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)

public:
    static constexpr int kChannelCount = USR_IO424Interface::kChannelCount;
    static constexpr quint16 kDoAddressBase = 0x0000;
    static constexpr quint16 kDiAddressBase = 0x0020;

    USR_IO424DataModel();
    ~USR_IO424DataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString& host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    bool isConnected() const { return _connected; }
    void setConnected(bool connected);

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

private slots:
    void onGlobalEvent(const GlobalEvent& ev);
    void recMsg(QByteArray msg, QString ip, int port);
    void readAllInputs();
    void setOutput(int index, bool state);
    void syncCycle();

private:
    USR_IO424Interface *_interface;
    TcpClient *_tcpClient;
    QTimer *_syncTimer;

    bool _inputStates[kChannelCount];
    bool _outputStates[kChannelCount];

    quint16 _transactionId;

    QString _host = "127.0.0.1";
    int _port = 8080;
    int _serverId = 1;
    bool _connected = false;

    std::shared_ptr<NodeDataTypes::VariableData> _outputData[kChannelCount];

    void processModbusResponse(const QByteArray &response);
    QByteArray generateReadDiscreteInputsCommand(quint16 startAddress, quint16 quantity);
    QByteArray generateWriteMultipleCoilsCommand(quint16 startAddress, const QVector<bool> &values, quint16 quantity);
    void updateOutputData(int port, bool value);
    void sendModbusCommand(const QByteArray &command);

    void writeAllOutputs();
};

} // namespace Nodes
