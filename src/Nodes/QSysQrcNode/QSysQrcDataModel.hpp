/**
 * @file QSysQrcDataModel.hpp
 * @brief Q-SYS ECP 节点：从媒体库加载 Extract Named Controls XML，自动同步端口并读写
 */
#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonObject>
#include <QPointer>
#include <QSignalBlocker>
#include <QTimer>
#include <QVariant>
#include <QXmlStreamReader>

#include "Common/AppConfig/ConstantDefines.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "EcpClient.h"
#include "PluginDefinition.hpp"
#include "QSysQrcInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class QSysQrcDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kChangeGroupId = 1;
        static constexpr int kPollPeriodMs = 500;
        static constexpr quint16 kEcpPort = EcpClient::kDefaultPort;

        static constexpr int kRoleIndex = Qt::UserRole;
        static constexpr int kRoleControlId = Qt::UserRole + 1;

        Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
        Q_PROPERTY(QString pin READ pin WRITE setPin NOTIFY pinChanged)
        Q_PROPERTY(QString xmlPath READ xmlPath WRITE setXmlPath NOTIFY xmlPathChanged)
        Q_PROPERTY(bool connected READ connected WRITE setConnectedDesired NOTIFY connectedChanged)

    public:
        struct NamedControl
        {
            QString id;
            QString controlId;
            QString controlName;
            QString componentName;
            QString componentLabel;
            QString type;
            QString mode;
            double minimumValue = 0;
            double maximumValue = 0;
            int size = 1;

            QVariant value;
            QString stringRepr;
            double position = 0;
        };

        QSysQrcDataModel()
        {
            InPortCount = 0;
            OutPortCount = 0;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            m_client = new EcpClient(this);
            m_client->setAutoReconnect(true);

            connect(m_client, &EcpClient::connectedChanged, this, &QSysQrcDataModel::onClientConnected);
            connect(m_client, &EcpClient::socketError, this, [this](const QString &msg) {
                setStatus(QStringLiteral("错误: %1").arg(msg));
            });
            connect(m_client, &EcpClient::lineReceived, this, &QSysQrcDataModel::onEcpLine);

            syncParametersFromWidget();
            widget->updateConnectionStatus(false);
            connectUiSignals();
            registerOscBindings();
        }

        ~QSysQrcDataModel() override
        {
            m_shuttingDown = true;

            GlobalEventBus::instance()->unsubscribe(this);

            if (m_client) {
                // 先断开信号，避免 disconnect/abort 过程中回调 UI
                disconnect(m_client, nullptr, this, nullptr);
                m_client->setAutoReconnect(false);
                m_client->disconnectFromHost();
                m_client = nullptr; // 实际由 QObject 父子关系销毁
            }

            clearStatusContainerBindings();

            // WidgetEmbeddable=false 时 widget 由模型持有；若曾被 proxy 接管则可能已销毁
            if (widget) {
                widget->setParent(nullptr);
                delete widget;
                widget = nullptr;
            }
        }

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        QString portCaption(PortType, PortIndex portIndex) const override
        {
            const int i = static_cast<int>(portIndex);
            if (i < 0) {
                return {};
            }
            if (i < m_controls.size()) {
                return QStringLiteral("[%1] %2").arg(i).arg(m_controls[i].id);
            }
            return QStringLiteral("[%1] —").arg(i);
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            const int i = static_cast<int>(portIndex);
            if (i < 0 || i >= static_cast<int>(OutPortCount)) {
                return nullptr;
            }
            ensureCaches();
            if (!m_outValues[i]) {
                m_outValues[i] = std::make_shared<VariableData>(
                    i < m_controls.size() ? m_controls[i].value : QVariant());
            }
            return m_outValues[i];
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }
            const int i = static_cast<int>(portIndex);
            if (i < 0 || i >= static_cast<int>(InPortCount) || i >= m_controls.size()) {
                return;
            }
            writeControl(i, var->value());
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            return (portType == PortType::In || portType == PortType::Out)
                ? ConnectionPolicy::Many
                : ConnectionPolicy::One;
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("host")] = m_host;
            values[QStringLiteral("userName")] = m_userName;
            values[QStringLiteral("pin")] = m_pin;
            values[QStringLiteral("xmlPath")] = m_xmlPath;

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject values = v.toObject();
            setHost(values.value(QStringLiteral("host")).toString(m_host));
            setUserName(values.value(QStringLiteral("userName")).toString(m_userName));
            setPin(values.value(QStringLiteral("pin")).toString(m_pin));
            const QString xml = values.value(QStringLiteral("xmlPath")).toString(m_xmlPath);
            if (!xml.isEmpty()) {
                setXmlPath(xml);
                loadXmlFile(xml, false);
            }
            ensureCaches();
        }

        QWidget *embeddedWidget() override { return widget; }

        QString host() const { return m_host; }
        QString userName() const { return m_userName; }
        QString pin() const { return m_pin; }
        QString xmlPath() const { return m_xmlPath; }
        bool connected() const { return m_connected; }

        void setHost(const QString &value)
        {
            const QString v = value.trimmed();
            if (m_host == v) {
                return;
            }
            m_host = v;
            if (widget) {
                QSignalBlocker b(widget->hostEdit);
                widget->hostEdit->setText(m_host);
            }
            applyEndpoint();
            Q_EMIT hostChanged(m_host);
        }

        void setUserName(const QString &value)
        {
            if (m_userName == value) {
                return;
            }
            m_userName = value;
            if (widget) {
                QSignalBlocker b(widget->userEdit);
                widget->userEdit->setText(m_userName);
            }
            Q_EMIT userNameChanged(m_userName);
        }

        void setPin(const QString &value)
        {
            if (m_pin == value) {
                return;
            }
            m_pin = value;
            if (widget) {
                QSignalBlocker b(widget->pinEdit);
                widget->pinEdit->setText(m_pin);
            }
            Q_EMIT pinChanged(m_pin);
        }

        void setXmlPath(const QString &value)
        {
            const QString v = value.trimmed();
            if (m_xmlPath == v) {
                return;
            }
            m_xmlPath = v;
            if (widget) {
                QSignalBlocker b(widget->xmlSelector);
                widget->xmlSelector->setCurrentValue(m_xmlPath);
            }
            Q_EMIT xmlPathChanged(m_xmlPath);
        }

        /** UI / OSC：勾选连接，取消勾选断开 */
        void setConnectedDesired(bool wantConnected)
        {
            if (wantConnected) {
                requestConnect();
            } else {
                requestDisconnect();
            }
        }

    Q_SIGNALS:
        void hostChanged(const QString &value);
        void userNameChanged(const QString &value);
        void pinChanged(const QString &value);
        void xmlPathChanged(const QString &value);
        void connectedChanged(bool connected);

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress(QLatin1String("/host")),
                           this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QLatin1String("/xmlPath")),
                           this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QLatin1String("/connected")),
                           this, SLOT(onGlobalEvent(GlobalEvent)));

            applyEndpoint();
            if (!m_xmlPath.isEmpty() && m_controls.isEmpty()) {
                loadXmlFile(m_xmlPath, false);
            }
            // 节点就绪后自动连接（按钮仅作状态 / OSC）
            QTimer::singleShot(0, this, &QSysQrcDataModel::requestConnect);
        }

    private slots:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (m_shuttingDown || ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addr = ev.address;
            if (addr == makeFullOscAddress("/host")) {
                setHost(ev.payload.toString());
            } else if (addr == makeFullOscAddress("/xmlPath")) {
                setXmlPath(ev.payload.toString());
                loadXmlFile(m_xmlPath, true);
            } else if (addr == makeFullOscAddress("/connected")) {
                setConnectedDesired(ev.payload.toBool());
            }
        }

        void onClientConnected(bool connected)
        {
            if (m_shuttingDown) {
                return;
            }
            setConnected(connected);
            m_changeGroupReady = false;
            if (!connected) {
                if (widget) {
                    widget->designLabel->setText(QStringLiteral("Design: —"));
                }
                setStatus(QStringLiteral("已断开"));
                return;
            }
            setStatus(QStringLiteral("已连接，初始化…"));
            m_client->sendLine(QStringLiteral("sg"));
            QTimer::singleShot(200, this, &QSysQrcDataModel::maybeLoginAndSetupChangeGroup);
        }

        void onEcpLine(const QString &line)
        {
            if (m_shuttingDown || !widget) {
                return;
            }
            const QStringList tok = EcpClient::tokenize(line);
            if (tok.isEmpty()) {
                return;
            }
            const QString cmd = tok.at(0);

            if (cmd == QLatin1String("sr") && tok.size() >= 5) {
                widget->designLabel->setText(
                    QStringLiteral("Design: %1 | primary=%2 active=%3")
                        .arg(tok.at(1))
                        .arg(tok.at(3), tok.at(4)));
                return;
            }
            if (cmd == QLatin1String("login_required")) {
                tryLogin();
                return;
            }
            if (cmd == QLatin1String("login_success")) {
                setStatus(QStringLiteral("登录成功"));
                setupChangeGroup();
                return;
            }
            if (cmd == QLatin1String("login_failed")) {
                setStatus(QStringLiteral("登录失败"));
                return;
            }
            if (cmd == QLatin1String("cv") && tok.size() >= 5) {
                applyControlValue(tok.at(1), tok.at(2), tok.at(3).toDouble(), tok.at(4).toDouble());
                return;
            }
            if (cmd == QLatin1String("cvv") && tok.size() >= 3) {
                const QString id = tok.at(1);
                bool ok = false;
                const int count = tok.at(2).toInt(&ok);
                if (ok && count > 0) {
                    const int valueCountIndex = 3 + count;
                    if (valueCountIndex < tok.size()) {
                        const int vcount = tok.at(valueCountIndex).toInt();
                        if (vcount > 0 && valueCountIndex + 1 < tok.size()) {
                            applyControlValue(id, tok.at(3), tok.at(valueCountIndex + 1).toDouble(), 0);
                        }
                    }
                }
                return;
            }
            if (cmd == QLatin1String("cgpa")) {
                return;
            }
            if (cmd == QLatin1String("bad_id") && tok.size() >= 2) {
                setStatus(QStringLiteral("未知控件: %1").arg(tok.at(1)));
                return;
            }
            if (cmd == QLatin1String("bad_command")) {
                setStatus(QStringLiteral("命令错误: %1").arg(line));
                return;
            }
            if (cmd == QLatin1String("core_not_active")) {
                setStatus(QStringLiteral("Core 非 Active（冗余待机）"));
                return;
            }
            if (cmd == QLatin1String("rc")) {
                setStatus(QStringLiteral("Core 关闭连接"));
                return;
            }
        }

        void requestConnect()
        {
            if (m_shuttingDown || !m_client) {
                return;
            }
            syncParametersFromWidget();
            applyEndpoint();
            if (m_host.isEmpty()) {
                setStatus(QStringLiteral("请填写主机"));
                if (widget) {
                    widget->updateConnectionStatus(false);
                }
                return;
            }
            if (m_controls.isEmpty()) {
                setStatus(QStringLiteral("请先选择 Named Controls XML"));
            }
            setStatus(QStringLiteral("正在连接 %1:%2 …").arg(m_host).arg(kEcpPort));
            m_client->connectToHost();
        }

        void requestDisconnect()
        {
            if (!m_client) {
                return;
            }
            m_client->setAutoReconnect(false);
            m_client->disconnectFromHost();
            m_client->setAutoReconnect(true);
            m_changeGroupReady = false;
            setStatus(QStringLiteral("已请求断开"));
        }

        void onXmlSelected(const QString &text)
        {
            const QString path = text.trimmed();
            setXmlPath(path);
            if (!path.isEmpty()) {
                loadXmlFile(path, true);
            }
        }

    private:
        void connectUiSignals()
        {
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
                setHost(widget->hostEdit->text().trimmed());
                requestConnect();
            });
            connect(widget->userEdit, &QLineEdit::editingFinished, this, [this]() {
                setUserName(widget->userEdit->text());
            });
            connect(widget->pinEdit, &QLineEdit::editingFinished, this, [this]() {
                setPin(widget->pinEdit->text());
            });
            connect(widget->xmlSelector, &SelectorComboBox::textChanged,
                    this, &QSysQrcDataModel::onXmlSelected);
        }

        void registerOscBindings()
        {
            registerBinding("/host", "host", widget->hostEdit);
            registerBinding("/userName", "userName", widget->userEdit);
            registerBinding("/pin", "pin", widget->pinEdit);
            registerBinding("/xmlPath", "xmlPath", widget->xmlSelector);
            registerBinding("/connected", "connected", widget->connectButton);
        }

        void registerBinding(const QString &path, const char *member, QWidget *control)
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = QString::fromUtf8(member);
            b.control = control;
            AbstractDelegateModel::registerExternalBinding(path, this, b);
        }

        void syncParametersFromWidget()
        {
            if (!widget) {
                return;
            }
            m_host = widget->hostEdit->text().trimmed();
            m_userName = widget->userEdit->text();
            m_pin = widget->pinEdit->text();
            m_xmlPath = widget->xmlSelector->text().trimmed();
        }

        void applyEndpoint()
        {
            if (!m_client) {
                return;
            }
            m_client->setEndpoint(m_host, kEcpPort);
        }

        void setStatus(const QString &text)
        {
            if (m_shuttingDown || !widget) {
                return;
            }
            widget->statusLabel->setText(QStringLiteral("状态: %1").arg(text));
        }

        void setConnected(bool connected)
        {
            if (m_connected == connected) {
                if (widget) {
                    widget->updateConnectionStatus(m_connected);
                }
                return;
            }
            m_connected = connected;
            if (widget) {
                widget->updateConnectionStatus(m_connected);
            }
            if (!m_shuttingDown) {
                Q_EMIT connectedChanged(m_connected);
            }
        }

        void clearStatusContainerBindings()
        {
            static const char *const kPaths[] = {
                "/host", "/userName", "/pin", "/xmlPath", "/connected",
            };
            auto *status = StatusContainer::instance();
            for (const char *rel : kPaths) {
                status->registerWidget(nullptr, makeFullOscAddress(QLatin1String(rel)));
            }
        }

        static QString resolveMediaPath(const QString &relativeOrAbsolute)
        {
            const QString trimmed = relativeOrAbsolute.trimmed();
            if (trimmed.isEmpty()) {
                return {};
            }
            const QFileInfo fi(trimmed);
            if (fi.isAbsolute()) {
                return QDir::cleanPath(trimmed);
            }
            return QDir::cleanPath(AppConstants::MEDIA_LIBRARY_STORAGE_DIR + QLatin1Char('/') + trimmed);
        }

        bool loadXmlFile(const QString &relativeOrAbsolute, bool notifyPorts)
        {
            const QString fullPath = resolveMediaPath(relativeOrAbsolute);
            if (fullPath.isEmpty()) {
                setStatus(QStringLiteral("XML 路径为空"));
                return false;
            }

            QFile file(fullPath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                setStatus(QStringLiteral("无法打开 XML: %1").arg(file.errorString()));
                return false;
            }

            QVector<NamedControl> parsed;
            QXmlStreamReader xml(&file);
            while (!xml.atEnd()) {
                xml.readNext();
                if (!xml.isStartElement() || xml.name() != QLatin1String("Control")) {
                    continue;
                }
                const auto attrs = xml.attributes();
                NamedControl c;
                c.id = attrs.value(QLatin1String("Id")).toString();
                if (c.id.isEmpty()) {
                    continue;
                }
                c.controlId = attrs.value(QLatin1String("ControlId")).toString();
                c.controlName = attrs.value(QLatin1String("ControlName")).toString();
                c.componentName = attrs.value(QLatin1String("ComponentName")).toString();
                c.componentLabel = attrs.value(QLatin1String("ComponentLabel")).toString();
                c.type = attrs.value(QLatin1String("Type")).toString();
                c.mode = attrs.value(QLatin1String("Mode")).toString();
                c.minimumValue = attrs.value(QLatin1String("MinimumValue")).toString().toDouble();
                c.maximumValue = attrs.value(QLatin1String("MaximumValue")).toString().toDouble();
                {
                    bool ok = false;
                    const int size = attrs.value(QLatin1String("Size")).toString().toInt(&ok);
                    c.size = ok ? size : 1;
                }
                parsed.push_back(c);
            }
            if (xml.hasError()) {
                setStatus(QStringLiteral("XML 解析错误: %1").arg(xml.errorString()));
                return false;
            }

            QHash<QString, NamedControl> oldById;
            for (const NamedControl &old : m_controls) {
                oldById.insert(old.id, old);
            }
            for (NamedControl &c : parsed) {
                if (oldById.contains(c.id)) {
                    const NamedControl &old = oldById.value(c.id);
                    c.value = old.value;
                    c.stringRepr = old.stringRepr;
                    c.position = old.position;
                }
            }

            m_controls = parsed;
            syncPortsFromControls(notifyPorts);
            rebuildControlList();
            publishAllOutputs();
            setStatus(QStringLiteral("已加载 %1 个 Named Control").arg(m_controls.size()));

            if (notifyPorts && m_client->isConnected()) {
                setupChangeGroup();
            }
            return true;
        }

        void syncPortsFromControls(bool notifyPorts)
        {
            const unsigned int newCount = static_cast<unsigned int>(qMax(0, m_controls.size()));
            auto syncSide = [this, newCount, notifyPorts](PortType type, unsigned int &countField) {
                const unsigned int oldCount = countField;
                if (newCount == oldCount) {
                    return;
                }
                if (notifyPorts) {
                    if (newCount > oldCount) {
                        Q_EMIT portsAboutToBeInserted(type, oldCount, newCount - 1);
                        countField = newCount;
                        Q_EMIT portsInserted();
                    } else {
                        Q_EMIT portsAboutToBeDeleted(type, newCount, oldCount - 1);
                        countField = newCount;
                        Q_EMIT portsDeleted();
                    }
                } else {
                    countField = newCount;
                }
            };
            syncSide(PortType::In, InPortCount);
            syncSide(PortType::Out, OutPortCount);
            ensureCaches();
        }

        void rebuildControlList()
        {
            if (!widget) {
                return;
            }
            QSignalBlocker blocker(widget->controlList);
            widget->controlList->clear();
            for (int i = 0; i < m_controls.size(); ++i) {
                const NamedControl &c = m_controls[i];
                const QString valueText = c.stringRepr.isEmpty()
                    ? (c.value.isValid() ? c.value.toString() : QStringLiteral("—"))
                    : c.stringRepr;
                auto *item = new QListWidgetItem(
                    QStringLiteral("[%1] %2  (%3/%4)  %5")
                        .arg(i)
                        .arg(c.id)
                        .arg(c.type, c.mode)
                        .arg(valueText));
                item->setData(kRoleIndex, i);
                item->setData(kRoleControlId, c.id);
                item->setToolTip(
                    QStringLiteral(
                        "Id=%1\nControlId=%2\nControlName=%3\nComponent=%4\n"
                        "Type=%5 Mode=%6\nMin=%7 Max=%8\nValue=%9")
                        .arg(c.id, c.controlId, c.controlName, c.componentName)
                        .arg(c.type, c.mode)
                        .arg(c.minimumValue)
                        .arg(c.maximumValue)
                        .arg(c.value.toString()));
                widget->controlList->addItem(item);
            }
        }

        void maybeLoginAndSetupChangeGroup()
        {
            if (!m_client->isConnected()) {
                return;
            }
            if (!m_userName.trimmed().isEmpty()) {
                tryLogin();
            } else {
                setupChangeGroup();
            }
        }

        void tryLogin()
        {
            syncParametersFromWidget();
            if (m_userName.trimmed().isEmpty()) {
                setStatus(QStringLiteral("需要登录，请填写用户/PIN"));
                return;
            }
            m_client->sendLine(QStringLiteral("login %1 %2")
                                   .arg(EcpClient::quoteArg(m_userName),
                                        EcpClient::quoteArg(m_pin)));
        }

        void setupChangeGroup()
        {
            if (!m_client->isConnected()) {
                return;
            }
            if (m_controls.isEmpty()) {
                setStatus(QStringLiteral("已连接，但尚未加载 XML"));
                return;
            }

            m_client->sendLine(QStringLiteral("cgd %1").arg(kChangeGroupId));
            m_client->sendLine(QStringLiteral("cgc %1").arg(kChangeGroupId));
            for (const NamedControl &c : m_controls) {
                m_client->sendLine(QStringLiteral("cga %1 %2")
                                       .arg(kChangeGroupId)
                                       .arg(EcpClient::quoteArg(c.id)));
            }
            m_client->sendLine(QStringLiteral("cgs %1 %2").arg(kChangeGroupId).arg(kPollPeriodMs));
            m_client->sendLine(QStringLiteral("cgi %1").arg(kChangeGroupId));
            m_client->sendLine(QStringLiteral("cgp %1").arg(kChangeGroupId));
            m_changeGroupReady = true;
            setStatus(QStringLiteral("已订阅 %1 个 Named Control").arg(m_controls.size()));
        }

        void applyControlValue(const QString &id,
                               const QString &stringRepr,
                               double value,
                               double position)
        {
            const int idx = findControlIndex(id);
            if (idx < 0) {
                return;
            }
            NamedControl &c = m_controls[idx];
            c.stringRepr = stringRepr;
            c.position = position;

            const QString t = c.type.toLower();
            if (t == QLatin1String("boolean") || t == QLatin1String("trigger")) {
                c.value = (value != 0.0) || (position >= 0.5);
            } else if (t == QLatin1String("string") || t == QLatin1String("enum")) {
                c.value = stringRepr;
            } else if (t == QLatin1String("integer")) {
                c.value = static_cast<int>(value);
            } else {
                c.value = value;
            }

            publishBindingOutput(idx);
            updateListItemText(idx);
        }

        void updateListItemText(int index)
        {
            if (!widget || index < 0 || index >= m_controls.size()) {
                return;
            }
            const NamedControl &c = m_controls[index];
            for (int row = 0; row < widget->controlList->count(); ++row) {
                QListWidgetItem *item = widget->controlList->item(row);
                if (item->data(kRoleIndex).toInt() != index) {
                    continue;
                }
                const QString valueText = c.stringRepr.isEmpty()
                    ? (c.value.isValid() ? c.value.toString() : QStringLiteral("—"))
                    : c.stringRepr;
                item->setText(QStringLiteral("[%1] %2  (%3/%4)  %5")
                                  .arg(index)
                                  .arg(c.id)
                                  .arg(c.type, c.mode)
                                  .arg(valueText));
                break;
            }
        }

        int findControlIndex(const QString &id) const
        {
            for (int i = 0; i < m_controls.size(); ++i) {
                if (m_controls[i].id == id) {
                    return i;
                }
            }
            return -1;
        }

        void writeControl(int index, const QVariant &raw)
        {
            if (index < 0 || index >= m_controls.size()) {
                return;
            }
            if (!m_client->isConnected()) {
                setStatus(QStringLiteral("未连接，无法写入"));
                return;
            }

            const NamedControl &c = m_controls[index];
            if (c.mode.compare(QLatin1String("R"), Qt::CaseInsensitive) == 0) {
                setStatus(QStringLiteral("%1 为只读").arg(c.id));
                return;
            }

            const QString id = EcpClient::quoteArg(c.id);
            const QString t = c.type.toLower();
            QString line;

            if (t == QLatin1String("trigger")) {
                if (!toBool(raw)) {
                    return;
                }
                line = QStringLiteral("ct %1").arg(id);
            } else if (t == QLatin1String("boolean")) {
                line = QStringLiteral("csv %1 %2").arg(id).arg(toBool(raw) ? 1 : 0);
            } else if (t == QLatin1String("string") || t == QLatin1String("enum")) {
                line = QStringLiteral("css %1 %2").arg(id, EcpClient::quoteArg(raw.toString()));
            } else {
                bool ok = false;
                double d = raw.toDouble(&ok);
                if (!ok) {
                    d = raw.toString().toDouble(&ok);
                }
                if (!ok) {
                    setStatus(QStringLiteral("无法将输入转为数值: %1").arg(c.id));
                    return;
                }
                line = QStringLiteral("csv %1 %2").arg(id).arg(d, 0, 'g', 12);
            }

            if (!m_client->sendLine(line)) {
                setStatus(QStringLiteral("发送失败"));
                return;
            }
            setStatus(QStringLiteral("已写入 [%1] %2").arg(index).arg(c.id));
        }

        void ensureCaches()
        {
            const int outN = static_cast<int>(OutPortCount);
            if (m_outValues.size() < outN) {
                const int old = m_outValues.size();
                m_outValues.resize(outN);
                for (int i = old; i < outN; ++i) {
                    m_outValues[i] = std::make_shared<VariableData>(QVariant());
                }
            } else if (m_outValues.size() > outN) {
                m_outValues.resize(outN);
            }
        }

        void publishAllOutputs()
        {
            ensureCaches();
            const int n = static_cast<int>(OutPortCount);
            for (int i = 0; i < n; ++i) {
                const QVariant val = (i < m_controls.size()) ? m_controls[i].value : QVariant();
                m_outValues[i] = std::make_shared<VariableData>(val);
                Q_EMIT dataUpdated(static_cast<PortIndex>(i));
            }
        }

        void publishBindingOutput(int index)
        {
            if (index < 0 || index >= static_cast<int>(OutPortCount)) {
                return;
            }
            ensureCaches();
            m_outValues[index] = std::make_shared<VariableData>(m_controls[index].value);
            Q_EMIT dataUpdated(static_cast<PortIndex>(index));
        }

        static bool toBool(const QVariant &raw)
        {
            if (raw.typeId() == QMetaType::Bool) {
                return raw.toBool();
            }
            if (raw.canConvert<int>()) {
                return raw.toInt() != 0;
            }
            const QString s = raw.toString().trimmed().toLower();
            return s == QLatin1String("true") || s == QLatin1String("1")
                || s == QLatin1String("on") || s == QLatin1String("muted");
        }

        QPointer<QSysQrcInterface> widget = new QSysQrcInterface();
        EcpClient *m_client = nullptr;

        QString m_host;
        QString m_userName;
        QString m_pin;
        QString m_xmlPath;
        bool m_connected = false;
        bool m_changeGroupReady = false;
        bool m_shuttingDown = false;

        QVector<NamedControl> m_controls;
        QVector<std::shared_ptr<VariableData>> m_outValues;
    };
}
