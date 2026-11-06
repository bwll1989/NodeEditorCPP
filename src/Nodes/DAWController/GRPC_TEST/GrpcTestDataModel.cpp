#include "GrpcTestDataModel.hpp"

#include <QtCore/QDateTime>

#include "control.pb.h"

namespace Nodes {

namespace {

// SPEED 输入的可选键（default 数值键之外）
constexpr const char *kRpsKeys[] = { "rps", "revolutionsPerSecond", "revolutions_per_second" };

// 从若干「子表键」中提取 {x,y,z}
bool readSubVec3(const QVariantMap &map, const std::initializer_list<const char *> &keys,
                 float &x, float &y, float &z)
{
    for (const char *key : keys) {
        const QVariant v = map.value(QLatin1String(key));
        if (v.canConvert<QVariantMap>()) {
            const QVariantMap m = v.toMap();
            if (m.contains("x") || m.contains("y") || m.contains("z")) {
                x = float(m.value("x").toDouble());
                y = float(m.value("y").toDouble());
                z = float(m.value("z").toDouble());
                return true;
            }
        }
    }
    return false;
}

// 从顶层键组（如 rx/ry/rz 或 pitch/yaw/roll）提取
bool readFlatVec3(const QVariantMap &map, const std::initializer_list<const char *> &keys,
                  float &x, float &y, float &z)
{
    QStringList list;
    for (const char *key : keys) {
        list << QLatin1String(key);
    }
    for (const QString &key : list) {
        if (!map.contains(key)) {
            return false;
        }
    }
    x = float(map.value(list.at(0)).toDouble());
    y = float(map.value(list.at(1)).toDouble());
    z = float(map.value(list.at(2)).toDouble());
    return true;
}

// 从 default 列表前 3 项提取
bool readDefaultVec3(const QVariantMap &map, float &x, float &y, float &z)
{
    const QVariantList list = map.value(QStringLiteral("default")).toList();
    if (list.size() >= 3) {
        x = float(list.at(0).toDouble());
        y = float(list.at(1).toDouble());
        z = float(list.at(2).toDouble());
        return true;
    }
    return false;
}

} // namespace

GrpcTestDataModel::GrpcTestDataModel()
    : AbstractDelegateModel()
{
    InPortCount = 2;
    OutPortCount = 1;
    CaptionVisible = true;
    PortEditable = false;
    Caption = QStringLiteral("GRPC-TEST");
    WidgetEmbeddable = false;
    Resizable = false;

    m_stateOut = std::make_shared<VariableData>();

    m_channel = new GrpcChannel(this);
    connect(m_channel, &GrpcChannel::connectedChanged, this, &GrpcTestDataModel::onConnectedChanged);

    m_stateTimer = new QTimer(this);
    connect(m_stateTimer, &QTimer::timeout, this, &GrpcTestDataModel::pollState);

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "host";
        AbstractDelegateModel::registerExternalBinding("/host", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "port";
        AbstractDelegateModel::registerExternalBinding("/port", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "interval";
        AbstractDelegateModel::registerExternalBinding("/interval", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "getState";
        AbstractDelegateModel::registerExternalBinding("/getState", this, b);
    }

    m_channel->connectTo(m_host, quint16(m_port));
}

GrpcTestDataModel::~GrpcTestDataModel()
{
    if (m_channel) {
        m_channel->shutdown();
    }
}

void GrpcTestDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/interval"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/getState"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/speed"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/pose"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

QString GrpcTestDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        switch (portIndex) {
        case 0: return QStringLiteral("SPEED");
        case 1: return QStringLiteral("POSE");
        default: break;
        }
        break;
    case PortType::Out:
        if (portIndex == 0) {
            return QStringLiteral("STATE");
        }
        break;
    default:
        break;
    }
    return QString();
}

NodeDataType GrpcTestDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return VariableData().type();
}

std::shared_ptr<NodeData> GrpcTestDataModel::outData(PortIndex port)
{
    Q_UNUSED(port);
    return m_stateOut;
}

void GrpcTestDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<VariableData>(data);
    if (!varData) {
        return;
    }
    switch (port) {
    case 0: {
        float rps = float(varData->asNumber());
        const QVariantMap m = varData->asMap();
        for (const char *key : kRpsKeys) {
            if (m.contains(QLatin1String(key))) {
                rps = float(m.value(QLatin1String(key)).toDouble());
                break;
            }
        }
        applySpeed(rps);
        break;
    }
    case 1:
        applyPose(varData->asMap());
        break;
    default:
        break;
    }
}

QJsonObject GrpcTestDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson["host"] = m_host;
    modelJson["port"] = m_port;
    modelJson["interval"] = m_interval;
    return modelJson;
}

void GrpcTestDataModel::load(const QJsonObject &p)
{
    if (p.contains("host")) {
        setHost(p["host"].toString());
    }
    if (p.contains("port")) {
        setPort(p["port"].toInt());
    }
    if (p.contains("interval")) {
        setInterval(p["interval"].toDouble());
    }
}

void GrpcTestDataModel::setHost(const QString &host)
{
    if (m_host == host) {
        return;
    }
    m_host = host;
    emit hostChanged(m_host);
    m_channel->connectTo(m_host, quint16(m_port));
}

void GrpcTestDataModel::setPort(int port)
{
    if (m_port == port) {
        return;
    }
    m_port = port;
    emit portChanged(m_port);
    m_channel->connectTo(m_host, quint16(m_port));
}

void GrpcTestDataModel::setInterval(double interval)
{
    const double clamped = qBound(0.0, interval, 3600.0);
    if (qFuzzyCompare(m_interval, clamped)) {
        return;
    }
    m_interval = clamped;
    emit intervalChanged(m_interval);
    if (m_status && m_interval > 0.0) {
        m_stateTimer->start(int(m_interval * 1000.0));
        pollState();
    } else {
        m_stateTimer->stop();
    }
}

void GrpcTestDataModel::setGetState(bool trigger)
{
    if (!trigger || m_getState) {
        return;
    }
    m_getState = true;
    emit getStateChanged(true);
    pollState();
    m_getState = false;
    emit getStateChanged(false);
}

void GrpcTestDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }
    const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    } else if (localPath == "interval") {
        setInterval(ev.payload.toDouble());
    } else if (localPath == "getState") {
        if (ev.payload.toBool()) {
            setGetState(true);
        }
    } else if (localPath == "speed") {
        const QVariantMap m = ev.payload.toMap();
        if (m.contains(QStringLiteral("rps"))) {
            applySpeed(float(m.value(QStringLiteral("rps")).toDouble()));
        } else {
            applySpeed(float(ev.payload.toDouble()));
        }
    } else if (localPath == "pose") {
        applyPose(ev.payload.toMap());
    }
}

void GrpcTestDataModel::onConnectedChanged(bool connected)
{
    setStatus(connected);
    if (connected) {
        setLastError(QString());
        if (m_interval > 0.0) {
            m_stateTimer->start(int(m_interval * 1000.0));
            pollState();
        }
    } else {
        m_stateTimer->stop();
    }
}

void GrpcTestDataModel::pollState()
{
    if (!m_channel->isConnected()) {
        return;
    }
    m_channel->call(QStringLiteral("/control.v1.ControlService/GetState"),
                    QByteArray(), // GetStateRequest 为空消息
                    [this](int grpcStatus, const QString &message, const QByteArray &response) {
        if (grpcStatus != 0) {
            setLastError(QStringLiteral("GetState: [%1] %2").arg(grpcStatus).arg(message));
            return;
        }
        control::v1::GetStateResponse resp;
        if (!resp.ParseFromString(response)) {
            setLastError(QStringLiteral("GetState: response parse failed"));
            return;
        }
        setLastError(QString());

        QVariantMap state;
        state["rps"] = resp.state().revolutions_per_second();
        QVariantMap pos;
        pos["x"] = resp.state().camera_position().x();
        pos["y"] = resp.state().camera_position().y();
        pos["z"] = resp.state().camera_position().z();
        state["position"] = pos;
        QVariantMap euler;
        euler["x"] = resp.state().camera_euler_degrees().x();
        euler["y"] = resp.state().camera_euler_degrees().y();
        euler["z"] = resp.state().camera_euler_degrees().z();
        state["euler"] = euler;
        state["timestamp"] = QDateTime::currentMSecsSinceEpoch();

        if (state != m_lastState) {
            m_lastState = state;
            m_stateOut = std::make_shared<VariableData>(state);
            Q_EMIT dataUpdated(0);
        }
    });
}

void GrpcTestDataModel::applySpeed(float rps)
{
    if (!m_channel->isConnected()) {
        setLastError(QStringLiteral("SetCylinderSpeed: not connected"));
        return;
    }
    control::v1::SetCylinderSpeedRequest req;
    req.set_revolutions_per_second(rps);
    const std::string payload = req.SerializeAsString();
    m_channel->call(QStringLiteral("/control.v1.ControlService/SetCylinderSpeed"),
                    QByteArray(payload.data(), int(payload.size())),
                    [this](int grpcStatus, const QString &message, const QByteArray &) {
        if (grpcStatus != 0) {
            setLastError(QStringLiteral("SetCylinderSpeed: [%1] %2").arg(grpcStatus).arg(message));
        } else {
            setLastError(QString());
        }
    });
}

void GrpcTestDataModel::applyPose(const QVariantMap &map)
{
    if (map.isEmpty()) {
        return;
    }
    float px = 0.f, py = 0.f, pz = 0.f;
    float ex = 0.f, ey = 0.f, ez = 0.f;

    // 位置：position/pos 子表 → 顶层 x/y/z → default 列表前 3 项
    if (!readSubVec3(map, { "position", "pos" }, px, py, pz)) {
        if (!readFlatVec3(map, { "x", "y", "z" }, px, py, pz)) {
            readDefaultVec3(map, px, py, pz);
        }
    }
    // 欧拉角：euler* 子表 → rx/ry/rz → pitch/yaw/roll
    if (!readSubVec3(map, { "euler", "eulerDegrees", "euler_degrees" }, ex, ey, ez)) {
        if (!readFlatVec3(map, { "rx", "ry", "rz" }, ex, ey, ez)) {
            readFlatVec3(map, { "pitch", "yaw", "roll" }, ex, ey, ez);
        }
    }

    if (!m_channel->isConnected()) {
        setLastError(QStringLiteral("SetCameraPose: not connected"));
        return;
    }
    control::v1::SetCameraPoseRequest req;
    req.mutable_position()->set_x(px);
    req.mutable_position()->set_y(py);
    req.mutable_position()->set_z(pz);
    req.mutable_euler_degrees()->set_x(ex);
    req.mutable_euler_degrees()->set_y(ey);
    req.mutable_euler_degrees()->set_z(ez);
    const std::string payload = req.SerializeAsString();
    m_channel->call(QStringLiteral("/control.v1.ControlService/SetCameraPose"),
                    QByteArray(payload.data(), int(payload.size())),
                    [this](int grpcStatus, const QString &message, const QByteArray &) {
        if (grpcStatus != 0) {
            setLastError(QStringLiteral("SetCameraPose: [%1] %2").arg(grpcStatus).arg(message));
        } else {
            setLastError(QString());
        }
    });
}

void GrpcTestDataModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged(m_lastError);
    stateFeedBack(QStringLiteral("/lastError"), m_lastError);
}

void GrpcTestDataModel::setStatus(bool status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    emit statusChanged(m_status);
    stateFeedBack(QStringLiteral("/status"), m_status);
}

} // namespace Nodes
