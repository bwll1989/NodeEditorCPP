#include "FTAJTDataModel.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>
#include <algorithm>

namespace Nodes {

namespace {
constexpr int kSendDebounceMs = 2;
constexpr int kUiDebounceMs = 16;
constexpr quint8 kTypeDim = 0x02;
constexpr quint8 kFuncControl = 0x13;
constexpr quint8 kNoControl = 0x01;
}

FTAJTDataModel::FTAJTDataModel()
    : _interface(new FTAJTInterface())
    , _sendDebounce(new QTimer(this))
    , _uiDebounce(new QTimer(this))
{
    InPortCount = kChannelCount + 1; // CH1～CH6 + 全开/全关
    OutPortCount = kChannelCount;
    PortEditable = false;
    CaptionVisible = true;
    Caption = "FT-AJT";
    WidgetEmbeddable = false;
    Resizable = false;

    for (int i = 0; i < kChannelCount; ++i) {
        _levels[i] = 0;
        _outputData[i] = std::make_shared<NodeDataTypes::VariableData>(0);
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
        b.member = "srcAddr";
        b.control = _interface->_srcAddrEdit;
        AbstractDelegateModel::registerExternalBinding("/srcAddr", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "dstAddr";
        b.control = _interface->_dstAddrEdit;
        AbstractDelegateModel::registerExternalBinding("/dstAddr", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/connected", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "enable";
        b.control = _interface->_enableButton;
        AbstractDelegateModel::registerExternalBinding("/enable", this, b);
    }

    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });
    connect(_interface->_portEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setPort(val);
    });
    connect(_interface->_srcAddrEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setSrcAddr(val);
    });
    connect(_interface->_dstAddrEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setDstAddr(val);
    });
    connect(_interface->_enableButton, &QPushButton::toggled, this, [this](bool checked) {
        setEnable(checked);
    });

    for (int i = 0; i < kChannelCount; ++i) {
        {
            NodeDelegateModel::ExternalBinding b;
            b.control = _interface->_channelEdits[i];
            AbstractDelegateModel::registerExternalBinding(QString("/CH%1").arg(i + 1), nullptr, b);
        }
        connect(_interface->_channelEdits[i], &IntDragValueWidget::valueChanged, this, [this, i](int val) {
            setChannelLevel(i, val);
        });
    }

    _sendDebounce->setSingleShot(true);
    _sendDebounce->setInterval(kSendDebounceMs);
    connect(_sendDebounce, &QTimer::timeout, this, &FTAJTDataModel::flushPendingSend);

    _uiDebounce->setSingleShot(true);
    _uiDebounce->setInterval(kUiDebounceMs);
    connect(_uiDebounce, &QTimer::timeout, this, &FTAJTDataModel::flushPendingUi);

    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_srcAddrEdit->setValue(_srcAddr);
    _interface->_dstAddrEdit->setValue(_dstAddr);
    _interface->setEnableChecked(_enable);
}

FTAJTDataModel::~FTAJTDataModel()
{
    _modelReady = false;
    _loading = true;
    clearPendingSend();
    if (_uiDebounce) {
        _uiDebounce->stop();
    }
    if (_sendDebounce) {
        _sendDebounce->stop();
    }
    _uiDirtyMask = 0;

    if (auto *bus = GlobalEventBus::instance()) {
        bus->unsubscribe(this);
    }
    destroyTcpClient();
}

void FTAJTDataModel::ensureTcpClient()
{
    if (_tcpClient || !_modelReady || _loading) {
        return;
    }

    _tcpClient = new TcpClient(_host, _port);
    connect(_tcpClient, &TcpClient::isReady, this, [this](bool ready) {
        if (!_modelReady || _loading || !_tcpClient) {
            return;
        }
        setConnected(ready);
    }, Qt::QueuedConnection);
}

void FTAJTDataModel::destroyTcpClient()
{
    if (!_tcpClient) {
        return;
    }
    // 先断开与本节点的信号，避免析构过程中回调
    QObject::disconnect(_tcpClient, nullptr, this, nullptr);
    // disconnect 会触发 worker onDisconnected 并再次启动重连定时器，必须随后 stopTimer
    _tcpClient->disconnectFromServer();
    _tcpClient->stopTimer();
    TcpClient *client = _tcpClient;
    _tcpClient = nullptr;
    _connected = false;
    delete client;
}

void FTAJTDataModel::setHost(const QString &host)
{
    if (_host == host) {
        return;
    }
    _host = host;
    if (_interface) {
        QSignalBlocker blocker(_interface->_hostEdit);
        _interface->_hostEdit->setText(_host);
    }
    emit hostChanged(_host);
    if (!_loading) {
        reconnect();
    }
}

void FTAJTDataModel::setPort(int port)
{
    if (_port == port) {
        return;
    }
    _port = port;
    if (_interface) {
        QSignalBlocker blocker(_interface->_portEdit);
        _interface->_portEdit->setValue(_port);
    }
    emit portChanged(_port);
    if (!_loading) {
        reconnect();
    }
}

void FTAJTDataModel::setSrcAddr(int addr)
{
    addr = std::clamp(addr, 0, 255);
    if (_srcAddr == addr) {
        return;
    }
    _srcAddr = addr;
    if (_interface) {
        QSignalBlocker blocker(_interface->_srcAddrEdit);
        _interface->_srcAddrEdit->setValue(_srcAddr);
    }
    emit srcAddrChanged(_srcAddr);
}

void FTAJTDataModel::setDstAddr(int addr)
{
    addr = std::clamp(addr, 0, 255);
    if (_dstAddr == addr) {
        return;
    }
    _dstAddr = addr;
    if (_interface) {
        QSignalBlocker blocker(_interface->_dstAddrEdit);
        _interface->_dstAddrEdit->setValue(_dstAddr);
    }
    emit dstAddrChanged(_dstAddr);
}

void FTAJTDataModel::setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }
    _connected = connected;
    if (_interface) {
        _interface->setConnectionStatus(_connected);
    }
    emit connectedChanged(_connected);
}

void FTAJTDataModel::setEnable(bool enable)
{
    if (_enable == enable) {
        return;
    }
    _enable = enable;
    if (_interface) {
        _interface->setEnableChecked(_enable);
    }
    emit enableChanged(_enable);

    if (_loading) {
        return;
    }

    if (_enable) {
        // 全开：立即下发当前界面值，并开始响应后续变化
        sendCurrentLevels(true);
    } else {
        // 全关：停止响应变化，并强制下发全 0（不改界面当前值）
        clearPendingSend();
        sendRawLevels({0, 0, 0, 0, 0, 0});
    }
}

void FTAJTDataModel::reconnect()
{
    if (_loading || !_modelReady) {
        return;
    }
    clearPendingSend();

    ensureTcpClient();
    if (!_tcpClient) {
        return;
    }
    _tcpClient->disconnectFromServer();
    _tcpClient->stopTimer();
    _tcpClient->connectToServer(_host, _port);
}

void FTAJTDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (_loading || ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == "host") {
        setHost(ev.payload.toString());
    } else if (localPath == "port") {
        setPort(ev.payload.toInt());
    } else if (localPath == "srcAddr") {
        setSrcAddr(ev.payload.toInt());
    } else if (localPath == "dstAddr") {
        setDstAddr(ev.payload.toInt());
    } else if (localPath == "enable") {
        setEnable(ev.payload.toBool());
    } else if (localPath.startsWith("CH")) {
        bool ok = false;
        const int ch = localPath.mid(2).toInt(&ok);
        if (ok && ch >= 1 && ch <= kChannelCount) {
            setChannelLevel(ch - 1, ev.payload.toInt());
        }
    }
}

void FTAJTDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    _modelReady = true;

    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/srcAddr"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/dstAddr"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
    for (int i = 0; i < kChannelCount; ++i) {
        bus->subscribe(makeFullOscAddress(QString("/CH%1").arg(i + 1)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

    QTimer::singleShot(0, this, [this]() {
        if (!_modelReady || _loading) {
            return;
        }
        ensureTcpClient();
        if (_enable) {
            sendCurrentLevels(true);
        }
    });
}

NodeDataType FTAJTDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType);
    Q_UNUSED(portIndex);
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> FTAJTDataModel::outData(PortIndex port)
{
    if (port >= 0 && port < kChannelCount) {
        return _outputData[port];
    }
    return nullptr;
}

void FTAJTDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    if (_loading) {
        return;
    }

    if (port == kEnablePort) {
        auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
        if (!varData) {
            setEnable(false);
            return;
        }
        setEnable(varData->asBool());
        return;
    }

    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!varData || port < 0 || port >= kChannelCount) {
        return;
    }
    setChannelLevel(port, varData->asInt());
}

QString FTAJTDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == PortType::In && portIndex == kEnablePort) {
        return QStringLiteral("ALL ON/OFF");
    }
    if (portIndex >= 0 && portIndex < kChannelCount) {
        return QString("CH%1").arg(portIndex + 1);
    }
    return {};
}

QJsonObject FTAJTDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values["host"] = _host;
    values["port"] = _port;
    values["srcAddr"] = _srcAddr;
    values["dstAddr"] = _dstAddr;
    values["enable"] = _enable;

    QJsonArray levels;
    for (int i = 0; i < kChannelCount; ++i) {
        levels.append(_levels[i]);
    }
    values["levels"] = levels;
    modelJson["values"] = values;
    return modelJson;
}

void FTAJTDataModel::load(QJsonObject const &p)
{
    const QJsonValue v = p["values"];
    if (!v.isObject()) {
        return;
    }

    _loading = true;
    clearPendingSend();

    const QJsonObject values = v.toObject();
    if (values.contains("host")) {
        setHost(values["host"].toString());
    }
    if (values.contains("port")) {
        setPort(values["port"].toInt());
    }
    if (values.contains("srcAddr")) {
        setSrcAddr(values["srcAddr"].toInt());
    }
    if (values.contains("dstAddr")) {
        setDstAddr(values["dstAddr"].toInt());
    }
    if (values.contains("levels") && values["levels"].isArray()) {
        const QJsonArray levels = values["levels"].toArray();
        for (int i = 0; i < kChannelCount && i < levels.size(); ++i) {
            setChannelLevel(i, levels[i].toInt(), false);
        }
    }
    if (values.contains("enable")) {
        setEnable(values["enable"].toBool());
    }

    _loading = false;
    flushPendingUi();
    if (_modelReady) {
        ensureTcpClient();
        if (_enable) {
            sendCurrentLevels(true);
        }
    }
}

ConnectionPolicy FTAJTDataModel::portConnectionPolicy(PortType portType, PortIndex index) const
{
    Q_UNUSED(index);
    switch (portType) {
    case PortType::In:
    case PortType::Out:
        return ConnectionPolicy::Many;
    case PortType::None:
        break;
    }
    return ConnectionPolicy::One;
}

int FTAJTDataModel::clampLevel(int level)
{
    return std::clamp(level, 0, 255);
}

void FTAJTDataModel::setChannelLevel(int index, int level, bool send)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    level = clampLevel(level);
    const bool changed = (_levels[index] != level);
    if (!changed) {
        return;
    }
    _levels[index] = level;

    // 本地值 / 输出立即更新；界面控件合并刷新，避免高频 setInData 卡 UI
    scheduleUiUpdate(index);
    updateOutputPort(index, level);
    AbstractDelegateModel::stateFeedBack(QString("/CH%1").arg(index + 1), level);

    if (send && !_loading && _enable) {
        scheduleSend(index, false);
    }
}

void FTAJTDataModel::scheduleUiUpdate(int index)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    _uiDirtyMask |= static_cast<quint8>(1u << index);
    if (_loading) {
        return; // load 结束时统一 flush
    }
    if (_uiDebounce) {
        _uiDebounce->start();
    }
}

void FTAJTDataModel::flushPendingUi()
{
    if (!_interface || _uiDirtyMask == 0) {
        return;
    }
    const quint8 mask = _uiDirtyMask;
    _uiDirtyMask = 0;
    for (int i = 0; i < kChannelCount; ++i) {
        if (mask & (1u << i)) {
            _interface->setChannelLevel(i, _levels[i]);
        }
    }
}

void FTAJTDataModel::sendCurrentLevels(bool immediate)
{
    if (_loading || !_modelReady || !_enable) {
        return;
    }
    scheduleSend(-1, true);
    if (immediate) {
        flushPendingSend();
    }
}

void FTAJTDataModel::sendRawLevels(const std::array<int, kChannelCount> &levels)
{
    // 强制下发指定亮度（用于全关），不依赖 enable，不改本地通道值
    if (_loading || !_connected || !_tcpClient) {
        return;
    }
    clearPendingSend();

    std::array<quint8, kChannelCount> channels{};
    for (int i = 0; i < kChannelCount; ++i) {
        channels[i] = static_cast<quint8>(clampLevel(levels[i]));
    }
    const QByteArray frame = buildDimFrame(channels);
    _tcpClient->sendMessage(QString::fromLatin1(frame.toHex()), 0);
}

void FTAJTDataModel::clearPendingSend()
{
    if (_sendDebounce) {
        _sendDebounce->stop();
    }
    _sendPending = false;
    _sendAllPending = false;
    _pendingChannel = -1;
}

void FTAJTDataModel::updateOutputPort(int index, int value)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    _outputData[index] = std::make_shared<NodeDataTypes::VariableData>(value);
    Q_EMIT dataUpdated(index);
}

void FTAJTDataModel::scheduleSend(int channelIndex, bool sendAll)
{
    if (_loading || !_modelReady || !_enable) {
        return;
    }
    if (sendAll) {
        _sendAllPending = true;
        _pendingChannel = -1;
    } else if (!_sendAllPending) {
        if (_pendingChannel < 0) {
            _pendingChannel = channelIndex;
        } else if (_pendingChannel != channelIndex) {
            _sendAllPending = true;
            _pendingChannel = -1;
        }
    }
    _sendPending = true;
    if (_sendDebounce) {
        _sendDebounce->start();
    }
}

void FTAJTDataModel::flushPendingSend()
{
    if (!_sendPending || _loading || !_enable) {
        return;
    }
    const bool sendAll = _sendAllPending;
    const int channel = _pendingChannel;
    _sendPending = false;
    _sendAllPending = false;
    _pendingChannel = -1;
    sendDimFrame(sendAll, channel);
}

void FTAJTDataModel::sendDimFrame(bool sendAll, int channelIndex)
{
    if (_loading || !_enable || !_connected || !_tcpClient) {
        return;
    }

    std::array<quint8, kChannelCount> channels{};
    if (sendAll || channelIndex < 0 || channelIndex >= kChannelCount) {
        for (int i = 0; i < kChannelCount; ++i) {
            channels[i] = static_cast<quint8>(clampLevel(_levels[i]));
        }
    } else {
        for (int i = 0; i < kChannelCount; ++i) {
            channels[i] = (i == channelIndex)
                ? static_cast<quint8>(clampLevel(_levels[i]))
                : kNoControl;
        }
    }

    const QByteArray frame = buildDimFrame(channels);
    _tcpClient->sendMessage(QString::fromLatin1(frame.toHex()), 0);
}

QByteArray FTAJTDataModel::buildDimFrame(const std::array<quint8, kChannelCount> &channels) const
{
    QByteArray body;
    body.reserve(10);
    body.append(static_cast<char>(_srcAddr & 0xFF));
    body.append(static_cast<char>(_dstAddr & 0xFF));
    body.append(static_cast<char>(kTypeDim));
    body.append(static_cast<char>(kFuncControl));
    for (int i = 0; i < kChannelCount; ++i) {
        body.append(static_cast<char>(channels[i]));
    }

    const quint8 len = static_cast<quint8>(body.size() + 1);
    quint16 sum = len;
    for (int i = 0; i < body.size(); ++i) {
        sum += static_cast<quint8>(body.at(i));
    }

    QByteArray frame;
    frame.reserve(14);
    frame.append(static_cast<char>(0xF7));
    frame.append(static_cast<char>(len));
    frame.append(body);
    frame.append(static_cast<char>(sum & 0xFF));
    frame.append(static_cast<char>(0xFD));
    return frame;
}

} // namespace Nodes
