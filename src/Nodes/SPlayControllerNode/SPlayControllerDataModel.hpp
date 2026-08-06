/**
 * @file SPlayControllerDataModel.hpp
 * @brief ENTTEC S-Play 多 playlist 并发控制节点
 *
 * - 每 1 秒 POST {"command":8} GET_ALL_PLAYLISTS（列表 + 保活）
 * - 默认各 1 个 In/Out；PortEditable 由用户手动改数量，与 playlist 数量无关
 * - 端口 i 控制/反映列表排序号 i（caption 为 [i] name）；不因脚本/列表变化改端口或拆线
 *
 * API：https://github.com/ENTTEC/SplayApi
 */
#pragma once

#include <algorithm>

#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QQueue>
#include <QSignalBlocker>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <QWidget>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "PluginDefinition.hpp"
#include "SplayHttpClient.h"
#include "SPlayControllerInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class SPlayControllerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kPollIntervalMs = 1000;
        static constexpr int kPlaylistStatusPlaying = 1;

        static constexpr int kRolePlaylistId = Qt::UserRole;
        static constexpr int kRoleIndex = Qt::UserRole + 1;
        static constexpr int kRoleStatus = Qt::UserRole + 2;

        Q_PROPERTY(QString hostAddress READ hostAddress WRITE setHostAddress NOTIFY hostAddressChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        struct PlaylistInfo
        {
            int playlistId = -1;
            QString name;
            int status = 0;
            int order = 0;
            double currentTime = 0;
            double duration = 0;
        };

        SPlayControllerDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;

            m_desiredPlaying = {false};
            m_statusOutputs = {std::make_shared<VariableData>(QVariant(false))};

            m_client = new SplayHttpClient(this);
            connect(m_client, &SplayHttpClient::jsonReady,
                    this, &SPlayControllerDataModel::onJsonReady);
            connect(m_client, &SplayHttpClient::requestFailed,
                    this, &SPlayControllerDataModel::onRequestFailed);

            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(kPollIntervalMs);
            connect(m_pollTimer, &QTimer::timeout, this, &SPlayControllerDataModel::fetchPlaylists);

            syncParametersFromWidget();
            widget->updateConnectionStatus(false);

            connectUiSignals();
            registerOscBindings();
        }

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            const int i = static_cast<int>(portIndex);
            if (i < 0) {
                return {};
            }
            if (i < m_playlists.size()) {
                const auto &pl = m_playlists[i];
                return QStringLiteral("[%1] %2").arg(i).arg(pl.name);
            }
            return QStringLiteral("[%1] —").arg(i);
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            const int i = static_cast<int>(portIndex);
            if (i < 0 || i >= static_cast<int>(OutPortCount)) {
                return nullptr;
            }
            if (i >= m_statusOutputs.size()) {
                m_statusOutputs.resize(static_cast<int>(OutPortCount));
            }
            if (!m_statusOutputs[i]) {
                const bool playing = (i < m_playlists.size())
                    && (m_playlists[i].status == kPlaylistStatusPlaying);
                m_statusOutputs[i] = std::make_shared<VariableData>(QVariant(playing));
            }
            return m_statusOutputs[i];
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }
            const int i = static_cast<int>(portIndex);
            if (i < 0 || i >= static_cast<int>(InPortCount)) {
                return;
            }
            ensureChannelCaches();
            // 端口每次写入都发令：true=PLAY，false=STOP（不因本地已勾选而吞掉）
            setDesiredPlaying(i, toBool(*var), true, false);
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
            values[QStringLiteral("hostAddress")] = m_hostAddress;

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

            setHostAddress(values.value(QStringLiteral("hostAddress")).toString(m_hostAddress));

            // 仅对齐内部缓存到当前（框架已恢复的）端口数，绝不改 In/OutPortCount
            ensureChannelCaches();
        }

        QWidget *embeddedWidget() override { return widget; }

        QString hostAddress() const { return m_hostAddress; }
        bool connected() const { return m_connected; }

        void setHostAddress(const QString &value)
        {
            if (m_hostAddress == value) {
                return;
            }
            m_hostAddress = value;
            {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(value);
            }
            // 换主机时丢掉旧请求，避免连错地址的失败回调/读未打开设备告警
            if (m_client) {
                m_client->abortAll();
            }
            m_pendingPoll = false;
            m_commandBusy = false;
            m_pollFailStreak = 0;
            m_commandQueue.clear();
            Q_EMIT hostAddressChanged(value);
        }

    Q_SIGNALS:
        void hostAddressChanged(const QString &value);
        void connectedChanged(bool connected);

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress(QLatin1String("/host")),
                           this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QLatin1String("/playAll")),
                           this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QLatin1String("/stopAll")),
                           this, SLOT(onGlobalEvent(GlobalEvent)));

            m_pollTimer->start();
            QTimer::singleShot(0, this, &SPlayControllerDataModel::fetchPlaylists);
        }

    private slots:
        void fetchPlaylists()
        {
            if (m_pendingPoll) {
                return;
            }
            syncParametersFromWidget();
            const QUrl url = buildApiUrl();
            if (!url.isValid() || url.host().isEmpty()) {
                setConnected(false);
                setStatus(QStringLiteral("Invalid host"));
                return;
            }

            QJsonObject body{{QStringLiteral("command"), 8}};
            m_pendingPoll = true;
            if (!m_connected) {
                setStatus(QStringLiteral("正在连接 %1 ...").arg(url.toString()));
            }
            m_client->postJson(url, body, QStringLiteral("poll"));
        }

        void playAllPlaylists()
        {
            enqueueCommand(QJsonObject{{QStringLiteral("command"), 5}},
                           QStringLiteral("playAll"),
                           -1);
            AbstractDelegateModel::stateFeedBack(QStringLiteral("/playAll"), true);
            for (int i = 0; i < m_desiredPlaying.size(); ++i) {
                m_desiredPlaying[i] = true;
            }
            syncListCheckboxesFromDesired();
            AbstractDelegateModel::stateFeedBack(QStringLiteral("/playAll"), false);
        }

        void stopAllPlaylists()
        {
            enqueueCommand(QJsonObject{{QStringLiteral("command"), 7}},
                           QStringLiteral("stopAll"),
                           -1);
            AbstractDelegateModel::stateFeedBack(QStringLiteral("/stopAll"), true);
            for (int i = 0; i < m_desiredPlaying.size(); ++i) {
                m_desiredPlaying[i] = false;
            }
            syncListCheckboxesFromDesired();
            AbstractDelegateModel::stateFeedBack(QStringLiteral("/stopAll"), false);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addr = ev.address;
            if (addr == makeFullOscAddress("/host")) {
                setHostAddress(ev.payload.toString());
                fetchPlaylists();
            } else if (addr == makeFullOscAddress("/playAll") && ev.payload.toBool()) {
                playAllPlaylists();
            } else if (addr == makeFullOscAddress("/stopAll") && ev.payload.toBool()) {
                stopAllPlaylists();
            }
        }

        void onJsonReady(const QJsonObject &json, const QVariant &context)
        {
            const QString kind = context.toString();
            if (kind == QLatin1String("poll")) {
                m_pendingPoll = false;
                m_pollFailStreak = 0;
                const bool wasConnected = m_connected;
                const int prevCount = m_playlists.size();
                if (!wasConnected) {
                    setConnected(true);
                }
                updatePlaylistsFromJson(json);
                if (!wasConnected || prevCount != m_playlists.size()) {
                    setStatus(QStringLiteral("已加载 %1 个 playlist").arg(m_playlists.size()));
                }
                return;
            }

            m_commandBusy = false;
            if (kind == QLatin1String("play") || kind == QLatin1String("stop")
                || kind == QLatin1String("playAll") || kind == QLatin1String("stopAll")) {
                if (kind == QLatin1String("playAll")) {
                    setStatus(QStringLiteral("已发送全部播放"));
                } else if (kind == QLatin1String("stopAll")) {
                    setStatus(QStringLiteral("已发送全停"));
                } else {
                    setStatus(QStringLiteral("命令成功: %1").arg(kind));
                }
                QTimer::singleShot(200, this, &SPlayControllerDataModel::fetchPlaylists);
            }
            flushCommandQueue();
        }

        void onRequestFailed(const QString &message, int, const QVariant &context)
        {
            const QString kind = context.toString();
            if (kind == QLatin1String("poll")) {
                m_pendingPoll = false;
                ++m_pollFailStreak;
                // 已连接时单次迟到超时不立刻断开，避免「先已连接再被旧请求打成未连接」
                if (!m_connected || m_pollFailStreak >= 2) {
                    setConnected(false);
                    setStatus(QStringLiteral("连接失败: %1").arg(message));
                } else {
                    setStatus(QStringLiteral("轮询异常，稍后重试: %1").arg(message));
                }
                return;
            }

            m_commandBusy = false;
            setStatus(QStringLiteral("命令失败 (%1): %2").arg(kind, message));
            flushCommandQueue();
        }

    private:
        void connectUiSignals()
        {
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
                setHostAddress(widget->hostEdit->text().trimmed());
                fetchPlaylists();
            });
            connect(widget->playAllButton, &QPushButton::clicked,
                    this, &SPlayControllerDataModel::playAllPlaylists);
            connect(widget->stopAllButton, &QPushButton::clicked,
                    this, &SPlayControllerDataModel::stopAllPlaylists);
        }

        void registerOscBindings()
        {
            registerBinding("/host", "hostAddress", widget->hostEdit);
            registerBinding("/connected", "connected", widget->connectionLabel);
            registerBinding("/playAll", "trigger", widget->playAllButton);
            registerBinding("/stopAll", "trigger", widget->stopAllButton);
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
            m_hostAddress = widget->hostEdit->text().trimmed();
        }

        void setStatus(const QString &text)
        {
            widget->statusLabel->setText(QStringLiteral("状态: %1").arg(text));
        }

        void setConnected(bool connected)
        {
            if (m_connected == connected) {
                return;
            }
            m_connected = connected;
            widget->updateConnectionStatus(m_connected);
            Q_EMIT connectedChanged(m_connected);
        }

        QUrl buildApiUrl() const
        {
            // 手动解析，避免把 "127.0.0.1:8080" 整段 setHost 导致连不上
            // 支持：IP | IP:port | http://IP | http://IP:port[/...]
            QString raw = m_hostAddress.trimmed();
            if (raw.isEmpty()) {
                raw = QStringLiteral("127.0.0.1");
            }

            if (raw.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)) {
                raw = raw.mid(7);
            } else if (raw.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
                raw = raw.mid(8);
            }

            const int slash = raw.indexOf(QLatin1Char('/'));
            if (slash >= 0) {
                raw = raw.left(slash);
            }

            QString host = raw;
            int port = 80;
            const int colon = raw.lastIndexOf(QLatin1Char(':'));
            if (colon > 0) {
                bool ok = false;
                const int parsed = raw.mid(colon + 1).toInt(&ok);
                if (ok && parsed > 0 && parsed <= 65535) {
                    host = raw.left(colon);
                    port = parsed;
                }
            }

            // 去掉可能残留的方括号（IPv6 字面量暂不特殊处理）
            if (host.startsWith(QLatin1Char('[')) && host.endsWith(QLatin1Char(']'))) {
                host = host.mid(1, host.size() - 2);
            }

            QUrl url;
            url.setScheme(QStringLiteral("http"));
            url.setHost(host);
            url.setPort(port);
            url.setPath(QStringLiteral("/api"));
            return url;
        }

        /** 仅对齐内部缓存到当前 In/OutPortCount，绝不修改端口数量或发 ports* 信号 */
        void ensureChannelCaches()
        {
            const int inN = static_cast<int>(InPortCount);
            const int outN = static_cast<int>(OutPortCount);
            if (m_desiredPlaying.size() < inN) {
                m_desiredPlaying.resize(inN);
            }
            if (m_statusOutputs.size() < outN) {
                const int old = m_statusOutputs.size();
                m_statusOutputs.resize(outN);
                for (int i = old; i < outN; ++i) {
                    m_statusOutputs[i] = std::make_shared<VariableData>(QVariant(false));
                }
            }
        }

        void setDesiredPlaying(int index, bool playing, bool sendCommand, bool requireChange = true)
        {
            if (index < 0) {
                return;
            }
            ensureDesiredSize(index + 1);
            const bool changed = (m_desiredPlaying[index] != playing);
            m_desiredPlaying[index] = playing;
            // 乐观更新勾选；下一轮轮询会用设备真实 status 校正
            syncListCheckbox(index, playing);

            if (!sendCommand) {
                return;
            }
            // 勾选框：仅变化时发令；输入端口/OSC：每次写入都发令，便于 Trigger 再次 true 重播
            if (requireChange && !changed) {
                return;
            }

            if (index >= m_playlists.size() || m_playlists[index].playlistId < 0) {
                setStatus(QStringLiteral("序号 %1 无对应 playlist").arg(index));
                return;
            }

            const int playlistId = m_playlists[index].playlistId;
            QJsonObject body;
            body.insert(QStringLiteral("command"), playing ? 0 : 2);
            body.insert(QStringLiteral("playlist_id"), playlistId);
            enqueueCommand(body, playing ? QStringLiteral("play") : QStringLiteral("stop"), index);
        }

        void ensureDesiredSize(int size)
        {
            if (m_desiredPlaying.size() < size) {
                m_desiredPlaying.resize(size);
            }
        }

        void enqueueCommand(const QJsonObject &body, const QString &kind, int index)
        {
            PendingCommand cmd;
            cmd.body = body;
            cmd.kind = kind;
            cmd.index = index;
            m_commandQueue.enqueue(cmd);
            flushCommandQueue();
        }

        void flushCommandQueue()
        {
            if (m_commandBusy || m_commandQueue.isEmpty()) {
                return;
            }
            syncParametersFromWidget();
            const QUrl url = buildApiUrl();
            if (!url.isValid()) {
                m_commandQueue.clear();
                setStatus(QStringLiteral("Invalid URL"));
                return;
            }

            const PendingCommand cmd = m_commandQueue.dequeue();
            m_commandBusy = true;
            setStatus(QStringLiteral("发送 %1...").arg(cmd.kind));
            m_client->postJson(url, cmd.body, cmd.kind);
        }

        void updatePlaylistsFromJson(const QJsonObject &json)
        {
            QVector<PlaylistInfo> items;
            const QJsonArray arr = json.value(QStringLiteral("playlists")).toArray();
            items.reserve(arr.size());
            for (const QJsonValue &val : arr) {
                if (!val.isObject()) {
                    continue;
                }
                const QJsonObject obj = val.toObject();
                PlaylistInfo info;
                info.playlistId = obj.value(QStringLiteral("playlist_id")).toInt(-1);
                info.name = obj.value(QStringLiteral("name")).toString();
                if (info.name.isEmpty()) {
                    info.name = QStringLiteral("Playlist %1").arg(info.playlistId);
                }
                info.status = obj.value(QStringLiteral("status")).toInt(0);
                info.order = obj.value(QStringLiteral("order")).toInt(0);
                info.currentTime = obj.value(QStringLiteral("current_time")).toDouble(0);
                info.duration = obj.value(QStringLiteral("duration")).toDouble(0);
                if (info.playlistId >= 0) {
                    items.push_back(info);
                }
            }

            std::sort(items.begin(), items.end(), [](const PlaylistInfo &a, const PlaylistInfo &b) {
                if (a.order != b.order) {
                    return a.order < b.order;
                }
                return a.playlistId < b.playlistId;
            });

            m_playlists = items;
            rebuildPlaylistWidget();
            publishAllStatusOutputs();
        }

        void rebuildPlaylistWidget()
        {
            QSignalBlocker blocker(widget->playlistWidget);
            widget->playlistWidget->clear();

            for (int i = 0; i < m_playlists.size(); ++i) {
                const auto &pl = m_playlists[i];
                auto *item = new QListWidgetItem(widget->playlistWidget);
                item->setData(kRolePlaylistId, pl.playlistId);
                item->setData(kRoleIndex, i);
                item->setData(kRoleStatus, pl.status);

                auto *row = new QWidget(widget->playlistWidget);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(4, 2, 4, 2);
                rowLayout->setSpacing(6);

                auto *check = new QCheckBox(row);
                check->setProperty("playlistIndex", i);
                ensureDesiredSize(i + 1);
                // 勾选与设备 status 一致（Playing=勾选），轮询校正
                const bool playing = (pl.status == kPlaylistStatusPlaying);
                m_desiredPlaying[i] = playing;
                {
                    QSignalBlocker b(check);
                    check->setChecked(playing);
                }
                connect(check, &QCheckBox::toggled, this, [this, i](bool checked) {
                    setDesiredPlaying(i, checked, true, true);
                });

                auto *label = new QLabel(
                    QStringLiteral("[%1] %2").arg(i).arg(pl.name), row);
                label->setToolTip(
                    QStringLiteral("index=%1\nplaylist_id=%2\nstatus=%3\n%4 / %5 ms")
                        .arg(i)
                        .arg(pl.playlistId)
                        .arg(pl.status)
                        .arg(pl.currentTime)
                        .arg(pl.duration));

                rowLayout->addWidget(check);
                rowLayout->addWidget(label, 1);
                row->setLayout(rowLayout);

                item->setSizeHint(row->sizeHint());
                widget->playlistWidget->addItem(item);
                widget->playlistWidget->setItemWidget(item, row);

                item->setBackground(playing ? QColor(60, 100, 160, 80) : QBrush());
            }
        }

        void syncListCheckboxesFromDesired()
        {
            for (int i = 0; i < widget->playlistWidget->count(); ++i) {
                syncListCheckbox(i, i < m_desiredPlaying.size() ? m_desiredPlaying[i] : false);
            }
        }

        void syncListCheckbox(int index, bool checked)
        {
            if (index < 0 || index >= widget->playlistWidget->count()) {
                return;
            }
            auto *item = widget->playlistWidget->item(index);
            auto *row = widget->playlistWidget->itemWidget(item);
            if (!row) {
                return;
            }
            auto *check = row->findChild<QCheckBox *>();
            if (!check) {
                return;
            }
            QSignalBlocker b(check);
            check->setChecked(checked);
        }

        void publishAllStatusOutputs()
        {
            const int n = static_cast<int>(OutPortCount);
            for (int i = 0; i < n; ++i) {
                const bool playing = (i < m_playlists.size())
                    && (m_playlists[i].status == kPlaylistStatusPlaying);
                if (i >= m_statusOutputs.size()) {
                    m_statusOutputs.resize(i + 1);
                }
                m_statusOutputs[i] = std::make_shared<VariableData>(QVariant(playing));
                Q_EMIT dataUpdated(static_cast<PortIndex>(i));
            }
        }

        static bool toBool(const VariableData &data)
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

        struct PendingCommand
        {
            QJsonObject body;
            QString kind;
            int index = -1;
        };

        SPlayControllerInterface *widget = new SPlayControllerInterface();
        SplayHttpClient *m_client = nullptr;
        QTimer *m_pollTimer = nullptr;

        QString m_hostAddress;
        bool m_connected = false;
        bool m_pendingPoll = false;
        bool m_commandBusy = false;
        int m_pollFailStreak = 0;

        QVector<PlaylistInfo> m_playlists;
        QVector<bool> m_desiredPlaying;
        QVector<std::shared_ptr<VariableData>> m_statusOutputs;
        QQueue<PendingCommand> m_commandQueue;
    };
}
