#include "PosiStageNetClientDataModel.hpp"

#include <psn_lib.hpp>

#include <QNetworkDatagram>
#include <QVariantList>

namespace Nodes {

/**
 * @brief 构造函数：初始化 UI、输出缓存、定时器，并绑定 UI 控件信号
 */
PosiStageNetClientDataModel::PosiStageNetClientDataModel()
{
    InPortCount = 1;
    OutPortCount = 3;
    Caption = PLUGIN_NAME;
    CaptionVisible = true;
    WidgetEmbeddable = false;
    Resizable = true;
    PortEditable = false;

    widget = new PosiStageNetClientInterface();
    m_connectedOutput = std::make_shared<VariableData>(QVariant(false));
    m_trackerCountOutput = std::make_shared<VariableData>(QVariant(0));
    m_trackersDataOutput = std::make_shared<VariableData>(QVariantMap());
    widget->resetStatus();

    m_udpSocket = new QUdpSocket(this);
    m_receiveTimer = new QTimer(this);
    m_receiveTimer->setInterval(kReceivePollIntervalMs);
    connect(m_receiveTimer, &QTimer::timeout, this, &PosiStageNetClientDataModel::onReceivePoll);

    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(kConnectionTimeoutMs);
    connect(m_watchdogTimer, &QTimer::timeout, this, &PosiStageNetClientDataModel::onConnectionWatchdog);
  
    {
        QSignalBlocker blocker(widget->multicastAddressEdit);

        widget->multicastAddressEdit->setText(m_multicastAddress);
    }
  
    {
        QSignalBlocker blocker(widget->portSpinBox);
        widget->portSpinBox->setValue(m_port);
    }

    {
        QSignalBlocker blocker(widget->enableCheckBox);
        widget->enableCheckBox->setChecked(m_enabled);
    }

    connect(widget->enableCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        setEnabled(checked);
    }, Qt::AutoConnection);

    connect(widget->multicastAddressEdit, &QLineEdit::editingFinished, this, [this]() {
        setMulticastAddress(widget->multicastAddressEdit->text().trimmed());
    }, Qt::AutoConnection);

    connect(widget->portSpinBox, &IntDragValueWidget::valueChanged, this, [this](int newValue) {
        setPort(newValue);
    }, Qt::AutoConnection);

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "enabled";
        b.control = widget->enableCheckBox;
        AbstractDelegateModel::registerExternalBinding("/enabled", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "multicastAddress";
        b.control = widget->multicastAddressEdit;
        AbstractDelegateModel::registerExternalBinding("/multicastAddress", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "port";
        b.control = widget->portSpinBox;
        AbstractDelegateModel::registerExternalBinding("/port", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = widget->connectionLabel;
        AbstractDelegateModel::registerExternalBinding("/connected", this, b);
    }
    m_psnDecoder.reset(new ::psn::psn_decoder());
}

/**
 * @brief 析构函数：按照"最外层先关闭"的原则安全释放所有资源
 *
 * 释放顺序（避免挂起和 use-after-free）：
 *   1. 置 m_destructing=true：屏蔽后续 UI 操作和状态推送
 *   2. 退订 GlobalEventBus：避免总线在析构中后期继续派发事件到已失效的 this
 *   3. 强制断开所有对外 connect（widget 子控件 → this）：防止子控件在销毁中触发 lambda
 *   4. stopReceiver()：停定时器 + 离组播 + socket.abort() 强制关闭网络栈
 *   5. widget->deleteLater()：在事件循环中销毁 UI（之后不再有任何代码访问 widget 成员）
 *   6. 派生类成员（unique_ptr、shared_ptr）析构（此时所有回调已停止）
 *   7. 基类 ~QObject()：delete QUdpSocket/QTimer（this 的 children，已停止回调安全）
 */
PosiStageNetClientDataModel::~PosiStageNetClientDataModel()
{
    m_destructing = true;

    auto *bus = GlobalEventBus::instance();
    if (bus) {
        bus->unsubscribe(makeFullOscAddress("/enabled"), this);
        bus->unsubscribe(makeFullOscAddress("/multicastAddress"), this);
        bus->unsubscribe(makeFullOscAddress("/port"), this);
    }

    if (widget) {
        widget->enableCheckBox->disconnect(this);
        widget->multicastAddressEdit->disconnect(this);
        widget->portSpinBox->disconnect(this);
    }
    if (m_receiveTimer) {
        m_receiveTimer->disconnect(this);
    }
    if (m_watchdogTimer) {
        m_watchdogTimer->disconnect(this);
    }
    if (m_udpSocket) {
        m_udpSocket->disconnect(this);
    }

    stopReceiver();

    if (widget) {
        widget->deleteLater();
        widget = nullptr;
    }
}

/**
 * @brief 模型就绪钩子：订阅 GlobalEventBus 上所有对外命令地址
 *
 * WebInterface / 其他节点发布 Command 事件到以下地址即可远程控制本节点：
 *   /enabled          bool     启用/停止接收
 *   /multicastAddress QString  设置组播地址
 *   /port             int      设置端口
 *   /start            (none)   等同于 setEnabled(true)
 *   /stop             (none)   等同于 setEnabled(false)
 */
void PosiStageNetClientDataModel::afterModelReady()
{
    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/enabled"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/multicastAddress"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

/**
 * @brief 全局事件分发：Command 类事件调用对应 setter 或命令处理
 */
void PosiStageNetClientDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }
    const int lastSlash = ev.address.lastIndexOf('/');
    const QString localPath = (lastSlash < 0) ? ev.address : ev.address.mid(lastSlash + 1);

    if (localPath == QStringLiteral("enabled")) {
        setEnabled(ev.payload.toBool());
    } else if (localPath == QStringLiteral("multicastAddress")) {
        setMulticastAddress(ev.payload.toString());
    } else if (localPath == QStringLiteral("port")) {
        setPort(ev.payload.toInt());
    } 
}

/**
 * @brief 获取指定端口的数据类型（统一返回 VariableData）
 */
NodeDataType PosiStageNetClientDataModel::dataType(PortType, PortIndex) const
{
    return VariableData().type();
}

/**
 * @brief 获取端口标题名称
 */
QString PosiStageNetClientDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In && portIndex == EnablePort) {
        return QStringLiteral("ENABLE");
    }
    if (portType == PortType::Out) {
        switch (portIndex) {
            case ConnectedPort:    return QStringLiteral("CONNECTED");
            case TrackerCountPort: return QStringLiteral("TRACKER_COUNT");
            case TrackersDataPort: return QStringLiteral("TRACKERS");
            default: break;
        }
    }
    return {};
}

/**
 * @brief 获取输出端口数据
 */
std::shared_ptr<NodeData> PosiStageNetClientDataModel::outData(PortIndex const portIndex)
{
    switch (portIndex) {
        case ConnectedPort:    return m_connectedOutput;
        case TrackerCountPort: return m_trackerCountOutput;
        case TrackersDataPort: return m_trackersDataOutput;
        default: break;
    }
    return nullptr;
}

/**
 * @brief 设置输入端口数据（ENABLE 驱动开关）
 */
void PosiStageNetClientDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    if (portIndex != EnablePort) {
        return;
    }
    auto var = std::dynamic_pointer_cast<VariableData>(data);
    if (!var) {
        setEnabled(false);
        return;
    }
    setEnabled(isBoolTrue(*var));
}

/**
 * @brief 端口连接策略：输入输出均为多连接
 */
ConnectionPolicy PosiStageNetClientDataModel::portConnectionPolicy(PortType portType, PortIndex) const
{
    return (portType == PortType::In || portType == PortType::Out)
               ? ConnectionPolicy::Many
               : ConnectionPolicy::One;
}

/**
 * @brief 保存节点配置到 JSON
 */
QJsonObject PosiStageNetClientDataModel::save() const
{
    QJsonObject values;
    values[QStringLiteral("enabled")] = m_enabled;
    values[QStringLiteral("multicastAddress")] = m_multicastAddress;
    values[QStringLiteral("port")] = m_port;
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

/**
 * @brief 从 JSON 加载节点配置
 */
void PosiStageNetClientDataModel::load(const QJsonObject &p)
{
    const QJsonValue v = p.value(QStringLiteral("values"));
    if (!v.isObject()) {
        return;
    }
    const QJsonObject values = v.toObject();

    if (values.contains(QStringLiteral("multicastAddress"))) {
        setMulticastAddress(values.value(QStringLiteral("multicastAddress")).toString());
    }
    if (values.contains(QStringLiteral("port"))) {
        setPort(values.value(QStringLiteral("port")).toInt(kDefaultPort));
    }
    if (values.contains(QStringLiteral("enabled"))) {
        setEnabled(values.value(QStringLiteral("enabled")).toBool());
    }
}

/**
 * @brief 获取嵌入控件指针
 */
QWidget *PosiStageNetClientDataModel::embeddedWidget()
{
    return widget;
}

/**
 * @brief 设置启用状态；启用时自动启动组播接收，禁用时停止
 */
void PosiStageNetClientDataModel::setEnabled(bool enabled)
{
    if (m_destructing) {
        return;
    }
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    {
        QSignalBlocker blocker(widget->enableCheckBox);
        widget->enableCheckBox->setChecked(m_enabled);
    }
    Q_EMIT enabledChanged(m_enabled);

    if (m_enabled) {
        startReceiver();
    } else {
        stopReceiver();
    }
}

/**
 * @brief 设置组播地址；若正在接收则重启以应用新地址
 */
void PosiStageNetClientDataModel::setMulticastAddress(const QString &address)
{
    if (m_destructing) {
        return;
    }
    if (m_multicastAddress == address) {
        return;
    }
    m_multicastAddress = address;
    {
        QSignalBlocker blocker(widget->multicastAddressEdit);
        widget->multicastAddressEdit->setText(m_multicastAddress);
    }
    Q_EMIT multicastAddressChanged(m_multicastAddress);

    if (m_enabled) {
        stopReceiver();
        startReceiver();
    }
}

/**
 * @brief 设置端口；若正在接收则重启以应用新端口
 */
void PosiStageNetClientDataModel::setPort(int port)
{
    if (m_destructing) {
        return;
    }
    if (port < 1 || port > 65535) {
        return;
    }
    if (m_port == port) {
        return;
    }
    m_port = port;
    {
        QSignalBlocker blocker(widget->portSpinBox);
        widget->portSpinBox->setValue(m_port);
    }
    Q_EMIT portChanged(m_port);

    if (m_enabled) {
        stopReceiver();
        startReceiver();
    }
}

/**
 * @brief 启动 UDP 组播接收器：绑定端口、加入组播组、启动轮询和看门狗
 */
bool PosiStageNetClientDataModel::startReceiver()
{
    if (m_bound && m_udpSocket) {
        stopReceiver();
    }

    const QHostAddress groupAddress(m_multicastAddress);
    if (groupAddress.isNull()) {
        setConnected(false);
        return false;
    }

    const auto bindMode = QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint;
    if (!m_udpSocket->bind(QHostAddress::AnyIPv4, static_cast<quint16>(m_port), bindMode)) {
        setConnected(false);
        m_bound = false;
        return false;
    }

    if (!m_udpSocket->joinMulticastGroup(groupAddress)) {
        m_udpSocket->close();
        setConnected(false);
        m_bound = false;
        return false;
    }

    m_bound = true;
    m_lastReceiveTimeMs = 0;
    m_lastFrameId = 0;

    m_receiveTimer->start();
    m_watchdogTimer->start();
    return true;
}

/**
 * @brief 停止 UDP 组播接收器
 *
 * 详细步骤：
 *   1. 停两个定时器（避免在 stop 过程中再次触发回调）
 *   2. 若处于析构阶段：只做网络相关清理，不再访问 widget/发送信号
 *   3. 非析构阶段：先 leaveMulticastGroup，再 socket.close() → 顺序不可反（Windows 网络栈 bug）
 *   4. socket.abort() 确保异步缓冲区立即释放、不落事件循环；Windows 上 close() 后加 abort() 是防挂起的关键
 */
void PosiStageNetClientDataModel::stopReceiver()
{
    if (m_receiveTimer) {
        m_receiveTimer->stop();
    }
    if (m_watchdogTimer) {
        m_watchdogTimer->stop();
    }

    if (m_udpSocket) {
        if (m_bound) {
            const QHostAddress groupAddress(m_multicastAddress);
            if (!groupAddress.isNull()) {
                m_udpSocket->leaveMulticastGroup(groupAddress);
            }
            m_udpSocket->close();
            m_udpSocket->abort();
        } else {
            m_udpSocket->abort();
        }
    }
    m_bound = false;
    m_lastReceiveTimeMs = 0;

    if (m_destructing) {
        m_connected = false;
        return;
    }

    setConnected(false);
    if (widget) {
        widget->resetStatus();
    }
    m_trackerCountOutput = std::make_shared<VariableData>(QVariant(0));
    m_trackersDataOutput = std::make_shared<VariableData>(QVariantMap());
    Q_EMIT dataUpdated(TrackerCountPort);
    Q_EMIT dataUpdated(TrackersDataPort);
}

/**
 * @brief 设置连接状态：变化时更新 UI、CONNECTED 输出端口和 StatusContainer 反馈
 */
void PosiStageNetClientDataModel::setConnected(bool connected)
{
    if (m_destructing) {
        return;
    }
    if (m_connected == connected) {
        return;
    }
    m_connected = connected;
    if (widget) {
        widget->updateConnectionStatus(m_connected);
    }
    m_connectedOutput = std::make_shared<VariableData>(QVariant(m_connected));
    Q_EMIT dataUpdated(ConnectedPort);
    Q_EMIT connectedChanged(m_connected);
}

/**
 * @brief 轮询接收 UDP 数据，逐包交给 psn_decoder 解码；新帧到达时更新输出
 */
void PosiStageNetClientDataModel::onReceivePoll()
{
    if (m_destructing) {
        return;
    }
    if (!m_udpSocket || !m_bound) {
        return;
    }

    while (m_udpSocket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        const QByteArray payload = datagram.data();
        if (payload.isEmpty()) {
            continue;
        }

        if (!m_psnDecoder->decode(payload.constData(), static_cast<size_t>(payload.size()))) {
            continue;
        }

        m_lastReceiveTimeMs = QDateTime::currentMSecsSinceEpoch();
        if (!m_connected) {
            setConnected(true);
        }

        const ::psn::psn_decoder::data_t &data = m_psnDecoder->get_data();
        const uint8_t curFrameId = data.header.frame_id;

        if (curFrameId != m_lastFrameId) {
            m_lastFrameId = curFrameId;

            const ::psn::psn_decoder::info_t &info = m_psnDecoder->get_info();
            const QString systemName = QString::fromStdString(info.system_name);

            widget->updateSystemName(systemName);
            widget->updateFrameInfo(static_cast<int>(curFrameId),
                                    static_cast<int>(data.trackers.size()));

            QMap<int, QString> trackerDisplay;
            const auto &names = info.tracker_names;
            for (const auto &kv : data.trackers) {
                const uint16_t id = kv.first;
                const ::psn::tracker &t = kv.second;
                QString name = QString::fromStdString(t.get_name());
                if (name.isEmpty()) {
                    auto it = names.find(static_cast<int>(id));
                    if (it != names.end()) {
                        name = QString::fromStdString(it->second);
                    }
                }
                QString display = name;
                if (t.is_pos_set()) {
                    const ::psn::float3 p = t.get_pos();
                    display += QStringLiteral("  (%1, %2, %3)")
                                   .arg(p.x, 0, 'f', 2)
                                   .arg(p.y, 0, 'f', 2)
                                   .arg(p.z, 0, 'f', 2);
                }
                trackerDisplay.insert(static_cast<int>(id), display);
            }
            widget->updateTrackerList(trackerDisplay);

            const int trackerCount = static_cast<int>(data.trackers.size());
            m_trackerCountOutput = std::make_shared<VariableData>(QVariant(trackerCount));
            Q_EMIT dataUpdated(TrackerCountPort);

            m_trackersDataOutput = std::make_shared<VariableData>(buildTrackersVariantMap());
            Q_EMIT dataUpdated(TrackersDataPort);
        }
    }
}

/**
 * @brief 看门狗：若超过超时阈值未收到数据则标记断连
 */
void PosiStageNetClientDataModel::onConnectionWatchdog()
{
    if (m_destructing) {
        return;
    }
    if (!m_bound) {
        return;
    }
    if (m_lastReceiveTimeMs <= 0) {
        return;
    }
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_lastReceiveTimeMs;
    if (elapsed >= kConnectionTimeoutMs) {
        setConnected(false);
    }
}

/**
 * @brief 将 psn_decoder 中的追踪器数据转换为 QVariantMap
 *
 * 输出格式：
 * {
 *   "system_name": "...",
 *   "frame_id": N,
 *   "timestamp_usec": N,
 *   "trackers": [
 *     { "id":1, "name":"T1",
 *       "pos":[0,0,0],
 *       "speed":[0,0,0],
 *       "ori":[0,0,0],
 *       "accel":[0,0,0],
 *       "target_pos":[0,0,0],
 *       "status":0, "timestamp":0 }
 *   ]
 * }
 */
QVariantMap PosiStageNetClientDataModel::buildTrackersVariantMap() const
{
    QVariantMap root;
    const ::psn::psn_decoder::info_t &info = m_psnDecoder->get_info();
    const ::psn::psn_decoder::data_t &data = m_psnDecoder->get_data();

    root.insert(QStringLiteral("system_name"), QString::fromStdString(info.system_name));
    root.insert(QStringLiteral("frame_id"), static_cast<int>(data.header.frame_id));
    root.insert(QStringLiteral("timestamp_usec"),
                static_cast<qlonglong>(data.header.timestamp_usec));

    QVariantList trackerList;
    const auto &names = info.tracker_names;
    for (const auto &kv : data.trackers) {
        const uint16_t id = kv.first;
        const ::psn::tracker &t = kv.second;

        QVariantMap tm;
        tm.insert(QStringLiteral("id"), static_cast<int>(id));

        QString name = QString::fromStdString(t.get_name());
        if (name.isEmpty()) {
            auto it = names.find(static_cast<int>(id));
            if (it != names.end()) {
                name = QString::fromStdString(it->second);
            }
        }
        tm.insert(QStringLiteral("name"), name);

        if (t.is_pos_set()) {
            const ::psn::float3 p = t.get_pos();
            QVariantList pm;
            pm.append(p.x);
            pm.append(p.y);
            pm.append(p.z);
            tm.insert(QStringLiteral("pos"), pm);
        }
        if (t.is_speed_set()) {
            const ::psn::float3 p = t.get_speed();
            QVariantList pm;
            pm.append(p.x);
            pm.append(p.y);
            pm.append(p.z);
            tm.insert(QStringLiteral("speed"), pm);
        }
        if (t.is_ori_set()) {
            const ::psn::float3 p = t.get_ori();
            QVariantList pm;
            pm.append(p.x);
            pm.append(p.y);
            pm.append(p.z);
            tm.insert(QStringLiteral("ori"), pm);
        }
        if (t.is_accel_set()) {
            const ::psn::float3 p = t.get_accel();
            QVariantList pm;
            pm.append(p.x);
            pm.append(p.y);
            pm.append(p.z);
            tm.insert(QStringLiteral("accel"), pm);
        }
        if (t.is_target_pos_set()) {
            const ::psn::float3 p = t.get_target_pos();
            QVariantList pm;
            pm.append(p.x);
            pm.append(p.y);
            pm.append(p.z);
            tm.insert(QStringLiteral("target_pos"), pm);
        }
        if (t.is_status_set()) {
            tm.insert(QStringLiteral("status"), static_cast<double>(t.get_status()));
        }
        if (t.is_timestamp_set()) {
            tm.insert(QStringLiteral("timestamp"),
                      static_cast<qlonglong>(t.get_timestamp()));
        }

        trackerList.append(tm);
    }
    root.insert(QStringLiteral("trackers"), trackerList);
    return root;
}

/**
 * @brief 将 VariableData 值解析为布尔（兼容 bool/int/字符串 true|1）
 */
bool PosiStageNetClientDataModel::isBoolTrue(const VariableData &data)
{
    const QVariant value = data.value();
    if (value.typeId() == QMetaType::Bool) {
        return value.toBool();
    }
    if (value.canConvert<int>()) {
        return value.toInt() != 0;
    }
    const QString text = value.toString().trimmed().toLower();
    return text == QLatin1String("true") || text == QLatin1String("1");
}

} // namespace Nodes
