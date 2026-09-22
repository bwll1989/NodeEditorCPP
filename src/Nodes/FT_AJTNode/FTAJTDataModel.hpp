#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <array>
#include <memory>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>

#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "FTAJTInterface.hpp"
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
 * @brief FT-AJT 六路调光控制器（TCP Client）
 *
 * 发送：F7 | 0B | SRC | DST | 02 | 13 | CH1..CH6 | CSUM | FD
 * 全开：立即发送当前值并响应变化；全关：强制下发全 0，之后仅更新界面/输出不下发。
 * 发送 2ms 防抖；界面显示约 16ms 合并刷新，减轻高频输入压力。
 */
class FTAJTDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(int srcAddr READ getSrcAddr WRITE setSrcAddr NOTIFY srcAddrChanged)
    Q_PROPERTY(int dstAddr READ getDstAddr WRITE setDstAddr NOTIFY dstAddrChanged)
    Q_PROPERTY(bool connected READ isConnected WRITE setConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool enable READ getEnable WRITE setEnable NOTIFY enableChanged)

public:
    static constexpr int kChannelCount = 6;
    static constexpr int kEnablePort = kChannelCount; // 输入端口：全开/全关

    FTAJTDataModel();
    ~FTAJTDataModel() override;

    QString getHost() const { return _host; }
    void setHost(const QString &host);

    int getPort() const { return _port; }
    void setPort(int port);

    int getSrcAddr() const { return _srcAddr; }
    void setSrcAddr(int addr);

    int getDstAddr() const { return _dstAddr; }
    void setDstAddr(int addr);

    bool isConnected() const { return _connected; }
    void setConnected(bool connected);

    bool getEnable() const { return _enable; }
    void setEnable(bool enable);

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
    void srcAddrChanged(int addr);
    void dstAddrChanged(int addr);
    void connectedChanged(bool connected);
    void enableChanged(bool enable);

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void flushPendingSend();
    void flushPendingUi();

private:
    QString _host = "127.0.0.1";
    int _port = 1001;
    int _srcAddr = 0x00;
    int _dstAddr = 0x35;
    bool _connected = false;
    bool _enable = false;
    bool _loading = false;
    bool _modelReady = false;

    FTAJTInterface *_interface = nullptr;
    TcpClient *_tcpClient = nullptr;
    QTimer *_sendDebounce = nullptr;
    QTimer *_uiDebounce = nullptr;

    std::array<int, kChannelCount> _levels{};
    std::array<std::shared_ptr<NodeDataTypes::VariableData>, kChannelCount> _outputData{};

    bool _sendPending = false;
    bool _sendAllPending = false;
    int _pendingChannel = -1;
    quint8 _uiDirtyMask = 0;

    void ensureTcpClient();
    void destroyTcpClient();
    void reconnect();
    void setChannelLevel(int index, int level, bool send = true);
    void scheduleUiUpdate(int index);
    void updateOutputPort(int index, int value);
    void scheduleSend(int channelIndex, bool sendAll);
    void sendDimFrame(bool sendAll, int channelIndex);
    void sendCurrentLevels(bool immediate);
    void sendRawLevels(const std::array<int, kChannelCount> &levels);
    void clearPendingSend();
    QByteArray buildDimFrame(const std::array<quint8, kChannelCount> &channels) const;
    static int clampLevel(int level);
};

} // namespace Nodes
