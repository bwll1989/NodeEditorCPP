#pragma once

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <memory>
#include <unordered_map>
#include <vector>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "AJTGatewayInterface.hpp"
#include "AJTProtocol.hpp"
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
 * @brief AJT Gateway：合并多个 AJT Dimming/Relay Node 的 DATA 下发
 *
 * - 同类型：500ms 去重，窗口内变化合并为一条
 * - 异类型：分开发送，任意两帧间隔 ≥500ms（厂家丢包约束）
 */
class AJTGatewayDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int srcAddr READ getSrcAddr WRITE setSrcAddr NOTIFY srcAddrChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)

public:
    AJTGatewayDataModel();
    ~AJTGatewayDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString &host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getSrcAddr() const { return _srcAddr; }
    void setSrcAddr(int addr);

    bool isConnected() const { return _connected; }
    void setConnected(bool connected);

    void afterModelReady() override;

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;
    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;
    QString portCaption(PortType portType, PortIndex portIndex) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;
    QWidget *embeddedWidget() override { return _interface; }

signals:
    void hostChanged(QString host);
    void portChanged(int port);
    void srcAddrChanged(int addr);
    void connectedChanged(bool connected);

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void onDimDebounceTimeout();
    void onRelayDebounceTimeout();

private:
    enum SendKindFlag : quint8 {
        SendNone = 0,
        SendDim = 1 << 0,
        SendRelay = 1 << 1,
        SendBoth = SendDim | SendRelay,
    };

    QString _host = QStringLiteral("127.0.0.1");
    int _port = 1001;
    int _srcAddr = 0x46;
    bool _connected = false;
    bool _loading = false;
    bool _modelReady = false;
    bool _pendingDim = false;
    bool _pendingRelay = false;

    AJTGatewayInterface *_interface = nullptr;
    TcpClient *_tcpClient = nullptr;
    QTimer *_dimDebounce = nullptr;
    QTimer *_relayDebounce = nullptr;
    QElapsedTimer _sinceLastSend;
    bool _hasSentOnce = false;

    std::unordered_map<int, AJTProtocol::DeviceState> _devicesByPort;
    std::shared_ptr<NodeDataTypes::VariableData> _statusOut;

    void ensureTcpClient();
    void destroyTcpClient();
    void reconnect();
    void scheduleSend(quint8 kinds);
    void clearPendingSend();
    void tryFlush(quint8 kind);
    void updateDeviceCountUi();
    void publishStatus();
    std::vector<AJTProtocol::DeviceState> collectDevices() const;
    static quint8 sendKindForDevice(const AJTProtocol::DeviceState &dev);
    int msUntilSendAllowed() const;
};

} // namespace Nodes
