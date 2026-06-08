#include "FTControlBoxDataModel.hpp"

#include <QJsonObject>
#include <QSignalBlocker>

namespace Nodes {

FTControlBoxDataModel::FTControlBoxDataModel()
    : _interface(new FTControlBoxInterface())
    , _tcpClient(new TcpClient("127.0.0.1", 2001))
{
    InPortCount = 0;
    OutPortCount = kTotalButtonCount;
    PortEditable = false;
    CaptionVisible = true;
    Caption = "FT-ControlBox";
    WidgetEmbeddable = false;
    Resizable = false;

    for (int i = 0; i < kTotalButtonCount; ++i) {
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>(false);
    }

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "host";
        b.control = _interface->_hostEdit;
        AbstractDelegateModel::registerExternalBinding("/host", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "port";
        b.control = _interface->_portEdit;
        AbstractDelegateModel::registerExternalBinding("/port", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "addr485";
        b.control = _interface->_addr485Edit;
        AbstractDelegateModel::registerExternalBinding("/addr485", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/connected", this, b);
    }

    for (int i = 0; i < kFilmButtonCount; ++i) {
        NodeDelegateModel::ExternalBinding b;
        b.control = _interface->_filmButtons[i];
        AbstractDelegateModel::registerExternalBinding("/BB" + QString::number(i + 1), nullptr, b);
    }
    for (int i = 0; i < kRemoteButtonCount; ++i) {
        NodeDelegateModel::ExternalBinding b;
        b.control = _interface->_remoteButtons[i];
        AbstractDelegateModel::registerExternalBinding("/YY" + QString::number(i + 1), nullptr, b);
    }

    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });
    connect(_interface->_portEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setPort(val);
    });
    connect(_interface->_addr485Edit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setAddr485(val);
    });

    for (int i = 0; i < kFilmButtonCount; ++i) {
        connect(_interface->_filmButtons[i], &QPushButton::clicked, this, [this, i]() {
            triggerFilmButton(i);
        });
    }
    for (int i = 0; i < kRemoteButtonCount; ++i) {
        connect(_interface->_remoteButtons[i], &QPushButton::clicked, this, [this, i]() {
            triggerRemoteButton(i);
        });
    }

    connect(_tcpClient, &TcpClient::recMsg, this, &FTControlBoxDataModel::recMsg);
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        setConnected(isReady);
    });

    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_addr485Edit->setValue(_addr485);
}

FTControlBoxDataModel::~FTControlBoxDataModel()
{
    if (_tcpClient) {
        _tcpClient->disconnectFromServer();
        delete _tcpClient;
    }
}

void FTControlBoxDataModel::setHost(const QString& host)
{
    if (_host == host) return;
    _host = host;

    QSignalBlocker blocker(_interface->_hostEdit);
    _interface->_hostEdit->setText(_host);

    emit hostChanged(_host);
    connectToServer();
}

void FTControlBoxDataModel::setPort(int port)
{
    if (_port == port) return;
    _port = port;

    QSignalBlocker blocker(_interface->_portEdit);
    _interface->_portEdit->setValue(_port);

    emit portChanged(_port);
    connectToServer();
}

void FTControlBoxDataModel::setAddr485(int addr485)
{
    if (_addr485 == addr485) return;
    _addr485 = addr485;

    QSignalBlocker blocker(_interface->_addr485Edit);
    _interface->_addr485Edit->setValue(_addr485);

    emit addr485Changed(_addr485);
}

void FTControlBoxDataModel::setConnected(bool connected)
{
    if (_connected == connected) return;
    _connected = connected;
    emit connectedChanged(_connected);

    if (_interface) {
        _interface->setConnectionStatus(_connected);
    }
}

void FTControlBoxDataModel::connectToServer()
{
    if (!_tcpClient || _host.isEmpty()) return;
    _tcpClient->disconnectFromServer();
    _tcpClient->connectToServer(_host, _port);
}

void FTControlBoxDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(
        AbstractDelegateModel::makeFullOscAddress("/addr485"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

NodeDataType FTControlBoxDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> FTControlBoxDataModel::outData(PortIndex port)
{
    if (port >= 0 && port < kTotalButtonCount) {
        return _outputData[port];
    }
    return nullptr;
}

void FTControlBoxDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    Q_UNUSED(data);
    Q_UNUSED(port);
}

QString FTControlBoxDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType != PortType::Out) {
        return QString();
    }
    if (portIndex < kFilmButtonCount) {
        return QString("BB %1").arg(portIndex + 1);
    }
    if (portIndex < kTotalButtonCount) {
        return QString("YY %1").arg(portIndex - kFilmButtonCount + 1);
    }
    return QString();
}

QJsonObject FTControlBoxDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values["host"] = _host;
    values["port"] = _port;
    values["addr485"] = _addr485;
    modelJson["values"] = values;
    return modelJson;
}

void FTControlBoxDataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined() && v.isObject()) {
        QJsonObject values = v.toObject();
        if (values.contains("host")) setHost(values["host"].toString());
        if (values.contains("port")) setPort(values["port"].toInt());
        if (values.contains("addr485")) setAddr485(values["addr485"].toInt());
    }
}

ConnectionPolicy FTControlBoxDataModel::portConnectionPolicy(PortType portType, PortIndex index) const
{
    Q_UNUSED(index);
    switch (portType) {
    case PortType::Out:
        return ConnectionPolicy::Many;
    case PortType::In:
    case PortType::None:
        break;
    }
    return ConnectionPolicy::One;
}

void FTControlBoxDataModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    } else if (localPath == "addr485") {
        setAddr485(ev.payload.toInt());
    }
}

void FTControlBoxDataModel::recMsg(const QVariantMap& dataMap)
{
    const QString msg = dataMap.value("ascii").toString().trimmed();
    if (msg.isEmpty()) {
        return;
    }
    processMessage(msg);
}

void FTControlBoxDataModel::processMessage(const QString& msg)
{
    const QString addr = QString::number(_addr485);
    const QString filmPrefix = addr + "$BB^";
    const QString remotePrefix = addr + "$YY^";

    if (msg.startsWith(filmPrefix)) {
        bool ok = false;
        const int index = msg.mid(filmPrefix.length()).toInt(&ok);
        if (ok && index >= 1 && index <= kFilmButtonCount) {
            triggerFilmButton(index - 1);
        }
        return;
    }

    if (msg.startsWith(remotePrefix)) {
        bool ok = false;
        const int index = msg.mid(remotePrefix.length()).toInt(&ok);
        if (ok && index >= 1 && index <= kRemoteButtonCount) {
            triggerRemoteButton(index - 1);
        }
    }
}

void FTControlBoxDataModel::triggerFilmButton(int index)
{
    if (index < 0 || index >= kFilmButtonCount) {
        return;
    }
    _interface->flashFilmButton(index);
    pulseOutput(index);
    AbstractDelegateModel::stateFeedBack(QString("/BB%1").arg(index + 1), true);
    QTimer::singleShot(200, this, [this, index]() {
        AbstractDelegateModel::stateFeedBack(QString("/BB%1").arg(index + 1), false);
    });
}

void FTControlBoxDataModel::triggerRemoteButton(int index)
{
    if (index < 0 || index >= kRemoteButtonCount) {
        return;
    }
    _interface->flashRemoteButton(index);
    pulseOutput(kFilmButtonCount + index);
    AbstractDelegateModel::stateFeedBack(QString("/YY%1").arg(index + 1), true);
    QTimer::singleShot(200, this, [this, index]() {
        AbstractDelegateModel::stateFeedBack(QString("/YY%1").arg(index + 1), false);
    });
}

void FTControlBoxDataModel::pulseOutput(int port)
{
    if (port < 0 || port >= kTotalButtonCount) {
        return;
    }
    _outputData[port] = std::make_shared<NodeDataTypes::VariableData>(true);
    Q_EMIT dataUpdated(port);

    QTimer::singleShot(200, this, [this, port]() {
        _outputData[port] = std::make_shared<NodeDataTypes::VariableData>(false);
        Q_EMIT dataUpdated(port);
    });
}

} // namespace Nodes
