/**
 * @file SlideShowDataModel.hpp
 * @brief SlideShow 播放列表控制节点
 *
 * 行为概要：
 * - 连接成功后每 5 秒 GET /ajax/content/get（查询列表 + 保活）
 * - 切换播放：PUT /ajax/playlist/set?playlistName=...（由 index 查名称）
 *
 * 切换入口：
 * - TRIGGER(true) / 「播放」按钮 / INDEX 端口 → 按 Index 控件值切换，并同步控件
 * - 双击列表项 → 按该项切换，不改 Index 控件
 *
 * 端口：TRIGGER / INDEX → DONE / CONNECTED
 * 用户名、密码、当前播放名仅内部使用，不进属性系统（避免对外发布）
 */
#pragma once

#include <QBrush>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "SlideShowInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class SlideShowDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kPollIntervalMs = 5000;

        /** 列表项数据角色：名称 / 排序号 */
        static constexpr int kRoleName = Qt::UserRole;
        static constexpr int kRoleIndex = Qt::UserRole + 1;

        enum InputPort : PortIndex { TriggerPort = 0, IndexPort = 1 };
        enum OutputPort : PortIndex { DonePort = 0, ConnectedPort = 1 };

        // 用户名、密码、playlistName 故意不声明 Q_PROPERTY，避免属性树默认向外发布
        Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
        Q_PROPERTY(QString zoneName READ zoneName WRITE setZoneName NOTIFY zoneNameChanged)
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        SlideShowDataModel()
        {
            InPortCount = 2;
            OutPortCount = 2;
            Caption = QStringLiteral("SlideShow");
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            m_doneOutput = std::make_shared<VariableData>(QVariant(false));
            m_connectedOutput = std::make_shared<VariableData>(QVariant(false));

            m_manager = new QNetworkAccessManager(this);
            connect(m_manager, &QNetworkAccessManager::finished,
                    this, &SlideShowDataModel::onReplyFinished);

            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(kPollIntervalMs);
            connect(m_pollTimer, &QTimer::timeout, this, &SlideShowDataModel::fetchPlaylists);

            // 与界面初始值对齐（Index 默认 0）
            syncParametersFromWidget();
            m_index = widget->indexSpinBox->value();
            widget->updateConnectionStatus(false);

            connectUiSignals();
            registerOscBindings();
        }

        // ---------- 端口 ----------

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                static const char *const kIn[] = {"TRIGGER", "INDEX"};
                return (portIndex >= 0 && portIndex < 2) ? QString::fromLatin1(kIn[portIndex]) : QString();
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

            if (portIndex == TriggerPort) {
                // true 时按 Index 控件当前值切换
                if (isTriggerTrue(*var)) {
                    playByIndex(widget->indexSpinBox->value(), true);
                }
            } else if (portIndex == IndexPort) {
                setIndex(extractIndexFromData(var));
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            return (portType == PortType::In || portType == PortType::Out)
                ? ConnectionPolicy::Many
                : ConnectionPolicy::One;
        }

        // ---------- 持久化 ----------

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("baseUrl")] = m_baseUrl;
            values[QStringLiteral("username")] = m_username; // 仅本地存盘
            values[QStringLiteral("password")] = m_password;
            values[QStringLiteral("playlistName")] = m_playlistName;
            values[QStringLiteral("zoneName")] = m_zoneName;
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

            setBaseUrl(values.value(QStringLiteral("baseUrl")).toString(m_baseUrl));
            loadCredential(values);
            m_playlistName = values.value(QStringLiteral("playlistName")).toString(m_playlistName).trimmed();
            setZoneName(values.value(QStringLiteral("zoneName")).toString(m_zoneName));

            if (values.contains(QStringLiteral("index"))) {
                applyIndexToControl(values.value(QStringLiteral("index")).toInt());
            }
            syncPlayingHighlight();
        }

        QWidget *embeddedWidget() override { return widget; }

        // ---------- 属性读写 ----------

        QString baseUrl() const { return m_baseUrl; }
        QString zoneName() const { return m_zoneName; }
        int index() const { return m_index; }
        bool connected() const { return m_connected; }

        void setBaseUrl(const QString &url)
        {
            updateStringField(m_baseUrl, url.trimmed(), widget->baseUrlEdit,
                              &SlideShowDataModel::baseUrlChanged);
        }

        void setZoneName(const QString &zoneName)
        {
            updateStringField(m_zoneName, zoneName.trimmed(), widget->zoneNameEdit,
                              &SlideShowDataModel::zoneNameChanged);
        }

        /** INDEX / 播放按钮 / OSC：写入 Index 并切换 */
        void setIndex(int index)
        {
            if (index < 0) {
                return;
            }
            playByIndex(index, true);
        }

    Q_SIGNALS:
        void baseUrlChanged(const QString &url);
        void zoneNameChanged(const QString &zoneName);
        void indexChanged(int index);
        void connectedChanged(bool connected);

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            for (const char *path : {"/baseUrl", "/zoneName", "/index", "/play"}) {
                bus->subscribe(makeFullOscAddress(QLatin1String(path)),
                               this, SLOT(onGlobalEvent(GlobalEvent)));
            }
            m_pollTimer->start();
            QTimer::singleShot(0, this, &SlideShowDataModel::fetchPlaylists);
        }

    private slots:
        /** GET content/get：刷新列表并更新连接状态 */
        void fetchPlaylists()
        {
            if (m_pendingFetch) {
                return;
            }

            syncParametersFromWidget();
            const QUrl url = QUrl::fromUserInput(resolveBaseUrl() + QStringLiteral("/ajax/content/get"));
            if (!url.isValid() || url.host().isEmpty()) {
                setConnected(false);
                setStatus(QStringLiteral("Invalid URL"));
                return;
            }

            QNetworkRequest request(url);
            applyBasicAuth(request);
            QNetworkReply *reply = m_manager->get(request);
            if (!reply) {
                setConnected(false);
                setStatus(QStringLiteral("查询失败"));
                return;
            }

            reply->setProperty("op", QStringLiteral("content/get"));
            m_pendingFetch = true;
            if (!m_connected) {
                setStatus(QStringLiteral("正在连接..."));
            }
        }

        /** 双击：按该项播放，不改 Index 控件 */
        void onPlaylistItemPlay(QListWidgetItem *item)
        {
            if (!item) {
                return;
            }
            playByIndex(item->data(kRoleIndex).toInt(), false);
        }

        /**
         * 按排序号切换播放列表
         * @param updateIndexControl true=同步 Index 控件；双击为 false
         */
        void playByIndex(int index, bool updateIndexControl = true)
        {
            if (m_pendingSet) {
                // 请求进行中：排队，完成后重发
                m_resendSet = true;
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

            const QString name = playlistNameAtIndex(index);
            if (name.isEmpty()) {
                setStatus(QStringLiteral("序号 %1 无对应播放列表").arg(index));
                publishDone(false);
                return;
            }

            const QUrl url = buildPlaylistSetUrl(name, m_zoneName.trimmed());
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QStringLiteral("Invalid URL"));
                publishDone(false);
                return;
            }

            QNetworkRequest request(url);
            applyBasicAuth(request);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/x-www-form-urlencoded"));

            QNetworkReply *reply = m_manager->put(request, QByteArray());
            if (!reply) {
                setStatus(QStringLiteral("请求失败"));
                publishDone(false);
                return;
            }

            reply->setProperty("op", QStringLiteral("playlist/set"));
            m_pendingSet = true;
            m_pendingIndex = -1;
            m_pendingUpdateIndex = true;
            m_playlistName = name;
            syncPlayingHighlight();
            setStatus(QStringLiteral("切换 [%1] \"%2\"...").arg(index).arg(name));
            AbstractDelegateModel::stateFeedBack("/play", true);
        }

        void onReplyFinished(QNetworkReply *reply)
        {
            if (!reply) {
                return;
            }
            if (reply->property("op").toString() == QLatin1String("content/get")) {
                handleFetchFinished(reply);
            } else {
                handleSetFinished(reply);
            }
            reply->deleteLater();
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addr = ev.address;
            if (addr == makeFullOscAddress("/baseUrl")) {
                setBaseUrl(ev.payload.toString());
                fetchPlaylists();
            } else if (addr == makeFullOscAddress("/index")) {
                setIndex(ev.payload.toInt());
            } else if (addr == makeFullOscAddress("/zoneName")) {
                setZoneName(ev.payload.toString());
            } else if (addr == makeFullOscAddress("/play") && ev.payload.toBool()) {
                playByIndex(widget->indexSpinBox->value(), true);
            }
        }

    private:
        using StringChangedSignal = void (SlideShowDataModel::*)(const QString &);

        void connectUiSignals()
        {
            connect(widget->baseUrlEdit, &QLineEdit::editingFinished, this, [this]() {
                setBaseUrl(widget->baseUrlEdit->text());
                fetchPlaylists();
            });
            connect(widget->usernameEdit, &QLineEdit::editingFinished, this, [this]() {
                m_username = widget->usernameEdit->text().trimmed();
                fetchPlaylists();
            });
            connect(widget->passwordEdit, &QLineEdit::editingFinished, this, [this]() {
                m_password = widget->passwordEdit->text();
                fetchPlaylists();
            });
            connect(widget->zoneNameEdit, &QLineEdit::editingFinished, this, [this]() {
                setZoneName(widget->zoneNameEdit->text());
            });
            // 改 Index 只更新属性，不自动播放
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
            connect(widget->playlistWidget, &QListWidget::itemDoubleClicked,
                    this, &SlideShowDataModel::onPlaylistItemPlay);
        }

        void registerOscBindings()
        {
            registerBinding("/baseUrl", "baseUrl", widget->baseUrlEdit);
            registerBinding("/zoneName", "zoneName", widget->zoneNameEdit);
            registerBinding("/index", "index", widget->indexSpinBox);
            registerBinding("/connected", "connected", widget->connectionLabel);
            registerBinding("/play", "trigger", nullptr);
        }

        void registerBinding(const QString &path, const char *member, QWidget *control)
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = QString::fromUtf8(member);
            b.control = control;
            AbstractDelegateModel::registerExternalBinding(path, this, b);
        }

        /** 加载用户名/密码到成员与界面（不走属性系统） */
        void loadCredential(const QJsonObject &values)
        {
            m_username = values.value(QStringLiteral("username")).toString(m_username).trimmed();
            m_password = values.value(QStringLiteral("password")).toString(m_password);
            {
                QSignalBlocker blocker(widget->usernameEdit);
                widget->usernameEdit->setText(m_username);
            }
            {
                QSignalBlocker blocker(widget->passwordEdit);
                widget->passwordEdit->setText(m_password);
            }
        }

        void updateStringField(QString &field, const QString &value, QLineEdit *edit,
                               StringChangedSignal signal)
        {
            if (field == value) {
                return;
            }
            field = value;
            if (edit) {
                QSignalBlocker blocker(edit);
                edit->setText(field);
            }
            Q_EMIT (this->*signal)(field);
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
            Q_EMIT connectedChanged(m_connected); // 属性系统会对外发布，无需再 stateFeedBack
        }

        void publishDone(bool success)
        {
            m_doneOutput = std::make_shared<VariableData>(QVariant(success));
            Q_EMIT dataUpdated(DonePort);
        }

        void handleFetchFinished(QNetworkReply *reply)
        {
            m_pendingFetch = false;

            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (!isHttpSuccess(reply, statusCode)) {
                setConnected(false);
                setStatus(QStringLiteral("连接失败 HTTP %1").arg(statusCode));
                return;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject()) {
                setConnected(false);
                setStatus(QStringLiteral("返回无效 JSON"));
                return;
            }

            const QJsonObject root = doc.object();
            if (root.contains(QStringLiteral("success"))
                && !root.value(QStringLiteral("success")).toBool()) {
                setConnected(false);
                setStatus(QStringLiteral("业务失败"));
                return;
            }

            setConnected(true);

            const QJsonArray content = root.value(QStringLiteral("result"))
                                           .toObject()
                                           .value(QStringLiteral("content"))
                                           .toArray();

            int loadedCount = 0;
            {
                QSignalBlocker blocker(widget->playlistWidget);
                widget->playlistWidget->clear();

                for (const QJsonValue &value : content) {
                    const QJsonObject obj = value.toObject();
                    const QString name = obj.value(QStringLiteral("name")).toString().trimmed();
                    if (name.isEmpty()) {
                        continue;
                    }

                    const int displayIndex = loadedCount++;
                    const QString path = obj.value(QStringLiteral("path")).toString().trimmed();
                    const QString type = obj.value(QStringLiteral("type")).toString().trimmed();
                    const QString detail = !path.isEmpty() ? path : type;
                    const QString text = detail.isEmpty()
                        ? QStringLiteral("[%1] %2").arg(displayIndex).arg(name)
                        : QStringLiteral("[%1] %2  %3").arg(displayIndex).arg(name, detail);

                    auto *item = new QListWidgetItem(text, widget->playlistWidget);
                    item->setData(kRoleName, name);
                    item->setData(kRoleIndex, displayIndex);
                    item->setToolTip(QStringLiteral("index=%1\nname=%2\npath=%3\ntype=%4")
                                         .arg(displayIndex)
                                         .arg(name, path, type));
                }
            }

            // Index 上限随列表变化；越界则钳回 0
            {
                QSignalBlocker blocker(widget->indexSpinBox);
                const int maxIndex = qMax(0, loadedCount - 1);
                widget->indexSpinBox->setMaximum(maxIndex);
                if (loadedCount > 0 && m_index > maxIndex) {
                    m_index = 0;
                    Q_EMIT indexChanged(m_index);
                }
                widget->indexSpinBox->setValue(m_index);
            }

            syncPlayingHighlight();
            setStatus(QStringLiteral("已加载 %1 个播放列表").arg(loadedCount));
        }

        void handleSetFinished(QNetworkReply *reply)
        {
            m_pendingSet = false;
            AbstractDelegateModel::stateFeedBack("/play", false);

            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool httpOk = isHttpSuccess(reply, statusCode);

            bool businessOk = httpOk;
            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (doc.isObject() && doc.object().contains(QStringLiteral("success"))) {
                businessOk = doc.object().value(QStringLiteral("success")).toVariant().toBool();
            }

            const bool success = httpOk && businessOk;
            publishDone(success);
            syncPlayingHighlight();

            if (!httpOk) {
                setStatus(QStringLiteral("HTTP %1 %2").arg(statusCode).arg(reply->errorString()));
            } else if (!businessOk) {
                setStatus(QStringLiteral("业务失败"));
            } else {
                setStatus(QStringLiteral("正在播放 \"%1\"").arg(m_playlistName));
            }

            if (m_resendSet) {
                m_resendSet = false;
                const int index = (m_pendingIndex >= 0) ? m_pendingIndex : widget->indexSpinBox->value();
                const bool updateIndex = m_pendingUpdateIndex;
                m_pendingIndex = -1;
                m_pendingUpdateIndex = true;
                QTimer::singleShot(0, this, [this, index, updateIndex]() {
                    playByIndex(index, updateIndex);
                });
            }
        }

        /** 按当前播放名称高亮列表项（不改 Index） */
        void syncPlayingHighlight()
        {
            QSignalBlocker blocker(widget->playlistWidget);
            QListWidgetItem *matched = nullptr;
            for (int i = 0; i < widget->playlistWidget->count(); ++i) {
                auto *item = widget->playlistWidget->item(i);
                const bool current = (item->data(kRoleName).toString() == m_playlistName);
                item->setBackground(current ? QColor(60, 100, 160, 80) : QBrush());
                if (current) {
                    matched = item;
                }
            }
            widget->playlistWidget->setCurrentItem(matched);
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

        void syncParametersFromWidget()
        {
            m_baseUrl = widget->baseUrlEdit->text().trimmed();
            m_username = widget->usernameEdit->text().trimmed();
            m_password = widget->passwordEdit->text();
            m_zoneName = widget->zoneNameEdit->text().trimmed();
        }

        /** INDEX 端口：优先 index 字段，否则 default 整数（忽略布尔） */
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
                return -1; // 避免 true→1
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

        QString playlistNameAtIndex(int index) const
        {
            if (index < 0 || index >= widget->playlistWidget->count()) {
                return {};
            }
            auto *item = widget->playlistWidget->item(index);
            return item ? item->data(kRoleName).toString().trimmed() : QString();
        }

        QString resolveBaseUrl() const
        {
            QString base = m_baseUrl.trimmed();
            while (base.endsWith(QLatin1Char('/'))) {
                base.chop(1);
            }
            return base;
        }

        QUrl buildPlaylistSetUrl(const QString &playlistName, const QString &zoneName) const
        {
            QUrl url = QUrl::fromUserInput(resolveBaseUrl() + QStringLiteral("/ajax/playlist/set"));
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("playlistName"), playlistName);
            if (!zoneName.isEmpty()) {
                query.addQueryItem(QStringLiteral("zoneName"), zoneName);
            }
            url.setQuery(query);
            return url;
        }

        void applyBasicAuth(QNetworkRequest &request) const
        {
            const QByteArray token = QStringLiteral("%1:%2")
                                         .arg(m_username, m_password)
                                         .toUtf8()
                                         .toBase64();
            request.setRawHeader("Authorization", QByteArray("Basic ") + token);
        }

        static bool isHttpSuccess(QNetworkReply *reply, int statusCode)
        {
            return reply->error() == QNetworkReply::NoError
                && statusCode >= 200 && statusCode < 300;
        }

        SlideShowInterface *widget = new SlideShowInterface();
        QNetworkAccessManager *m_manager = nullptr;
        QTimer *m_pollTimer = nullptr;

        std::shared_ptr<VariableData> m_doneOutput;
        std::shared_ptr<VariableData> m_connectedOutput;

        QString m_baseUrl;
        QString m_username; // 不进属性系统
        QString m_password;
        QString m_playlistName; // 最近一次切换成功的播放列表名（用于高亮）
        QString m_zoneName;
        int m_index = 0; // 与 Index 控件一致
        bool m_connected = false;

        bool m_pendingFetch = false;
        bool m_pendingSet = false;
        bool m_resendSet = false;
        int m_pendingIndex = -1;
        bool m_pendingUpdateIndex = true;
    };
}
