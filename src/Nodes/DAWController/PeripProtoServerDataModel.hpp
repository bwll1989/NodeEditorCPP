#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

#include <QtNodes/NodeDelegateModel>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "NodeDataList.hpp"
#include "PeripProtoHandler.h"
#include "PeripProtoTcpServer.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes {

/**
 * @brief 基于 perip2s.proto 的 TCP Protobuf 服务端节点
 */
class PeripProtoServerDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    // Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)

public:
    PeripProtoServerDataModel();
    ~PeripProtoServerDataModel() override;

    QString host() const { return m_host; }
    int port() const { return m_port; }
    bool listening() const { return m_listening; }

    void afterModelReady() override;

    QString portCaption(PortType portType, PortIndex portIndex) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;

    QJsonObject save() const override;
    void load(const QJsonObject &p) override;

public slots:
    void setHost(const QString &host);
    void setPort(int port);

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void listeningChanged(bool listening);

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void onPosReceived(const QVariantMap &data);
    void onPointReceived(const QVariantMap &data);
    void onReplyRequested(const QString &peerHost, int32_t cmd, const QByteArray &body);
    void restartServer();

private:
    void setListening(bool listening);

    PeripProtoTcpServer *m_server = nullptr;
    PeripProtoHandler *m_handler = nullptr;

    QString m_host = QStringLiteral("0.0.0.0");
    int m_port = 9001;
    bool m_listening = false;

    std::shared_ptr<VariableData> m_posOut;
    std::shared_ptr<VariableData> m_pointOut;
};

} // namespace Nodes
