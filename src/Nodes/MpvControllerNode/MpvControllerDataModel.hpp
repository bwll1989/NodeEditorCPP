/**
 * @file MpvControllerDataModel.hpp
 * @brief MPV Controller 节点（media-api，交互对齐 SlideShow / VLC Remote）
 *
 * - 每 5 秒 GET /api/v1/library（媒体库 + 保活）
 * - 切换：POST /api/v1/playback/play {"id":"..."}（由排序号取媒体 id）
 * - 停止：POST /api/v1/playback/stop
 *
 * 切换入口：
 * - PLAY(true) / 「播放」 → 按 Index 控件值切换并同步控件
 * - INDEX 端口 → 整数排序号，同步 Index 并播放
 * - 双击列表项 → 按该项切换，不改 Index 控件
 * - STOP 端口 / 「停止」 / OSC /stop(true) → 停止播放，不改 Index
 *
 * 端口：PLAY / INDEX / STOP → DONE / CONNECTED
 *
 * 协议见 media-api.md
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

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "MpvControllerInterface.hpp"
#include "PluginDefinition.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class MpvControllerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kPollIntervalMs = 5000;

        /** 列表项：UserRole = 媒体 id，UserRole+1 = 排序号 */
        static constexpr int kRoleMediaId = Qt::UserRole;
        static constexpr int kRoleIndex = Qt::UserRole + 1;

        enum InputPort : PortIndex { PlayPort = 0, IndexPort = 1, StopPort = 2 };
        enum OutputPort : PortIndex { DonePort = 0, ConnectedPort = 1 };

        Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        MpvControllerDataModel()
        {
            InPortCount = 3;
            OutPortCount = 2;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            m_doneOutput = std::make_shared<VariableData>(QVariant(false));
            m_connectedOutput = std::make_shared<VariableData>(QVariant(false));

            m_manager = new QNetworkAccessManager(this);
            connect(m_manager, &QNetworkAccessManager::finished,
                    this, &MpvControllerDataModel::onReplyFinished);

            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(kPollIntervalMs);
            connect(m_pollTimer, &QTimer::timeout, this, &MpvControllerDataModel::fetchLibrary);

            m_baseUrl = widget->baseUrlEdit->text().trimmed();
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
            values[QStringLiteral("baseUrl")] = m_baseUrl;
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
            if (values.contains(QStringLiteral("index"))) {
                applyIndexToControl(values.value(QStringLiteral("index")).toInt());
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        QString baseUrl() const { return m_baseUrl; }
        int index() const { return m_index; }
        bool connected() const { return m_connected; }

        void setBaseUrl(const QString &url)
        {
            QString trimmed = url.trimmed();
            while (trimmed.endsWith(QLatin1Char('/'))) {
                trimmed.chop(1);
            }
            if (m_baseUrl == trimmed) {
                return;
            }
            m_baseUrl = trimmed;
            {
                QSignalBlocker blocker(widget->baseUrlEdit);
                widget->baseUrlEdit->setText(m_baseUrl);
            }
            Q_EMIT baseUrlChanged(m_baseUrl);
        }

        void setIndex(int index)
        {
            if (index < 0) {
                return;
            }
            playByIndex(index, true);
        }

    Q_SIGNALS:
        void baseUrlChanged(const QString &url);
        void indexChanged(int index);
        void connectedChanged(bool connected);

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            for (const char *path : {"/baseUrl", "/index", "/play", "/stop"}) {
                bus->subscribe(makeFullOscAddress(QLatin1String(path)),
                               this, SLOT(onGlobalEvent(GlobalEvent)));
            }
            m_pollTimer->start();
            QTimer::singleShot(0, this, &MpvControllerDataModel::fetchLibrary);
        }

    private slots:
        /** GET /api/v1/library：刷新列表 + 保活 */
        void fetchLibrary()
        {
            if (m_pendingLibrary) {
                return;
            }

            syncParametersFromWidget();
            const QUrl url = QUrl::fromUserInput(apiRoot() + QStringLiteral("/library"));
            if (!url.isValid() || url.host().isEmpty()) {
                setConnected(false);
                setStatus(QStringLiteral("Invalid URL"));
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));
            QNetworkReply *reply = m_manager->get(request);
            if (!reply) {
                setConnected(false);
                setStatus(QStringLiteral("查询失败"));
                return;
            }

            reply->setProperty("op", QStringLiteral("library"));
            m_pendingLibrary = true;
            if (!m_connected) {
                setStatus(QStringLiteral("正在连接..."));
            }
        }

        /** 双击：按该项播放，不改 Index */
        void onMediaItemPlay(QListWidgetItem *item)
        {
            if (!item) {
                return;
            }
            playByIndex(item->data(kRoleIndex).toInt(), false);
        }

        /**
         * 按排序号播放媒体
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
                setStatus(QStringLiteral("无效的媒体序号"));
                publishDone(false);
                return;
            }

            if (updateIndexControl) {
                applyIndexToControl(index);
            }

            const QString mediaId = mediaIdAtIndex(index);
            if (mediaId.isEmpty()) {
                setStatus(QStringLiteral("序号 %1 无对应媒体").arg(index));
                publishDone(false);
                return;
            }

            const QUrl url = QUrl::fromUserInput(apiRoot() + QStringLiteral("/playback/play"));
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QStringLiteral("Invalid URL"));
                publishDone(false);
                return;
            }

            QJsonObject body;
            body.insert(QStringLiteral("id"), mediaId);

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));

            QNetworkReply *reply = m_manager->post(
                request, QJsonDocument(body).toJson(QJsonDocument::Compact));
            if (!reply) {
                setStatus(QStringLiteral("请求失败"));
                publishDone(false);
                return;
            }

            reply->setProperty("op", QStringLiteral("play"));
            reply->setProperty("mediaId", mediaId);
            reply->setProperty("playIndex", index);
            m_pendingPlay = true;
            m_pendingIndex = -1;
            m_pendingUpdateIndex = true;
            setStatus(QStringLiteral("切换 [%1] \"%2\"...").arg(index).arg(mediaId));
            AbstractDelegateModel::stateFeedBack("/play", true);
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
            const QUrl url = QUrl::fromUserInput(apiRoot() + QStringLiteral("/playback/stop"));
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QStringLiteral("Invalid URL"));
                publishDone(false);
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));

            QNetworkReply *reply = m_manager->post(request, QByteArrayLiteral("{}"));
            if (!reply) {
                setStatus(QStringLiteral("请求失败"));
                publishDone(false);
                return;
            }

            reply->setProperty("op", QStringLiteral("stop"));
            m_pendingStop = true;
            setStatus(QStringLiteral("正在停止..."));
            AbstractDelegateModel::stateFeedBack("/stop", true);
        }

        void onReplyFinished(QNetworkReply *reply)
        {
            if (!reply) {
                return;
            }
            const QString op = reply->property("op").toString();
            if (op == QLatin1String("library")) {
                handleLibraryFinished(reply);
            } else if (op == QLatin1String("play")) {
                handlePlayFinished(reply);
            } else if (op == QLatin1String("stop")) {
                handleStopFinished(reply);
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
                fetchLibrary();
            } else if (addr == makeFullOscAddress("/index")) {
                setIndex(ev.payload.toInt());
            } else if (addr == makeFullOscAddress("/play") && ev.payload.toBool()) {
                playByIndex(widget->indexSpinBox->value(), true);
            } else if (addr == makeFullOscAddress("/stop") && ev.payload.toBool()) {
                stopPlayback();
            }
        }

    private:
        void connectUiSignals()
        {
            connect(widget->baseUrlEdit, &QLineEdit::editingFinished, this, [this]() {
                setBaseUrl(widget->baseUrlEdit->text());
                fetchLibrary();
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
            connect(widget->mediaListWidget, &QListWidget::itemDoubleClicked,
                    this, &MpvControllerDataModel::onMediaItemPlay);
        }

        void registerOscBindings()
        {
            registerBinding("/baseUrl", "baseUrl", widget->baseUrlEdit);
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
            QString trimmed = widget->baseUrlEdit->text().trimmed();
            while (trimmed.endsWith(QLatin1Char('/'))) {
                trimmed.chop(1);
            }
            m_baseUrl = trimmed;
        }

        QString apiRoot() const
        {
            return resolveBaseUrl() + QStringLiteral("/api/v1");
        }

        QString resolveBaseUrl() const
        {
            QString base = m_baseUrl.trimmed();
            while (base.endsWith(QLatin1Char('/'))) {
                base.chop(1);
            }
            return base;
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
            QSignalBlocker blocker(widget->mediaListWidget);
            QListWidgetItem *matched = nullptr;
            for (int i = 0; i < widget->mediaListWidget->count(); ++i) {
                auto *item = widget->mediaListWidget->item(i);
                const bool current = (item->data(kRoleMediaId).toString() == m_playingMediaId);
                item->setBackground(current ? QColor(60, 100, 160, 80) : QBrush());
                if (current) {
                    matched = item;
                }
            }
            widget->mediaListWidget->setCurrentItem(matched);
        }

        QString mediaIdAtIndex(int index) const
        {
            if (index < 0 || index >= widget->mediaListWidget->count()) {
                return {};
            }
            auto *item = widget->mediaListWidget->item(index);
            return item ? item->data(kRoleMediaId).toString().trimmed() : QString();
        }

        /** 解析响应壳：返回 data 对象；失败时写 status 并返回空对象 */
        QJsonObject parseEnvelope(QNetworkReply *reply, bool *okOut) const
        {
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool httpOk = reply->error() == QNetworkReply::NoError
                && statusCode >= 200 && statusCode < 300;

            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject()) {
                if (okOut) {
                    *okOut = false;
                }
                return {};
            }

            const QJsonObject root = doc.object();
            const bool apiOk = root.value(QStringLiteral("ok")).toBool(httpOk);
            if (okOut) {
                *okOut = httpOk && apiOk;
            }
            if (!apiOk) {
                return root.value(QStringLiteral("error")).toObject();
            }
            return root.value(QStringLiteral("data")).toObject();
        }

        void handleLibraryFinished(QNetworkReply *reply)
        {
            m_pendingLibrary = false;

            bool ok = false;
            const QJsonObject dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                setConnected(false);
                const QString msg = dataOrError.value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QStringLiteral("连接失败")
                              : QStringLiteral("连接失败: %1").arg(msg));
                return;
            }

            setConnected(true);

            const QJsonArray items = dataOrError.value(QStringLiteral("items")).toArray();
            int loadedCount = 0;
            {
                QSignalBlocker blocker(widget->mediaListWidget);
                widget->mediaListWidget->clear();

                for (const QJsonValue &value : items) {
                    const QJsonObject obj = value.toObject();
                    const QString id = obj.value(QStringLiteral("id")).toString().trimmed();
                    if (id.isEmpty()) {
                        continue;
                    }
                    QString name = obj.value(QStringLiteral("name")).toString().trimmed();
                    if (name.isEmpty()) {
                        name = id;
                    }

                    const int displayIndex = loadedCount++;
                    const QString text = QStringLiteral("[%1] %2").arg(displayIndex).arg(name);
                    auto *item = new QListWidgetItem(text, widget->mediaListWidget);
                    item->setData(kRoleMediaId, id);
                    item->setData(kRoleIndex, displayIndex);
                    item->setToolTip(QStringLiteral("index=%1\nid=%2\npath=%3")
                                         .arg(displayIndex)
                                         .arg(id, obj.value(QStringLiteral("path")).toString()));
                }
            }

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
            setStatus(QStringLiteral("已加载 %1 个媒体").arg(loadedCount));
        }

        void handlePlayFinished(QNetworkReply *reply)
        {
            m_pendingPlay = false;
            AbstractDelegateModel::stateFeedBack("/play", false);

            bool ok = false;
            const QJsonObject dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                const QString msg = dataOrError.value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QStringLiteral("播放失败")
                              : QStringLiteral("播放失败: %1").arg(msg));
                publishDone(false);
            } else {
                m_playingMediaId = reply->property("mediaId").toString();
                // 优先用返回的 playback.media.id
                const QJsonObject media = dataOrError.value(QStringLiteral("media")).toObject();
                const QString idFromResp = media.value(QStringLiteral("id")).toString().trimmed();
                if (!idFromResp.isEmpty()) {
                    m_playingMediaId = idFromResp;
                }
                syncPlayingHighlight();
                publishDone(true);
                setStatus(QStringLiteral("正在播放 \"%1\"").arg(m_playingMediaId));
            }

            flushQueuedControl();
        }

        void handleStopFinished(QNetworkReply *reply)
        {
            m_pendingStop = false;
            AbstractDelegateModel::stateFeedBack("/stop", false);

            bool ok = false;
            const QJsonObject dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                const QString msg = dataOrError.value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QStringLiteral("停止失败")
                              : QStringLiteral("停止失败: %1").arg(msg));
                publishDone(false);
            } else {
                m_playingMediaId.clear();
                syncPlayingHighlight();
                publishDone(true);
                setStatus(QStringLiteral("已停止"));
            }

            flushQueuedControl();
        }

        /** 播放/停止互斥排队：完成当前请求后执行最新意图 */
        void flushQueuedControl()
        {
            if (m_resendStop) {
                m_resendStop = false;
                m_resendPlay = false;
                QTimer::singleShot(0, this, &MpvControllerDataModel::stopPlayback);
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

        MpvControllerInterface *widget = new MpvControllerInterface();
        QNetworkAccessManager *m_manager = nullptr;
        QTimer *m_pollTimer = nullptr;

        std::shared_ptr<VariableData> m_doneOutput;
        std::shared_ptr<VariableData> m_connectedOutput;

        QString m_baseUrl;
        int m_index = 0;
        bool m_connected = false;
        QString m_playingMediaId;

        bool m_pendingLibrary = false;
        bool m_pendingPlay = false;
        bool m_pendingStop = false;
        bool m_resendPlay = false;
        bool m_resendStop = false;
        int m_pendingIndex = -1;
        bool m_pendingUpdateIndex = true;
    };
}
