/**
 * @file VlcRemoteDataModel.hpp
 * @brief VLC Remote 节点（交互逻辑对齐 Mpv Controller）
 *
 * - 每 5 秒 GET playlist.json（查询列表 + 保活）
 * - 切换：status.json?command=pl_play&id={playlistId}（由排序号取 ID）
 * - 停止：status.json?command=pl_stop
 *
 * 切换入口：
 * - PLAY(true) / 「播放」 → 按 Index 控件值切换并同步控件
 * - INDEX 端口 → 整数排序号，同步 Index 并播放
 * - 双击列表项 → 按该项切换，不改 Index 控件
 * - STOP 端口 / 「停止」 / OSC /stop(true) → 停止播放，不改 Index
 *
 * 端口：PLAY / INDEX / STOP → DONE / CONNECTED
 * 密码不进属性系统（仅界面 + 本地存盘）
 *
 * VLC：vlc --extraintf http --http-password xxx --http-port 8080
 */
#pragma once

#include <QBrush>
#include <QColor>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidget>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "PluginDefinition.hpp"
#include "VlcHttpClient.h"
#include "VlcRemoteInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class VlcRemoteDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kPollIntervalMs = 5000;

        /** 列表项：UserRole = VLC playlistId，UserRole+1 = 排序号 */
        static constexpr int kRolePlaylistId = Qt::UserRole;
        static constexpr int kRoleIndex = Qt::UserRole + 1;

        enum InputPort : PortIndex { PlayPort = 0, IndexPort = 1, StopPort = 2 };
        enum OutputPort : PortIndex { DonePort = 0, ConnectedPort = 1 };

        // 密码故意不声明 Q_PROPERTY
        Q_PROPERTY(QString hostAddress READ hostAddress WRITE setHostAddress NOTIFY hostAddressChanged)
        Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        VlcRemoteDataModel()
        {
            InPortCount = 3;
            OutPortCount = 2;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = true;
            PortEditable = false;

            m_doneOutput = std::make_shared<VariableData>(QVariant(false));
            m_connectedOutput = std::make_shared<VariableData>(QVariant(false));

            m_statusClient = new VlcHttpClient(this);
            m_playlistClient = new VlcHttpClient(this);

            connect(m_statusClient, &VlcHttpClient::jsonReady,
                    this, &VlcRemoteDataModel::onStatusResponse);
            connect(m_statusClient, &VlcHttpClient::requestFailed,
                    this, &VlcRemoteDataModel::onStatusFailed);
            connect(m_playlistClient, &VlcHttpClient::jsonReady,
                    this, &VlcRemoteDataModel::onPlaylistResponse);
            connect(m_playlistClient, &VlcHttpClient::requestFailed,
                    this, &VlcRemoteDataModel::onPlaylistFailed);

            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(kPollIntervalMs);
            connect(m_pollTimer, &QTimer::timeout, this, &VlcRemoteDataModel::fetchPlaylists);

            syncParametersFromWidget();
            m_index = widget->indexSpinBox->value();
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
            if (portType == PortType::In) {
                static const char *const kIn[] = {"PLAY", "INDEX", "STOP"};
                return (portIndex >= 0 && portIndex < 3) ? QString::fromLatin1(kIn[portIndex]) : QString();
            }
            if (portType == PortType::Out) {
                static const char *const kOut[] = {"DONE", "CONNECTED"};
                return (portIndex >= 0 && portIndex < 2) ? QString::fromLatin1(kOut[portIndex]) : QString();
            }
            return {};
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            if (portIndex == DonePort) {
                return m_doneOutput;
            }
            if (portIndex == ConnectedPort) {
                return m_connectedOutput;
            }
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }

            if (portIndex == PlayPort) {
                if (isTriggerTrue(*var)) {
                    playByIndex(widget->indexSpinBox->value(), true);
                }
            } else if (portIndex == IndexPort) {
                setIndex(extractIndexFromData(var));
            } else if (portIndex == StopPort) {
                if (isTriggerTrue(*var)) {
                    stopPlayback();
                }
            }
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
            values[QStringLiteral("port")] = m_port;
            values[QStringLiteral("password")] = m_password;
            values[QStringLiteral("index")] = m_index;

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
            setPort(values.value(QStringLiteral("port")).toInt(m_port));

            m_password = values.value(QStringLiteral("password")).toString(m_password);
            {
                QSignalBlocker blocker(widget->passwordEdit);
                widget->passwordEdit->setText(m_password);
            }

            if (values.contains(QStringLiteral("index"))) {
                applyIndexToControl(values.value(QStringLiteral("index")).toInt());
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        QString hostAddress() const { return m_hostAddress; }
        int port() const { return m_port; }
        int index() const { return m_index; }
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
            Q_EMIT hostAddressChanged(value);
        }

        void setPort(int value)
        {
            value = qBound(1, value, 65535);
            if (m_port == value) {
                return;
            }
            m_port = value;
            {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(value);
            }
            Q_EMIT portChanged(value);
        }

        void setIndex(int index)
        {
            if (index < 0) {
                return;
            }
            playByIndex(index, true);
        }

    Q_SIGNALS:
        void hostAddressChanged(const QString &value);
        void portChanged(int value);
        void indexChanged(int index);
        void connectedChanged(bool connected);

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            for (const char *path : {"/host", "/port", "/index", "/play", "/stop"}) {
                bus->subscribe(makeFullOscAddress(QLatin1String(path)),
                               this, SLOT(onGlobalEvent(GlobalEvent)));
            }
            m_pollTimer->start();
            QTimer::singleShot(0, this, &VlcRemoteDataModel::fetchPlaylists);
        }

    private slots:
        /** 拉取 playlist.json：刷新列表 + 保活 */
        void fetchPlaylists()
        {
            if (m_pendingPlaylist) {
                return;
            }
            syncParametersFromWidget();
            const QUrl url = buildUrl(QStringLiteral("playlist.json"), {});
            if (!url.isValid() || url.host().isEmpty()) {
                setConnected(false);
                setStatus(QStringLiteral("Invalid host"));
                return;
            }
            m_pendingPlaylist = true;
            if (!m_connected) {
                setStatus(QStringLiteral("正在连接..."));
            }
            m_playlistClient->sendGet(url, m_password);
        }

        /** 双击：按该项播放，不改 Index */
        void onPlaylistItemPlay(QListWidgetItem *item)
        {
            if (!item) {
                return;
            }
            playByIndex(item->data(kRoleIndex).toInt(), false);
        }

        /**
         * 按排序号切换播放
         * @param updateIndexControl true 时同步 Index 控件；双击传 false
         */
        void playByIndex(int index, bool updateIndexControl = true)
        {
            if (m_pendingPlay || m_pendingStop) {
                m_resendPlay = true;
                m_resendStop = false;
                if (index >= 0) {
                    m_pendingIndex = index;
                    m_pendingUpdateIndex = updateIndexControl;
                    if (updateIndexControl) {
                        applyIndexToControl(index);
                    }
                }
                setStatus(QStringLiteral("Queued..."));
                return;
            }

            syncParametersFromWidget();
            if (index < 0) {
                setStatus(QStringLiteral("无效的播放列表序号"));
                publishDone(false);
                return;
            }

            if (updateIndexControl) {
                applyIndexToControl(index);
            }

            const QString playlistId = playlistIdAtIndex(index);
            if (playlistId.isEmpty()) {
                setStatus(QStringLiteral("序号 %1 无对应播放项").arg(index));
                publishDone(false);
                return;
            }

            QUrlQuery query;
            query.addQueryItem(QStringLiteral("command"), QStringLiteral("pl_play"));
            query.addQueryItem(QStringLiteral("id"), playlistId);

            const QUrl url = buildUrl(QStringLiteral("status.json"), query);
            if (!url.isValid()) {
                setStatus(QStringLiteral("Invalid URL"));
                publishDone(false);
                return;
            }

            m_pendingPlay = true;
            m_pendingPlayId = playlistId;
            m_pendingPlayIndex = index;
            m_pendingIndex = -1;
            m_pendingUpdateIndex = true;
            setStatus(QStringLiteral("切换 [%1] id=%2...").arg(index).arg(playlistId));
            AbstractDelegateModel::stateFeedBack("/play", true);
            m_statusClient->sendGet(url, m_password);
        }

        /** 停止当前播放；不修改 Index */
        void stopPlayback()
        {
            if (m_pendingPlay || m_pendingStop) {
                m_resendStop = true;
                m_resendPlay = false;
                setStatus(QStringLiteral("Queued stop..."));
                return;
            }

            syncParametersFromWidget();
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("command"), QStringLiteral("pl_stop"));

            const QUrl url = buildUrl(QStringLiteral("status.json"), query);
            if (!url.isValid()) {
                setStatus(QStringLiteral("Invalid URL"));
                publishDone(false);
                return;
            }

            m_pendingStop = true;
            setStatus(QStringLiteral("正在停止..."));
            AbstractDelegateModel::stateFeedBack("/stop", true);
            m_statusClient->sendGet(url, m_password);
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
            } else if (addr == makeFullOscAddress("/port")) {
                setPort(ev.payload.toInt());
                fetchPlaylists();
            } else if (addr == makeFullOscAddress("/index")) {
                setIndex(ev.payload.toInt());
            } else if (addr == makeFullOscAddress("/play") && ev.payload.toBool()) {
                playByIndex(widget->indexSpinBox->value(), true);
            } else if (addr == makeFullOscAddress("/stop") && ev.payload.toBool()) {
                stopPlayback();
            }
        }

        void onPlaylistResponse(const QJsonObject &json)
        {
            m_pendingPlaylist = false;
            setConnected(true);
            updatePlaylistView(json);
            setStatus(QStringLiteral("已加载 %1 个播放项").arg(widget->playlistWidget->count()));
        }

        void onPlaylistFailed(const QString &message, int)
        {
            m_pendingPlaylist = false;
            setConnected(false);
            setStatus(QStringLiteral("连接失败: %1").arg(message));
        }

        void onStatusResponse(const QJsonObject &json)
        {
            if (m_pendingStop) {
                m_pendingStop = false;
                AbstractDelegateModel::stateFeedBack("/stop", false);
                m_playingPlaylistId.clear();
                syncPlayingHighlight();
                publishDone(true);
                setStatus(QStringLiteral("已停止"));
                flushQueuedControl();
                return;
            }

            if (m_pendingPlay) {
                m_pendingPlay = false;
                AbstractDelegateModel::stateFeedBack("/play", false);
                m_playingPlaylistId = m_pendingPlayId;
                syncPlayingHighlight();
                publishDone(true);
                setStatus(QStringLiteral("正在播放 [%1] id=%2")
                              .arg(m_pendingPlayIndex)
                              .arg(m_playingPlaylistId));
                QTimer::singleShot(300, this, &VlcRemoteDataModel::fetchPlaylists);
                flushQueuedControl();
                return;
            }

            // 保活/状态回显：更新当前播放高亮
            const QString currentId = json.value(QStringLiteral("currentplid")).toVariant().toString();
            if (!currentId.isEmpty() && currentId != QStringLiteral("-1")) {
                m_playingPlaylistId = currentId;
                syncPlayingHighlight();
            }
            setConnected(true);
        }

        void onStatusFailed(const QString &message, int)
        {
            if (m_pendingStop) {
                m_pendingStop = false;
                AbstractDelegateModel::stateFeedBack("/stop", false);
                publishDone(false);
                setStatus(QStringLiteral("停止失败: %1").arg(message));
                flushQueuedControl();
                return;
            }
            if (m_pendingPlay) {
                m_pendingPlay = false;
                AbstractDelegateModel::stateFeedBack("/play", false);
                publishDone(false);
                setStatus(QStringLiteral("切换失败: %1").arg(message));
                flushQueuedControl();
            }
        }

    private:
        void connectUiSignals()
        {
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
                setHostAddress(widget->hostEdit->text().trimmed());
                fetchPlaylists();
            });
            connect(widget->portSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int value) {
                        setPort(value);
                        fetchPlaylists();
                    });
            connect(widget->passwordEdit, &QLineEdit::editingFinished, this, [this]() {
                m_password = widget->passwordEdit->text();
                fetchPlaylists();
            });
            connect(widget->indexSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int value) {
                        if (m_index == value) {
                            return;
                        }
                        m_index = value;
                        Q_EMIT indexChanged(m_index);
                    });
            connect(widget->playButton, &QPushButton::clicked, this, [this]() {
                setIndex(widget->indexSpinBox->value());
            });
            connect(widget->stopButton, &QPushButton::clicked, this, [this]() {
                stopPlayback();
            });
            connect(widget->playlistWidget, &QListWidget::itemDoubleClicked,
                    this, &VlcRemoteDataModel::onPlaylistItemPlay);
        }

        void registerOscBindings()
        {
            registerBinding("/host", "hostAddress", widget->hostEdit);
            registerBinding("/port", "port", widget->portSpinBox);
            registerBinding("/index", "index", widget->indexSpinBox);
            registerBinding("/connected", "connected", widget->connectionLabel);
            registerBinding("/play", "trigger", nullptr);
            registerBinding("/stop", "trigger", nullptr);
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
            m_port = widget->portSpinBox->value();
            m_password = widget->passwordEdit->text();
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
            m_connectedOutput = std::make_shared<VariableData>(QVariant(m_connected));
            Q_EMIT dataUpdated(ConnectedPort);
            Q_EMIT connectedChanged(m_connected);
        }

        void publishDone(bool success)
        {
            m_doneOutput = std::make_shared<VariableData>(QVariant(success));
            Q_EMIT dataUpdated(DonePort);
        }

        void applyIndexToControl(int index)
        {
            if (m_index != index) {
                m_index = index;
                Q_EMIT indexChanged(m_index);
            }
            QSignalBlocker blocker(widget->indexSpinBox);
            if (widget->indexSpinBox->value() != index) {
                widget->indexSpinBox->setValue(index);
            }
        }

        void syncPlayingHighlight()
        {
            QSignalBlocker blocker(widget->playlistWidget);
            QListWidgetItem *matched = nullptr;
            for (int i = 0; i < widget->playlistWidget->count(); ++i) {
                auto *item = widget->playlistWidget->item(i);
                const bool current = (item->data(kRolePlaylistId).toString() == m_playingPlaylistId);
                item->setBackground(current ? QColor(60, 100, 160, 80) : QBrush());
                if (current) {
                    matched = item;
                }
            }
            widget->playlistWidget->setCurrentItem(matched);
        }

        QString playlistIdAtIndex(int index) const
        {
            if (index < 0 || index >= widget->playlistWidget->count()) {
                return {};
            }
            auto *item = widget->playlistWidget->item(index);
            return item ? item->data(kRolePlaylistId).toString() : QString();
        }

        QUrl buildUrl(const QString &resource, const QUrlQuery &query) const
        {
            QUrl url;
            url.setScheme(QStringLiteral("http"));
            url.setHost(m_hostAddress.isEmpty() ? QStringLiteral("127.0.0.1") : m_hostAddress);
            url.setPort(m_port > 0 ? m_port : 8080);
            url.setPath(QStringLiteral("/requests/") + resource);
            if (!query.isEmpty()) {
                url.setQuery(query);
            }
            return url;
        }

        static int extractIndexFromData(const std::shared_ptr<VariableData> &data)
        {
            if (!data || data->isEmpty()) {
                return -1;
            }
            if (data->hasKey(QStringLiteral("index"))) {
                bool ok = false;
                const int index = data->value(QStringLiteral("index")).toInt(&ok);
                return ok ? index : -1;
            }
            if (!data->hasKey(QStringLiteral("default"))) {
                return -1;
            }
            const QVariant value = data->value(QStringLiteral("default"));
            if (value.typeId() == QMetaType::Bool) {
                return -1;
            }
            bool ok = false;
            const int index = value.toInt(&ok);
            if (!ok) {
                return -1;
            }
            const auto tid = value.typeId();
            if (tid == QMetaType::Int || tid == QMetaType::LongLong
                || tid == QMetaType::Double || tid == QMetaType::UInt) {
                return index;
            }
            if (tid == QMetaType::QString
                && value.toString().trimmed() == QString::number(index)) {
                return index;
            }
            return -1;
        }

        static bool isTriggerTrue(const VariableData &data)
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

        static void appendPlaylistLeaf(const QJsonObject &obj,
                                       QListWidget *listWidget,
                                       int &displayIndex,
                                       QString *currentPlayingId)
        {
            const QString type = obj.value(QStringLiteral("type")).toString();
            if (type == QLatin1String("node")) {
                return;
            }

            QString playlistId = obj.value(QStringLiteral("id")).toString();
            if (playlistId.isEmpty()) {
                playlistId = QString::number(obj.value(QStringLiteral("id")).toInt());
            }
            if (playlistId.isEmpty() || playlistId == QLatin1String("-1")) {
                return;
            }

            QString name = obj.value(QStringLiteral("name")).toString();
            const QString uri = obj.value(QStringLiteral("uri")).toString();
            if (name.isEmpty()) {
                name = uri;
            }
            if (name.isEmpty()) {
                name = QStringLiteral("Item %1").arg(playlistId);
            }

            const int index = displayIndex++;
            const QString text = QStringLiteral("[%1] %2").arg(index).arg(name);
            auto *item = new QListWidgetItem(text, listWidget);
            item->setData(kRolePlaylistId, playlistId);
            item->setData(kRoleIndex, index);
            item->setToolTip(QStringLiteral("index=%1\nid=%2\n%3").arg(index).arg(playlistId, uri));

            if (currentPlayingId
                && obj.value(QStringLiteral("current")).toString() == QLatin1String("current")) {
                *currentPlayingId = playlistId;
            }
        }

        static void collectPlaylistItems(const QJsonArray &children,
                                         QListWidget *listWidget,
                                         int &displayIndex,
                                         QString *currentPlayingId)
        {
            for (const QJsonValue &value : children) {
                if (!value.isObject()) {
                    continue;
                }
                const QJsonObject obj = value.toObject();
                if (obj.contains(QStringLiteral("children")) && obj[QStringLiteral("children")].isArray()) {
                    collectPlaylistItems(obj[QStringLiteral("children")].toArray(),
                                         listWidget, displayIndex, currentPlayingId);
                    continue;
                }
                appendPlaylistLeaf(obj, listWidget, displayIndex, currentPlayingId);
            }
        }

        void updatePlaylistView(const QJsonObject &json)
        {
            QSignalBlocker blocker(widget->playlistWidget);
            widget->playlistWidget->clear();

            int displayIndex = 0;
            QString currentId;
            if (json.contains(QStringLiteral("_playlistItems"))
                && json[QStringLiteral("_playlistItems")].isArray()) {
                for (const QJsonValue &value : json[QStringLiteral("_playlistItems")].toArray()) {
                    if (value.isObject()) {
                        appendPlaylistLeaf(value.toObject(), widget->playlistWidget,
                                           displayIndex, &currentId);
                    }
                }
            } else if (json.contains(QStringLiteral("children"))
                       && json[QStringLiteral("children")].isArray()) {
                collectPlaylistItems(json[QStringLiteral("children")].toArray(),
                                     widget->playlistWidget, displayIndex, &currentId);
            } else if (json.contains(QStringLiteral("root"))
                       && json[QStringLiteral("root")].isObject()) {
                const QJsonObject root = json[QStringLiteral("root")].toObject();
                if (root.contains(QStringLiteral("children"))
                    && root[QStringLiteral("children")].isArray()) {
                    collectPlaylistItems(root[QStringLiteral("children")].toArray(),
                                         widget->playlistWidget, displayIndex, &currentId);
                }
            }

            if (!currentId.isEmpty()) {
                m_playingPlaylistId = currentId;
            }

            const int count = widget->playlistWidget->count();
            {
                QSignalBlocker spinBlocker(widget->indexSpinBox);
                const int maxIndex = qMax(0, count - 1);
                widget->indexSpinBox->setMaximum(maxIndex);
                if (count > 0 && m_index > maxIndex) {
                    m_index = 0;
                    Q_EMIT indexChanged(m_index);
                }
                widget->indexSpinBox->setValue(m_index);
            }

            syncPlayingHighlight();
        }

        /** 播放/停止互斥排队：完成当前请求后执行最新意图 */
        void flushQueuedControl()
        {
            if (m_resendStop) {
                m_resendStop = false;
                m_resendPlay = false;
                QTimer::singleShot(0, this, &VlcRemoteDataModel::stopPlayback);
                return;
            }
            if (m_resendPlay) {
                m_resendPlay = false;
                const int index = (m_pendingIndex >= 0)
                    ? m_pendingIndex
                    : widget->indexSpinBox->value();
                const bool updateIndex = m_pendingUpdateIndex;
                m_pendingIndex = -1;
                m_pendingUpdateIndex = true;
                QTimer::singleShot(0, this, [this, index, updateIndex]() {
                    playByIndex(index, updateIndex);
                });
            }
        }

        VlcRemoteInterface *widget = new VlcRemoteInterface();
        VlcHttpClient *m_statusClient = nullptr;
        VlcHttpClient *m_playlistClient = nullptr;
        QTimer *m_pollTimer = nullptr;

        std::shared_ptr<VariableData> m_doneOutput;
        std::shared_ptr<VariableData> m_connectedOutput;

        QString m_hostAddress;
        int m_port = 8080;
        QString m_password;
        int m_index = 0;
        bool m_connected = false;

        bool m_pendingPlaylist = false;
        bool m_pendingPlay = false;
        bool m_pendingStop = false;
        bool m_resendPlay = false;
        bool m_resendStop = false;
        int m_pendingIndex = -1;
        bool m_pendingUpdateIndex = true;
        QString m_pendingPlayId;
        int m_pendingPlayIndex = -1;
        QString m_playingPlaylistId;
    };
}
