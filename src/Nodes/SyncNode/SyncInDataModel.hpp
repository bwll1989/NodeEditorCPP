#pragma once

/**
 * @file SyncInDataModel.hpp
 * @brief Sync In：TCP 客户端，连到 Sync Out 的 Listen Port；可一对多。
 */

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#include <memory>
#include <unordered_map>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "SyncInInterface.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

namespace Nodes {

class SyncInDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastSource READ lastSource NOTIFY lastSourceChanged)

public:
    SyncInDataModel();
    ~SyncInDataModel() override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;
    QWidget *embeddedWidget() override;

    QJsonObject save() const override;
    void load(QJsonObject const &p) override;

    QString host() const { return _host; }
    void setHost(const QString &host);

    int port() const { return _port; }
    void setPort(int port);

    /** 兼容旧 Hub / 旧工程：等同 port */
    int listenPort() const { return _port; }

    bool active() const { return _active; }
    void setActive(bool active);

    bool connected() const { return _connected; }
    QString lastSource() const { return _lastSource; }

    void applyPortCountFromPair(unsigned int newCount);

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void activeChanged(bool active);
    void connectedChanged(bool connected);
    void lastSourceChanged(const QString &source);

protected:
    void afterModelReady() override;

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev);
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onReconnectTick();
    void onHeartbeatWatchTick();

private:
    void setupUi();
    void applyPortCount(unsigned int newCount);
    void setConnected(bool connected);
    void setLastSource(const QString &source);
    void disconnectSocket();
    void connectSocket();
    void scheduleReconnect();
    void handleLine(const QByteArray &line);
    void touchRx();
    void replyHeartbeat();

    SyncInInterface *_widget = nullptr;
    QString _host = QStringLiteral("127.0.0.1");
    int _port = 9100;
    bool _active = true;
    bool _connected = false;
    QString _lastSource;
    QByteArray _rxBuffer;
    qint64 _lastRxMs = 0;

    std::unordered_map<PortIndex, std::shared_ptr<NodeDataTypes::VariableData>> _cache;
    std::unique_ptr<QTcpSocket> _socket;
    QTimer _reconnectTimer;
    QTimer _heartbeatWatchTimer;
};

} // namespace Nodes
