#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "FTControlBoxInterface.hpp"
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
 * @brief FT-ControlBox 控制盒节点
 *
 * 通过 TCP 接收控制盒上抛的按钮事件：
 * - 薄膜按钮：{485地址}$BB^{索引}，索引 1~6
 * - 遥控器按钮：{485地址}$YY^{索引}，索引 1~8
 */
class FTControlBoxDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int addr485 READ getAddr485 WRITE setAddr485 NOTIFY addr485Changed)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)

public:
    static constexpr int kFilmButtonCount = FTControlBoxInterface::kFilmButtonCount;
    static constexpr int kRemoteButtonCount = FTControlBoxInterface::kRemoteButtonCount;
    static constexpr int kTotalButtonCount = kFilmButtonCount + kRemoteButtonCount;

    FTControlBoxDataModel();
    ~FTControlBoxDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString& host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getAddr485() const { return _addr485; }
    void setAddr485(int addr485);

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
    void addr485Changed(int addr485);
    void connectedChanged(bool connected);

private slots:
    void onGlobalEvent(const GlobalEvent& ev);
    void recMsg(const QVariantMap& dataMap);

private:
    void connectToServer();
    void processMessage(const QString& msg);
    void triggerFilmButton(int index);
    void triggerRemoteButton(int index);
    void pulseOutput(int port);

    FTControlBoxInterface *_interface;
    TcpClient *_tcpClient;

    QString _host = "127.0.0.1";
    int _port = 2001;
    int _addr485 = 1;
    bool _connected = false;

    std::shared_ptr<NodeDataTypes::VariableData> _outputData[kTotalButtonCount];
};

} // namespace Nodes
