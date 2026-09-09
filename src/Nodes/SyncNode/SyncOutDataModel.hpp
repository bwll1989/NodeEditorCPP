#pragma once

/**
 * @file SyncOutDataModel.hpp
 * @brief Sync Out：TCP 服务端，向多个 Sync In 客户端广播 Variable。
 */

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <memory>
#include <unordered_map>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "SyncOutInterface.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

namespace Nodes {

class SyncOutDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool push READ push WRITE setPush NOTIFY pushChanged)

public:
    SyncOutDataModel();
    ~SyncOutDataModel() override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;
    QWidget *embeddedWidget() override;

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;

    int port() const { return _port; }
    void setPort(int port);

    bool active() const { return _active; }
    void setActive(bool active);

    bool connected() const { return _connected; }

    bool push() const { return _push; }
    void setPush(bool v);

signals:
    void portChanged(int port);
    void activeChanged(bool active);
    void connectedChanged(bool connected);
    void pushChanged(bool v);

protected:
    void afterModelReady() override;

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev);
    void onNewConnection();
    void onClientDisconnected();
    void onClientReadyRead();
    void onHeartbeatTick();
    void onClientWatchTick();

private:
    void setupUi();
    void refreshStatus();
    void setConnected(bool connected);
    void startServer();
    void stopServer();
    void writeLine(const QByteArray &json);
    void announceSchema();
    void sendPort(PortIndex index, const std::shared_ptr<NodeDataTypes::VariableData> &data);
    void pushAll();
    void pushToClient(QTcpSocket *sock);
    void touchClient(QTcpSocket *sock);
    void dropClient(QTcpSocket *sock);

    SyncOutInterface *_widget = nullptr;
    int _port = 9100;
    bool _active = true;
    bool _connected = false;
    bool _listening = false;
    bool _push = false;
    int _lastAnnouncedN = -1;

    std::unordered_map<PortIndex, std::shared_ptr<NodeDataTypes::VariableData>> _cache;
    std::unique_ptr<QTcpServer> _server;
    QList<QPointer<QTcpSocket>> _clients;
    QHash<QTcpSocket *, qint64> _clientLastRxMs;
    QHash<QTcpSocket *, QByteArray> _clientRxBuffers;
    QTimer _schemaTimer;
    QTimer _heartbeatTimer;
    QTimer _clientWatchTimer;
};

} // namespace Nodes
