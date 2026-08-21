#include "PeripProtoServerDataModel.hpp"

namespace Nodes {

PeripProtoServerDataModel::PeripProtoServerDataModel()
{
    InPortCount = 0;
    OutPortCount = 2;
    CaptionVisible = true;
    PortEditable = false;
    Caption = QStringLiteral("FT-LocationProto");
    WidgetEmbeddable = false;
    Resizable = false;

    m_posOut = std::make_shared<VariableData>();
    m_pointOut = std::make_shared<VariableData>();

    m_server = new PeripProtoTcpServer(m_host, m_port, this);
    m_handler = new PeripProtoHandler(this);

    connect(m_server, &PeripProtoTcpServer::isReady, this, [this](bool ready) {
        setListening(ready);
    });
    connect(m_server, &PeripProtoTcpServer::packetReceived, m_handler, &PeripProtoHandler::handlePacket);
    connect(m_handler, &PeripProtoHandler::posReceived, this, &PeripProtoServerDataModel::onPosReceived);
    connect(m_handler, &PeripProtoHandler::pointReceived, this, &PeripProtoServerDataModel::onPointReceived);
    connect(m_handler, &PeripProtoHandler::replyRequested, this, &PeripProtoServerDataModel::onReplyRequested);

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
    // {
    //     NodeDelegateModel::ExternalBinding b;
    //     b.member = "listening";
    //     AbstractDelegateModel::registerExternalBinding("/listening", this, b);
    // }

    restartServer();
}

PeripProtoServerDataModel::~PeripProtoServerDataModel()
{
    if (m_server) {
        m_server->cleanup();
    }
}

void PeripProtoServerDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

QString PeripProtoServerDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType != PortType::Out) {
        return QString();
    }
    switch (portIndex) {
    case 0:
        return QStringLiteral("POS");
    case 1:
        return QStringLiteral("POINT");
    default:
        break;
    }
    return QString();
}

NodeDataType PeripProtoServerDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return VariableData().type();
}

std::shared_ptr<NodeData> PeripProtoServerDataModel::outData(PortIndex port)
{
    switch (port) {
    case 0:
        return m_posOut;
    case 1:
        return m_pointOut;
    default:
        break;
    }
    return nullptr;
}

void PeripProtoServerDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    Q_UNUSED(data);
    Q_UNUSED(port);
}

QJsonObject PeripProtoServerDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson["host"] = m_host;
    modelJson["port"] = m_port;
    return modelJson;
}

void PeripProtoServerDataModel::load(const QJsonObject &p)
{
    if (p.contains("host")) {
        setHost(p["host"].toString());
    }
    if (p.contains("port")) {
        setPort(p["port"].toInt());
    }
}

void PeripProtoServerDataModel::setHost(const QString &host)
{
    if (m_host == host) {
        return;
    }
    m_host = host;
    emit hostChanged(m_host);
    restartServer();
}

void PeripProtoServerDataModel::setPort(int port)
{
    if (m_port == port) {
        return;
    }
    m_port = port;
    emit portChanged(m_port);
    restartServer();
}

void PeripProtoServerDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    }
}

void PeripProtoServerDataModel::onPosReceived(const QVariantMap &data)
{
    QVariantMap map = data;
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    const QVariant posVar = map.value(QStringLiteral("pos"));
    if (posVar.canConvert<QVariantMap>()) {
        const QVariantMap pos = posVar.toMap();
        x = pos.value(QStringLiteral("x")).toFloat();
        y = pos.value(QStringLiteral("y")).toFloat();
        z = pos.value(QStringLiteral("z")).toFloat();
    }
    map.insert(QStringLiteral("default"), QVariantList{x, y, z});
    m_posOut = std::make_shared<VariableData>(map);
    Q_EMIT dataUpdated(0);
}

void PeripProtoServerDataModel::onPointReceived(const QVariantMap &data)
{
    m_pointOut = std::make_shared<VariableData>(data);
    Q_EMIT dataUpdated(1);
}

void PeripProtoServerDataModel::onReplyRequested(const QString &peerHost, int32_t cmd, const QByteArray &body)
{
    m_server->sendPacketToHost(peerHost, cmd, body);
}

void PeripProtoServerDataModel::restartServer()
{
    if (m_server) {
        m_server->setHost(m_host, m_port);
    }
}

void PeripProtoServerDataModel::setListening(bool listening)
{
    if (m_listening == listening) {
        return;
    }
    m_listening = listening;
    emit listeningChanged(m_listening);
}

} // namespace Nodes
