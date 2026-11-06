// 函数级注释：GRPC-TEST 节点 —— 基于 control.proto 的 gRPC 客户端
// 参考 PeripProtoServerDataModel 的结构：外部绑定 / 全局事件订阅 / VariableData 端口
// - 输入 SPEED → 调用 SetCylinderSpeed（转速，转/秒）
// - 输入 POSE  → 调用 SetCameraPose（位置 + 欧拉角，Unity 坐标系）
// - 按 interval 轮询 GetState → 输出 STATE（变化时刷新）
#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVariantMap>

#include <QtNodes/NodeDelegateModel>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "NodeDataList.hpp"
#include "GrpcChannel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes {

class GrpcTestDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(double interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(bool status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    // 脉冲型命令属性：写 true 触发一次 GetState 后自动复位
    Q_PROPERTY(bool getState READ getState WRITE setGetState NOTIFY getStateChanged)

public:
    GrpcTestDataModel();
    ~GrpcTestDataModel() override;

    QString host() const { return m_host; }
    int port() const { return m_port; }
    double interval() const { return m_interval; }
    bool status() const { return m_status; }
    QString lastError() const { return m_lastError; }
    bool getState() const { return m_getState; }

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
    void setInterval(double interval);
    void setGetState(bool trigger);

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void intervalChanged(double interval);
    void statusChanged(bool status);
    void lastErrorChanged(const QString &lastError);
    void getStateChanged(bool getState);

private slots:
    void onGlobalEvent(const GlobalEvent &ev);
    void onConnectedChanged(bool connected);
    void pollState();

private:
    void applySpeed(float rps);
    void applyPose(const QVariantMap &map);
    void setLastError(const QString &message);
    void setStatus(bool status);

    GrpcChannel *m_channel = nullptr;
    QTimer *m_stateTimer = nullptr;

    QString m_host = QStringLiteral("127.0.0.1");
    int m_port = 50051;
    double m_interval = 2.0;
    bool m_status = false;
    QString m_lastError;
    bool m_getState = false;

    std::shared_ptr<VariableData> m_stateOut;
    QVariantMap m_lastState;
};

} // namespace Nodes
