#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <array>
#include <memory>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "../../Common/Devices/TcpServer/TcpServer.h"
#include "NodeDataList.hpp"
#include "FTC040AInterface.hpp"
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
 * @brief FT-C040A Modbus TCP 从机
 *
 * - 监听 TCP，响应主机轮询/写线圈
 * - 16 路线圈，PDU 地址 0001～0016（数值 1～16）
 * - 与界面勾选、触摸屏外部控制、输出端口双向同步
 */
class FTC040ADataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int serverId READ getServerId WRITE setServerId NOTIFY serverIdChanged)
    Q_PROPERTY(bool listening READ isListening WRITE setListening NOTIFY listeningChanged)

public:
    static constexpr int kCoilCount = 16;
    static constexpr quint16 kCoilBaseAddress = 1; // 地址 0001～0016

    static QString coilAddressLabel(int address);

    FTC040ADataModel();
    ~FTC040ADataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString &host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getServerId() const { return _serverId; }
    void setServerId(int serverId);

    bool isListening() const { return _listening; }
    void setListening(bool listening);

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

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void onServerReady(bool ready);
    void onRecMsg(const QVariantMap &msg);

private:
    FTC040AInterface *_interface = nullptr;

    QString _host = "0.0.0.0";
    int _port = 50001;
    int _serverId = 1;
    bool _listening = false;

    // 必须在 _host/_port 之后声明，避免构造时用未初始化参数创建
    TcpServer *_tcpServer = nullptr;

    std::array<bool, kCoilCount> _coils{};
    std::array<std::shared_ptr<NodeDataTypes::VariableData>, kCoilCount> _outputData{};
    QHash<QString, QByteArray> _rxBuffers;

    void restartServer();
    void setCoil(int index, bool state, bool fromModbus = false);
    void updateOutputPort(int index, bool value);

    void processClientData(const QString &peerHost, const QByteArray &chunk);
    void handleRequestFrame(const QString &peerHost, const QByteArray &frame);
    QByteArray buildException(quint16 transactionId, quint8 unitId, quint8 functionCode, quint8 exceptionCode) const;
    QByteArray buildReadCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const;
    QByteArray buildWriteSingleCoilResponse(quint16 transactionId, quint8 unitId, quint16 address, quint16 value) const;
    QByteArray buildWriteMultipleCoilsResponse(quint16 transactionId, quint8 unitId, quint16 startAddress, quint16 quantity) const;

    static bool isCoilAddressValid(quint16 address);
    static int addressToIndex(quint16 address);
};

} // namespace Nodes
