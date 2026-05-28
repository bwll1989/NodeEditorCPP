#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>
#include <QtQml/QJSValueIterator> 
#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "NodeDataList.hpp"
#include "showStoreGBxInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

#include <QSignalBlocker>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

namespace Nodes {

class showStoreGBxDataModel : public AbstractDelegateModel
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool connected READ connected WRITE setConnected NOTIFY connectedChanged)

    Q_PROPERTY(bool stop READ stop WRITE setStop NOTIFY stopChanged)
    Q_PROPERTY(bool channel1 READ channel1 WRITE setChannel1 NOTIFY channel1Changed)
    Q_PROPERTY(bool channel2 READ channel2 WRITE setChannel2 NOTIFY channel2Changed)
    Q_PROPERTY(bool channel3 READ channel3 WRITE setChannel3 NOTIFY channel3Changed)
    Q_PROPERTY(bool channel4 READ channel4 WRITE setChannel4 NOTIFY channel4Changed)

public:
    /**
     * @brief 构造函数：初始化 showStoreGBx 节点并完成外部控制绑定
     */
    showStoreGBxDataModel();

    /**
     * @brief 析构函数：释放 TCP 客户端与界面资源
     */
    ~showStoreGBxDataModel() override;

public:
    QJsonObject save() const override;
    void load(QJsonObject const &p) override;

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;

    QWidget *embeddedWidget() override { return _interface; }

    /**
     * @brief 节点就绪后订阅全局事件总线，接收外部控制命令
     */
    void afterModelReady() override;

public:
    /**
     * @brief 获取主机地址
     */
    QString host() const { return m_host; }

    /**
     * @brief 设置主机地址并重连
     */
    void setHost(const QString& host);

    /**
     * @brief 获取端口
     */
    int port() const { return m_port; }

    /**
     * @brief 设置端口并重连
     */
    void setPort(int port);

    /**
     * @brief 获取连接状态
     */
    bool connected() const { return m_connected; }

    /**
     * @brief 设置连接状态（由 TcpClient::isReady 驱动）
     */
    void setConnected(bool connected);

    /**
     * @brief 获取停止触发（脉冲）
     */
    bool stop() const { return m_stop; }

    /**
     * @brief 触发停止（置 true 触发一次，自动复位）
     */
    void setStop(bool v);

    /**
     * @brief 获取通道触发（脉冲）
     */
    bool channel1() const { return m_channel1; }
    bool channel2() const { return m_channel2; }
    bool channel3() const { return m_channel3; }
    bool channel4() const { return m_channel4; }

    /**
     * @brief 触发通道播放（置 true 触发一次，自动复位）
     */
    void setChannel1(bool v);
    void setChannel2(bool v);
    void setChannel3(bool v);
    void setChannel4(bool v);

signals:
    void hostChanged(const QString& host);
    void portChanged(int port);
    void connectedChanged(bool connected);

    void stopChanged(bool v);
    void channel1Changed(bool v);
    void channel2Changed(bool v);
    void channel3Changed(bool v);
    void channel4Changed(bool v);

private slots:
    /**
     * @brief 接收 TCP 文本消息
     */
    void recMsg(QString msg);

    /**
     * @brief 处理全局事件总线命令
     */
    void onGlobalEvent(const GlobalEvent& ev);

private:
    void processResponse(const QString &response);

    void triggerChannel(int index);
    void stopPlay();
    void connectToServer();

private:
    QString m_host = "127.0.0.1";
    int m_port = 23;

    showStoreGBxInterface *_interface;
    TcpClient *_tcpClient;
    std::shared_ptr<NodeDataTypes::VariableData> logData;
    bool m_connected = false;

    bool m_stop = false;
    bool m_channel1 = false;
    bool m_channel2 = false;
    bool m_channel3 = false;
    bool m_channel4 = false;

};

} // namespace Nodes