#include "showStoreGBxDataModel.hpp"
#include "../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJSEngine>
#include <QDateTime>
#include <QCoreApplication>

namespace Nodes {

showStoreGBxDataModel::showStoreGBxDataModel()
    : _interface(new showStoreGBxInterface())
    , _tcpClient(new TcpClient(m_host, m_port))
    , logData(std::shared_ptr<NodeDataTypes::VariableData>())
{
    InPortCount =4;
    OutPortCount=1;
    PortEditable=false;
    CaptionVisible=true;
    Caption="showStoreGBx";
    WidgetEmbeddable= false;
    Resizable=false;

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "host";
        b.control = _interface->_hostEdit;
        AbstractDelegateModel::registerExternalBinding("/host", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "port";
        AbstractDelegateModel::registerExternalBinding("/port", this, b);
    }

    for (int i = 0; i < 4; ++i)
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QString("channel%1").arg(i + 1);
        b.control = _interface->_playButtons[i];
        AbstractDelegateModel::registerExternalBinding("/channel" + QString::number(i + 1), this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "stop";
        b.control = _interface->Stop;
        AbstractDelegateModel::registerExternalBinding("/stop", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = _interface->status;
        AbstractDelegateModel::registerExternalBinding("/connected", this, b);
    }

    m_host = _interface->getHost();
    m_port = _interface->getPort();
    connectToServer();

    // 连接信号和槽
    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("ascii").toString());
        }
    });
    
    // 连接TCP客户端连接状态信号
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        setConnected(isReady);
    });
    
    // 连接输出触发信号（button 非 checkable，clicked(bool) 的 bool 值不可靠；这里忽略 state）
    connect(_interface, &showStoreGBxInterface::outputChanged, this, [this](int index, bool) {
        if (index >= 0 && index < 4) {
            triggerChannel(index);
        }
    });
    
    // 连接主机变更信号（统一走属性 setter）
    connect(_interface, &showStoreGBxInterface::hostChanged, this, &showStoreGBxDataModel::setHost);

    connect(_interface->Stop, &QPushButton::clicked, this, [this](){
        setStop(true);
    });
}

showStoreGBxDataModel::~showStoreGBxDataModel()
{
    if (_tcpClient) {
        _tcpClient->disconnectFromServer();
        delete _tcpClient;
        _tcpClient = nullptr;
    }
    if (_interface) {
        delete _interface;
        _interface = nullptr;
    }
}


NodeDataType showStoreGBxDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In) {

        return NodeDataTypes::VariableData().type();

    } else if (portType == PortType::Out) {
        return NodeDataTypes::VariableData().type();
    }

    return NodeDataType();
}

std::shared_ptr<NodeData> showStoreGBxDataModel::outData(PortIndex port)
{
    return logData;
}

void showStoreGBxDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!varData) {
        return;
    }
    if (!varData->value().toBool()) {
        return;
    }

    if (port >= 0 && port < 4) {
        triggerChannel(port);
    }
}

QString showStoreGBxDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        return "SHOW "+QString::number(portIndex);
    case PortType::Out:

        return "LOG " +QString::number(portIndex);
    default:
        break;
    }
    return "";
}
QJsonObject showStoreGBxDataModel::save() const
{
    QJsonObject modelJson  = NodeDelegateModel::save();
    QJsonObject values;
    values["host"] = m_host;
    values["port"] = m_port;
    modelJson["values"] = values;
    return modelJson;
}

void showStoreGBxDataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject())
    {
        QJsonObject values = v.toObject();
        if (values.contains("host")) {
            setHost(values["host"].toString());
        }
        if (values.contains("port")) {
            setPort(values["port"].toInt());
        }
        connectToServer();
    }

    NodeDelegateModel::load(p);
}

void showStoreGBxDataModel::recMsg(QString msg)
{
    // 处理接收到的Modbus响应
    processResponse(msg);
}
void showStoreGBxDataModel::processResponse(const QString &msg)
{
    
    // 检查logData是否为空,为空则创建新的数据
    if(!logData) {
        logData = std::make_shared<NodeDataTypes::VariableData>();
    }
    logData->insert("default", msg);
    emit dataUpdated(0);
}
void showStoreGBxDataModel::triggerChannel(int index)
{
    if (!_tcpClient) return;
    if (index < 0 || index >= 4) return;

    const QString addr = QString("/channel%1").arg(index + 1);
    AbstractDelegateModel::stateFeedBack(addr, true);
    _tcpClient->sendMessage(QString("ST%1\r").arg(index + 1, 2, 10, QChar('0')), 1);
    AbstractDelegateModel::stateFeedBack(addr, false);
}

void showStoreGBxDataModel::stopPlay() {
    if (!_tcpClient) return;
    _tcpClient->sendMessage("SP\r", 1);
}


void showStoreGBxDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    auto bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/channel1"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/channel2"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/channel3"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/channel4"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

void showStoreGBxDataModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) return;
    const QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);

    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    } else if (localPath == "stop") {
        if (!ev.payload.isValid() || ev.payload.toBool()) {
            setStop(true);
        }
    } else if (localPath == "channel1") {
        if (!ev.payload.isValid() || ev.payload.toBool()) setChannel1(true);
    } else if (localPath == "channel2") {
        if (!ev.payload.isValid() || ev.payload.toBool()) setChannel2(true);
    } else if (localPath == "channel3") {
        if (!ev.payload.isValid() || ev.payload.toBool()) setChannel3(true);
    } else if (localPath == "channel4") {
        if (!ev.payload.isValid() || ev.payload.toBool()) setChannel4(true);
    }
}

void showStoreGBxDataModel::connectToServer()
{
    if (!_tcpClient) return;
    if (m_host.isEmpty()) return;
    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(m_host, m_port);
}

void showStoreGBxDataModel::setHost(const QString& host)
{
    if (m_host == host) return;
    m_host = host;

    if (_interface && _interface->_hostEdit && _interface->_hostEdit->text() != host) {
        QSignalBlocker b(_interface->_hostEdit);
        _interface->_hostEdit->setText(host);
    }

    Q_EMIT hostChanged(m_host);
    connectToServer();
}

void showStoreGBxDataModel::setPort(int port)
{
    if (m_port == port) return;
    m_port = port;
    Q_EMIT portChanged(m_port);
    connectToServer();
}

void showStoreGBxDataModel::setConnected(bool connected)
{
    if (m_connected == connected) return;
    m_connected = connected;
    if (_interface) {
        _interface->setConnectionStatus(connected);
    }
    Q_EMIT connectedChanged(m_connected);
}

void showStoreGBxDataModel::setStop(bool v)
{
    if (!v) return;
    if (!m_stop) {
        m_stop = true;
        Q_EMIT stopChanged(m_stop);
        stopPlay();
        m_stop = false;
        Q_EMIT stopChanged(m_stop);
    } else {
        stopPlay();
    }
}

void showStoreGBxDataModel::setChannel1(bool v)
{
    if (!v) return;
    if (!m_channel1) {
        m_channel1 = true;
        Q_EMIT channel1Changed(m_channel1);
        triggerChannel(0);
        m_channel1 = false;
        Q_EMIT channel1Changed(m_channel1);
    } else {
        triggerChannel(0);
    }
}

void showStoreGBxDataModel::setChannel2(bool v)
{
    if (!v) return;
    if (!m_channel2) {
        m_channel2 = true;
        Q_EMIT channel2Changed(m_channel2);
        triggerChannel(1);
        m_channel2 = false;
        Q_EMIT channel2Changed(m_channel2);
    } else {
        triggerChannel(1);
    }
}

void showStoreGBxDataModel::setChannel3(bool v)
{
    if (!v) return;
    if (!m_channel3) {
        m_channel3 = true;
        Q_EMIT channel3Changed(m_channel3);
        triggerChannel(2);
        m_channel3 = false;
        Q_EMIT channel3Changed(m_channel3);
    } else {
        triggerChannel(2);
    }
}

void showStoreGBxDataModel::setChannel4(bool v)
{
    if (!v) return;
    if (!m_channel4) {
        m_channel4 = true;
        Q_EMIT channel4Changed(m_channel4);
        triggerChannel(3);
        m_channel4 = false;
        Q_EMIT channel4Changed(m_channel4);
    } else {
        triggerChannel(3);
    }
}

} // namespace Nodes