/**
 * @file VlcRemoteDataModel.hpp
 * @brief VLC Remote 节点核心数据模型
 *
 * 通过 VLC HTTP Web 接口（Lua HTTP）远程控制播放器，功能包括：
 * - 播放 / 停止 / 全屏 / 音量 / 列表切换
 * - 实时状态 JSON 输出（STATUS 端口）
 * - 嵌入式 UI 与 GlobalEventBus OSC 外部控制
 *
 * VLC 需启用 HTTP 接口，例如：
 *   vlc --extraintf http --http-password xxx --http-port 8080
 *
 * 输入端口（5 个）：
 *   0 PLAY/STOP  - true 播放，false 停止
 *   1 STOP       - true 停止
 *   2 VOLUME     - 0～100 百分比
 *   3 INDEX      - playlistID 切换列表项
 *   4 FULLSCREEN - true 切换全屏
 *
 * 输出端口（1 个）：
 *   0 STATUS     - VariableData JSON 播放状态
 */
#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QUrlQuery>
#include <QtCore/QHash>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListWidget>
#include <QSignalBlocker>
#include <QColor>

#include "NodeDataList.hpp"
#include "VlcRemoteInterface.hpp"
#include "PluginDefinition.hpp"
#include "VlcHttpClient.h"
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @class VlcRemoteDataModel
     * @brief VLC HTTP 远程控制节点
     *
     * 架构说明：
     * - client：处理 status.json（状态查询 + 命令发送）
     * - playlistClient：独立处理 playlist.json，避免与状态轮询互相阻塞
     * - refreshTimer：命令发送后延迟 900ms 再刷新状态，等待 VLC 更新
     *
     * 音量换算：VLC 内部 256 = 100%，发送命令时使用绝对值而非 "50%" 字符串，
     * 避免部分 VLC 版本解析百分号时返回非 JSON 响应。
     */
    class VlcRemoteDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString hostAddress READ hostAddress WRITE setHostAddress NOTIFY hostAddressChanged)
        Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
        Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

    public:
        VlcRemoteDataModel()
        {
            InPortCount = 5;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = true;

            // --- 网络与 UI 组件 ---
            client = new VlcHttpClient(this);
            playlistClient = new VlcHttpClient(this);
            widget = new VlcRemoteInterface();

            // 命令发送后延迟刷新状态（单次触发）
            refreshTimer = new QTimer(this);
            refreshTimer->setInterval(900);
            refreshTimer->setSingleShot(true);

            // --- 外部 OSC 绑定：按钮触发类 ---
            const auto bindTrigger = [this](const QString &path, QPushButton *button) {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("trigger");
                b.control = button;
                AbstractDelegateModel::registerExternalBinding(path, this, b);
            };
            bindTrigger("/play", widget->playPauseButton);
            bindTrigger("/stop", widget->stopButton);
            bindTrigger("/fullscreen", widget->fullscreenButton);
            bindTrigger("/refresh_playlist", widget->refreshPlaylistButton);

            // --- 外部 OSC 绑定：属性同步类 ---
            const auto bindProperty = [this](const QString &path, const char *member, QWidget *control) {
                NodeDelegateModel::ExternalBinding b;
                b.member = QString::fromUtf8(member);
                b.control = control;
                AbstractDelegateModel::registerExternalBinding(path, this, b);
            };
            bindProperty("/host", "hostAddress", widget->hostEdit);
            bindProperty("/port", "port", widget->portSpinBox);
            bindProperty("/password", "password", widget->passwordEdit);
            bindProperty("/volume", "volume", widget->volumeEditor);

            // --- HTTP 响应回调 ---
            connect(client, &VlcHttpClient::jsonReady, this, &VlcRemoteDataModel::onStatusResponse);
            connect(client, &VlcHttpClient::requestFailed, this, &VlcRemoteDataModel::onStatusRequestFailed);
            connect(playlistClient, &VlcHttpClient::jsonReady, this, &VlcRemoteDataModel::onPlaylistResponse);
            connect(playlistClient, &VlcHttpClient::requestFailed, this, &VlcRemoteDataModel::onPlaylistRequestFailed);

            // --- UI → 模型 信号 ---
            connect(widget->hostEdit, &QLineEdit::textChanged, this, &VlcRemoteDataModel::setHostAddress);
            connect(widget->portSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &VlcRemoteDataModel::setPort);
            connect(widget->passwordEdit, &QLineEdit::textChanged, this, &VlcRemoteDataModel::setPassword);
            connect(widget->volumeEditor, &IntDragValueWidget::valueChanged,
                    this, &VlcRemoteDataModel::setVolume);

            connect(widget->playPauseButton, &QPushButton::clicked, this, &VlcRemoteDataModel::onPlayPause);
            connect(widget->stopButton, &QPushButton::clicked, this, &VlcRemoteDataModel::onStop);
            connect(widget->fullscreenButton, &QPushButton::clicked, this, &VlcRemoteDataModel::onFullscreen);
            connect(widget->refreshPlaylistButton, &QPushButton::clicked, this, &VlcRemoteDataModel::refreshPlaylist);

            connect(widget->playlistWidget, &QListWidget::itemClicked,
                    this, &VlcRemoteDataModel::onPlaylistItemActivated);
            connect(widget->playlistWidget, &QListWidget::itemDoubleClicked,
                    this, &VlcRemoteDataModel::onPlaylistItemActivated);

            connect(refreshTimer, &QTimer::timeout, this, &VlcRemoteDataModel::refreshStatus);

            // 从 UI 初始值同步到模型
            m_hostAddress = widget->hostEdit->text();
            m_port = widget->portSpinBox->value();
            m_password = widget->passwordEdit->text();
            m_volume = widget->volumeEditor->value();
        }

        ~VlcRemoteDataModel() override = default;

        /** @brief 端口显示名称 */
        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                switch (portIndex) {
                case 0: return QStringLiteral("PLAY/STOP");
                case 1: return QStringLiteral("STOP");
                case 2: return QStringLiteral("VOLUME");
                case 3: return QStringLiteral("INDEX");
                case 4: return QStringLiteral("FULLSCREEN");
                default: break;
                }
            }
            if (portType == PortType::Out && portIndex == 0) {
                return QStringLiteral("STATUS");
            }
            return AbstractDelegateModel::portCaption(portType, portIndex);
        }

        /** @brief 所有端口均为 VariableData 类型 */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            return VariableData().type();
        }

        /** @brief 输出 STATUS JSON */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(&statusOutput);
        }

        /**
         * @brief 处理输入端口数据
         *
         * 各端口语义见类头注释；触发类端口通过 isTriggerTrue/False 解析布尔值。
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) {
                return;
            }

            auto variableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!variableData || variableData->isEmpty()) {
                return;
            }

            switch (portIndex) {
            case 0: // PLAY/STOP：true 播放，false 停止
                if (isTriggerTrue(*variableData)) {
                    onPlay();
                } else if (isTriggerFalse(*variableData)) {
                    onStop();
                }
                break;
            case 1: // STOP：true 时停止
                if (isTriggerTrue(*variableData)) {
                    onStop();
                }
                break;
            case 2: // VOLUME：0～100 百分比
                setVolumeFromInput(variableData->value());
                break;
            case 3: // INDEX：按 playlistID 切换
            {
                const QString playlistId = playlistIdFromInput(variableData->value());
                if (!playlistId.isEmpty()) {
                    playPlaylistItemById(playlistId);
                }
                break;
            }
            case 4: // FULLSCREEN：true 时切换全屏
                if (isTriggerTrue(*variableData)) {
                    onFullscreen();
                }
                break;
            default:
                break;
            }
        }

        /** @brief 持久化连接配置与音量 */
        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values["hostAddress"] = m_hostAddress;
            values["port"] = m_port;
            values["password"] = m_password;
            values["volume"] = m_volume;
            modelJson["values"] = values;
            return modelJson;
        }

        /** @brief 恢复持久化配置 */
        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p["values"];
            if (!v.isObject()) {
                return;
            }

            const QJsonObject values = v.toObject();
            if (values.contains("hostAddress")) {
                setHostAddress(values["hostAddress"].toString());
            }
            if (values.contains("port")) {
                setPort(values["port"].toInt(8080));
            }
            if (values.contains("password")) {
                setPassword(values["password"].toString());
            }
            if (values.contains("volume")) {
                setVolume(values["volume"].toInt(100));
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            Q_UNUSED(portType)
            return ConnectionPolicy::Many;
        }

        /**
         * @brief 节点就绪后订阅 GlobalEventBus 并拉取初始状态
         *
         * OSC 地址格式：/{nodeId}/host、/{nodeId}/volume 等
         */
        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/password"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/volume"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/index"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/fullscreen"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/refresh_playlist"), this, SLOT(onGlobalEvent(GlobalEvent)));

            refreshStatus();
            refreshPlaylist();
        }

        // ===== Q_PROPERTY 访问器 =====

        QString hostAddress() const { return m_hostAddress; }
        int port() const { return m_port; }
        QString password() const { return m_password; }
        int volume() const { return m_volume; }

        /**
         * @brief 设置音量并发送到 VLC
         * @param value 0～100 百分比
         *
         * 用户主动修改时调用，会发送 HTTP volume 命令。
         * 从 VLC 状态回显时应使用 syncVolumeFromStatus() 避免循环发送。
         */
        void setVolume(int value)
        {
            value = qBound(0, value, 100);
            if (m_volume == value) {
                return;
            }
            m_volume = value;

            if (widget->volumeEditor->value() != value) {
                QSignalBlocker blocker(widget->volumeEditor);
                widget->volumeEditor->setValue(value);
            }

            sendCommand(QStringLiteral("volume"), QString::number(vlcPercentToAbsolute(value)));
            emit volumeChanged(value);
        }

        void setHostAddress(const QString &value)
        {
            if (m_hostAddress == value) {
                return;
            }
            m_hostAddress = value;
            syncLineEdit(widget->hostEdit, value);
            emit hostAddressChanged(value);
        }

        void setPort(int value)
        {
            if (m_port == value) {
                return;
            }
            m_port = value;
            if (widget->portSpinBox->value() != value) {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(value);
            }
            emit portChanged(value);
        }

        /** @brief 密码变更后重新拉取状态与列表 */
        void setPassword(const QString &value)
        {
            if (m_password == value) {
                return;
            }
            m_password = value;
            syncLineEdit(widget->passwordEdit, value);
            emit passwordChanged(value);
            refreshStatus();
            refreshPlaylist();
        }

    Q_SIGNALS:
        void hostAddressChanged(QString value);
        void portChanged(int value);
        void passwordChanged(QString value);
        void volumeChanged(int value);

    private Q_SLOTS:
        /** @brief GlobalEventBus 外部命令分发 */
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
            if (localPath == "host") {
                setHostAddress(ev.payload.toString());
            } else if (localPath == "port") {
                setPort(ev.payload.toInt());
            } else if (localPath == "password") {
                setPassword(ev.payload.toString());
            } else if (localPath == "play") {
                onPlay();
            } else if (localPath == "stop") {
                onStop();
            } else if (localPath == "volume") {
                setVolume(ev.payload.toInt());
            } else if (localPath == "index") {
                const QString playlistId = playlistIdFromInput(ev.payload);
                if (!playlistId.isEmpty()) {
                    playPlaylistItemById(playlistId);
                }
            } else if (localPath == "fullscreen") {
                onFullscreen();
            } else if (localPath == "refresh_playlist") {
                refreshPlaylist();
            }
        }

        /** @brief 播放（pl_play，无 id 时继续/启动列表） */
        void onPlay()
        {
            AbstractDelegateModel::stateFeedBack("/play", true);
            sendCommand(QStringLiteral("pl_play"));
            AbstractDelegateModel::stateFeedBack("/play", false);
        }

        /** @brief 播放/暂停切换（pl_pause，界面按钮专用） */
        void onPlayPause()
        {
            AbstractDelegateModel::stateFeedBack("/play", true);
            sendCommand(QStringLiteral("pl_pause"));
            AbstractDelegateModel::stateFeedBack("/play", false);
        }

        /** @brief 停止播放 */
        void onStop()
        {
            AbstractDelegateModel::stateFeedBack("/stop", true);
            sendCommand("pl_stop");
            AbstractDelegateModel::stateFeedBack("/stop", false);
        }

        /** @brief 切换全屏 */
        void onFullscreen()
        {
            AbstractDelegateModel::stateFeedBack("/fullscreen", true);
            sendCommand("fullscreen");
            AbstractDelegateModel::stateFeedBack("/fullscreen", false);
        }

        /** @brief 拉取 status.json */
        void refreshStatus()
        {
            sendRequest("status.json", {}, false);
        }

        /** @brief 拉取 playlist.json */
        void refreshPlaylist()
        {
            sendRequest("playlist.json", {}, true);
        }

        /** @brief 列表项单击/双击 → 按 playlistID 切换 */
        void onPlaylistItemActivated(QListWidgetItem *item)
        {
            if (!item) {
                return;
            }

            const QString playlistId = item->data(Qt::UserRole).toString();
            if (playlistId.isEmpty()) {
                return;
            }

            playPlaylistItemById(playlistId);
        }

    private:
        // ===== 输入值解析 =====

        /** @brief 判断 VariableData 是否为 true/1/"true" */
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
            return text == QStringLiteral("true") || text == QStringLiteral("1");
        }

        /** @brief 判断 VariableData 是否为 false/0/"false" */
        static bool isTriggerFalse(const VariableData &data)
        {
            const QVariant value = data.value();
            if (value.typeId() == QMetaType::Bool) {
                return !value.toBool();
            }
            if (value.canConvert<int>()) {
                return value.toInt() == 0;
            }
            const QString text = value.toString().trimmed().toLower();
            return text == QStringLiteral("false") || text == QStringLiteral("0");
        }

        // ===== 音量换算（VLC：256 = 100%） =====

        /** @brief VLC 绝对音量 → 百分比 */
        static int vlcVolumeToPercent(int volume)
        {
            return qBound(0, qRound(volume * 100.0 / 256.0), 200);
        }

        /** @brief 百分比 → VLC 绝对音量 */
        static int vlcPercentToAbsolute(int volumePercent)
        {
            volumePercent = qBound(0, volumePercent, 100);
            return qRound(volumePercent * 256.0 / 100.0);
        }

        /** @brief 从输入端口/O SC 解析 playlistID */
        static QString playlistIdFromInput(const QVariant &value)
        {
            if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::LongLong
                || value.typeId() == QMetaType::Double) {
                const int id = value.toInt();
                return id >= 0 ? QString::number(id) : QString();
            }

            const QString text = value.toString().trimmed();
            if (text.isEmpty()) {
                return QString();
            }

            bool ok = false;
            const int id = text.toInt(&ok);
            return ok && id >= 0 ? QString::number(id) : text;
        }

        /** @brief VOLUME 输入端口入口，解析后调用 setVolume */
        void setVolumeFromInput(const QVariant &value)
        {
            bool ok = false;
            int volumePercent = value.toInt(&ok);
            if (!ok) {
                volumePercent = static_cast<int>(value.toDouble());
            }
            setVolume(qBound(0, volumePercent, 100));
        }

        /**
         * @brief 从 VLC 状态回显音量到 UI（不发送 HTTP 命令）
         *
         * 防止状态轮询与用户拖动形成反馈循环。
         */
        void syncVolumeFromStatus(int volumePercent)
        {
            volumePercent = qBound(0, volumePercent, 100);
            if (m_volume == volumePercent) {
                return;
            }
            m_volume = volumePercent;

            if (widget->volumeEditor->value() != volumePercent) {
                QSignalBlocker blocker(widget->volumeEditor);
                widget->volumeEditor->setValue(volumePercent);
            }
            emit volumeChanged(volumePercent);
        }

        /** @brief 同步 QLineEdit 文本，阻塞信号避免循环 */
        static void syncLineEdit(QLineEdit *edit, const QString &value)
        {
            if (edit && edit->text() != value) {
                QSignalBlocker blocker(edit);
                edit->setText(value);
            }
        }

        // ===== HTTP 请求 =====

        /** @brief 构建 VLC HTTP 请求 URL */
        QUrl buildRequestUrl(const QString &resource, const QUrlQuery &query) const
        {
            QUrl url;
            url.setScheme("http");
            url.setHost(m_hostAddress.isEmpty() ? QStringLiteral("127.0.0.1") : m_hostAddress);
            url.setPort(m_port > 0 ? m_port : 8080);
            url.setPath("/requests/" + resource);
            if (!query.isEmpty()) {
                url.setQuery(query);
            }
            return url;
        }

        /**
         * @brief 发送 GET 请求
         * @param playlistRequest true 时使用 playlistClient，false 时使用 client 并启动刷新定时器
         */
        void sendRequest(const QString &resource, const QUrlQuery &query, bool playlistRequest)
        {
            const QUrl url = buildRequestUrl(resource, query);
            if (playlistRequest) {
                playlistClient->sendGet(url, m_password);
            } else {
                client->sendGet(url, m_password);
                refreshTimer->start();
            }
        }

        /**
         * @brief 向 status.json 发送 VLC 命令
         * @param command VLC 命令名（pl_play、pl_stop、volume、fullscreen 等）
         * @param value   可选参数，如音量绝对值
         */
        void sendCommand(const QString &command, const QString &value = QString())
        {
            QUrlQuery query;
            query.addQueryItem("command", command);
            if (!value.isEmpty()) {
                query.addQueryItem("val", value);
            }
            sendRequest("status.json", query, false);
            if (command == "pl_play") {
                QTimer::singleShot(300, this, &VlcRemoteDataModel::refreshPlaylist);
            }
        }

        // ===== 状态 JSON 构建 =====

        /** @brief 从 status.json 的 information.meta 提取当前文件名 */
        static QString extractCurrentFileFromMeta(const QJsonObject &json)
        {
            const QJsonObject information = json.value(QStringLiteral("information")).toObject();
            const QJsonObject category = information.value(QStringLiteral("category")).toObject();
            const QJsonObject meta = category.value(QStringLiteral("meta")).toObject();

            const QStringList keys = {
                QStringLiteral("filename"),
                QStringLiteral("title"),
                QStringLiteral("artist"),
                QStringLiteral("album")
            };
            for (const QString &key : keys) {
                const QString value = meta.value(key).toString();
                if (!value.isEmpty()) {
                    return value;
                }
            }
            return QString();
        }

        /**
         * @brief 将 VLC 原始 status.json 转换为 STATUS 输出 JSON
         *
         * 输出字段：connected、playing、paused、stopped、state、volume、
         * volumePercent、time、length、position、progressPercent、
         * currentFile、currentPlId、fullscreen、rate
         */
        QJsonObject buildStatusOutput(const QJsonObject &raw) const
        {
            const QString state = raw.value(QStringLiteral("state")).toString();
            const int volume = raw.value(QStringLiteral("volume")).toInt();
            const qint64 timeSec = raw.value(QStringLiteral("time")).toVariant().toLongLong();
            const qint64 lengthSec = raw.value(QStringLiteral("length")).toVariant().toLongLong();
            const double position = raw.value(QStringLiteral("position")).toDouble();
            const bool fullscreen = raw.value(QStringLiteral("fullscreen")).toBool()
                || raw.value(QStringLiteral("fullscreen")).toInt() != 0;
            const double rate = raw.value(QStringLiteral("rate")).toDouble(1.0);
            const QString currentPlId = raw.value(QStringLiteral("currentplid")).toVariant().toString();

            QString currentFile = extractCurrentFileFromMeta(raw);
            if (currentFile.isEmpty() && !currentPlId.isEmpty()) {
                currentFile = m_playlistTitles.value(currentPlId);
            }

            QJsonObject out;
            out.insert(QStringLiteral("connected"), m_connected);
            out.insert(QStringLiteral("state"), state);
            out.insert(QStringLiteral("playing"), state == QStringLiteral("playing"));
            out.insert(QStringLiteral("paused"), state == QStringLiteral("paused"));
            out.insert(QStringLiteral("stopped"), state == QStringLiteral("stopped"));
            out.insert(QStringLiteral("volume"), volume);
            out.insert(QStringLiteral("volumePercent"), vlcVolumeToPercent(volume));
            out.insert(QStringLiteral("time"), static_cast<double>(timeSec));
            out.insert(QStringLiteral("length"), static_cast<double>(lengthSec));
            out.insert(QStringLiteral("position"), position);
            out.insert(QStringLiteral("progressPercent"), qRound(position * 100.0));
            out.insert(QStringLiteral("currentFile"), currentFile);
            out.insert(QStringLiteral("currentPlId"), currentPlId);
            out.insert(QStringLiteral("fullscreen"), fullscreen);
            out.insert(QStringLiteral("rate"), rate);
            return out;
        }

        // ===== HTTP 响应处理 =====

        void onStatusResponse(const QJsonObject &json)
        {
            m_rawStatus = json;
            m_connected = true;
            statusOutput = buildStatusOutput(json);
            syncVolumeFromStatus(statusOutput.value(QStringLiteral("volumePercent")).toInt());
            updateStatusLabel(statusOutput);
            emit dataUpdated(0);
        }

        void onPlaylistResponse(const QJsonObject &json)
        {
            m_connected = true;
            updatePlaylistView(json);

            if (!m_rawStatus.isEmpty()) {
                statusOutput = buildStatusOutput(m_rawStatus);
                syncVolumeFromStatus(statusOutput.value(QStringLiteral("volumePercent")).toInt());
                updateStatusLabel(statusOutput);
                emit dataUpdated(0);
            }
        }

        void onStatusRequestFailed(const QString &message, int httpStatus)
        {
            Q_UNUSED(httpStatus)
            m_connected = false;
            statusOutput = QJsonObject{
                {QStringLiteral("connected"), false},
                {QStringLiteral("error"), message}
            };
            widget->statusLabel->setText(QString("连接失败: %1").arg(message));
            emit dataUpdated(0);
        }

        void onPlaylistRequestFailed(const QString &message, int httpStatus)
        {
            Q_UNUSED(httpStatus)
            if (!m_connected) {
                widget->statusLabel->setText(QString("连接失败: %1").arg(message));
            }
        }

        /** @brief 更新界面底部状态栏文字 */
        void updateStatusLabel(const QJsonObject &status)
        {
            widget->statusLabel->setText(
                QString("已连接 | 状态: %1 | 音量: %2% | 文件: %3 | 进度: %4 / %5 秒")
                    .arg(status.value(QStringLiteral("state")).toString("unknown"))
                    .arg(status.value(QStringLiteral("volumePercent")).toInt())
                    .arg(status.value(QStringLiteral("currentFile")).toString("-"))
                    .arg(status.value(QStringLiteral("time")).toInt())
                    .arg(status.value(QStringLiteral("length")).toInt()));
        }

        /**
         * @brief 按 playlistID 切换播放项
         *
         * VLC 命令：GET status.json?command=pl_play&id={playlistId}
         */
        void playPlaylistItemById(const QString &playlistId)
        {
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("command"), QStringLiteral("pl_play"));
            query.addQueryItem(QStringLiteral("id"), playlistId);
            sendRequest(QStringLiteral("status.json"), query, false);
            QTimer::singleShot(300, this, &VlcRemoteDataModel::refreshPlaylist);
            QTimer::singleShot(300, this, &VlcRemoteDataModel::refreshStatus);
        }

        // ===== 播放列表解析 =====

        /**
         * @brief 添加单个播放列表叶子节点到 QListWidget
         * @param titleMap playlistID → 文件名，供 STATUS 输出 currentFile 回退查找
         *
         * 显示格式：[playlistID] 文件名；当前播放项高亮。
         */
        static void appendPlaylistLeaf(const QJsonObject &obj,
                                       QListWidget *listWidget,
                                       QHash<QString, QString> &titleMap)
        {
            const QString type = obj.value(QStringLiteral("type")).toString();
            if (type == QStringLiteral("node")) {
                return;  // 跳过分组节点
            }

            QString playlistId = obj.value(QStringLiteral("id")).toString();
            if (playlistId.isEmpty()) {
                playlistId = QString::number(obj.value(QStringLiteral("id")).toInt());
            }
            if (playlistId.isEmpty() || playlistId == QStringLiteral("-1")) {
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

            titleMap.insert(playlistId, name);

            const QString displayText = QStringLiteral("[%1] %2").arg(playlistId, name);
            auto *item = new QListWidgetItem(displayText, listWidget);
            item->setData(Qt::UserRole, playlistId);
            item->setToolTip(uri);

            if (obj.value(QStringLiteral("current")).toString() == QStringLiteral("current")) {
                item->setBackground(QColor(60, 100, 160, 80));
                listWidget->setCurrentItem(item);
            }
        }

        /** @brief 递归遍历 playlist.json 树形 children */
        static void collectPlaylistItems(const QJsonArray &children,
                                         QListWidget *listWidget,
                                         QHash<QString, QString> &titleMap)
        {
            for (const QJsonValue &value : children) {
                if (!value.isObject()) {
                    continue;
                }
                const QJsonObject obj = value.toObject();
                if (obj.contains(QStringLiteral("children")) && obj[QStringLiteral("children")].isArray()) {
                    collectPlaylistItems(obj[QStringLiteral("children")].toArray(), listWidget, titleMap);
                    continue;
                }
                appendPlaylistLeaf(obj, listWidget, titleMap);
            }
        }

        /**
         * @brief 解析 playlist.json 并刷新列表 UI
         *
         * 兼容三种 JSON 结构：
         * 1. 扁平数组（VlcHttpClient 包装后的 _playlistItems）
         * 2. 顶层 children 数组
         * 3. root.children 嵌套结构
         */
        void updatePlaylistView(const QJsonObject &json)
        {
            widget->playlistWidget->clear();
            m_playlistTitles.clear();

            if (json.contains(QStringLiteral("_playlistItems"))
                && json[QStringLiteral("_playlistItems")].isArray()) {
                const QJsonArray items = json[QStringLiteral("_playlistItems")].toArray();
                for (const QJsonValue &value : items) {
                    if (value.isObject()) {
                        appendPlaylistLeaf(value.toObject(), widget->playlistWidget, m_playlistTitles);
                    }
                }
                return;
            }

            if (json.contains(QStringLiteral("children")) && json[QStringLiteral("children")].isArray()) {
                collectPlaylistItems(json[QStringLiteral("children")].toArray(),
                                     widget->playlistWidget,
                                     m_playlistTitles);
            } else if (json.contains(QStringLiteral("root")) && json[QStringLiteral("root")].isObject()) {
                const QJsonObject root = json[QStringLiteral("root")].toObject();
                if (root.contains(QStringLiteral("children")) && root[QStringLiteral("children")].isArray()) {
                    collectPlaylistItems(root[QStringLiteral("children")].toArray(),
                                         widget->playlistWidget,
                                         m_playlistTitles);
                }
            }
        }

        // ===== 成员变量 =====

        VlcHttpClient *client = nullptr;           ///< 状态/命令 HTTP 客户端
        VlcHttpClient *playlistClient = nullptr;   ///< 播放列表 HTTP 客户端（独立并发）
        VlcRemoteInterface *widget = nullptr;      ///< 嵌入式 UI
        QTimer *refreshTimer = nullptr;            ///< 命令后延迟刷新定时器
        bool m_connected = false;                  ///< 最近一次 HTTP 是否成功

        QString m_hostAddress = "127.0.0.1";
        int m_port = 8080;
        QString m_password;
        int m_volume = 100;

        QJsonObject m_rawStatus;                   ///< 最近一次原始 status.json
        QJsonObject statusOutput;                  ///< 格式化后的 STATUS 输出
        QHash<QString, QString> m_playlistTitles;  ///< playlistID → 文件名映射
    };
}
