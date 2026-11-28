/**
 * @file FlowDisplayControllerDataModel.hpp
 * @brief FlowDisplay Controller 节点（standalone HTTP 控制面，交互对齐 MpvController / VLC Remote）
 *
 * - 每 5 秒 GET /api/v1/timelines（时间轴列表 + 保活，含 is_active 活跃标记与各条目 loop）
 * - 切换：POST /api/v1/timelines/{id}/play（由排序号取时间轴 id）
 * - 停止：POST /api/v1/playback/stop
 * - 循环：GET /api/v1/timelines/{id} → 改 loop → PUT /api/v1/timelines/{id}
 *   （乐观锁 rev；409 CONFLICT 自动重读重写，≤2 次）
 *
 * 切换入口：
 * - PLAY(true) / 「播放」 → 按 Index 控件值切换并同步控件
 * - INDEX 端口 → 整数排序号，同步 Index 并播放
 * - 双击列表项 → 按该项切换，不改 Index 控件
 * - STOP 端口 / 「停止」 / OSC /stop(true) → 停止播放回待机，不改 Index
 * - LOOP 端口 / 「循环」勾选 / OSC /loop → 写回当前 Index 时间轴文档 loop 字段（true=循环 / false=单次）
 *
 * 端口：PLAY / INDEX / STOP / LOOP → DONE / CONNECTED
 *
 * 协议见 docs/display-standalone-api.md（FlowDisplay standalone HTTP API v1.8.01）
 */
#pragma once

#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
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
#include "FlowDisplayControllerInterface.hpp"
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
    class FlowDisplayControllerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kPollIntervalMs = 5000;      // 已连接保活轮询
        static constexpr int kRetryIntervalMs = 1000;     // 未连接快速重试
        static constexpr int kTransferTimeoutMs = 3000;   // 单请求传输超时（含 TCP 建连）
        static constexpr int kMaxLoopRetries = 2;

        /** 列表项：UserRole = 时间轴 id，UserRole+1 = 排序号，UserRole+2 = loop */
        static constexpr int kRoleTimelineId = Qt::UserRole;
        static constexpr int kRoleIndex = Qt::UserRole + 1;
        static constexpr int kRoleLoop = Qt::UserRole + 2;

        enum InputPort : PortIndex { PlayPort = 0, IndexPort = 1, StopPort = 2, LoopPort = 3 };
        enum OutputPort : PortIndex { DonePort = 0, ConnectedPort = 1 };

        Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
        Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        FlowDisplayControllerDataModel()
        {
            InPortCount = 4;
            OutPortCount = 2;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            m_doneOutput = std::make_shared<VariableData>(QVariant(false));
            m_connectedOutput = std::make_shared<VariableData>(QVariant(false));

            m_manager = new QNetworkAccessManager(this);
            // 兜底超时：目标不可达时 TCP SYN 重试可卡 20s+，会堵住"在途跳过"的轮询守卫
            m_manager->setTransferTimeout(kTransferTimeoutMs);
            connect(m_manager, &QNetworkAccessManager::finished,
                    this, &FlowDisplayControllerDataModel::onReplyFinished);

            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(kPollIntervalMs);
            connect(m_pollTimer, &QTimer::timeout, this, &FlowDisplayControllerDataModel::fetchTimelines);

            m_baseUrl = widget->baseUrlEdit->text().trimmed();
            m_index = widget->indexSpinBox->value();
            m_loop = widget->loopCheckBox->isChecked();
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
                static const char *const kIn[] = {"PLAY", "INDEX", "STOP", "LOOP"};
                return (portIndex >= 0 && portIndex < 4) ? QString::fromLatin1(kIn[portIndex]) : QString();
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
            } else if (portIndex == LoopPort) {
                setLoop(isTriggerTrue(*var));
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
            values[QStringLiteral("loop")] = m_loop;

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
            if (values.contains(QStringLiteral("loop"))) {
                applyLoopToControl(values.value(QStringLiteral("loop")).toBool(), false);
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        QString baseUrl() const { return m_baseUrl; }
        int index() const { return m_index; }
        bool loop() const { return m_loop; }
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

        void setLoop(bool loop)
        {
            applyLoopToControl(loop, true);
        }

    Q_SIGNALS:
        void baseUrlChanged(const QString &url);
        void indexChanged(int index);
        void loopChanged(bool loop);
        void connectedChanged(bool connected);

    protected:
        void afterModelReady() override
        {
            auto *bus = GlobalEventBus::instance();
            for (const char *path : {"/baseUrl", "/index", "/play", "/stop", "/loop"}) {
                bus->subscribe(makeFullOscAddress(QLatin1String(path)),
                               this, SLOT(onGlobalEvent(GlobalEvent)));
            }
            m_pollTimer->start();
            QTimer::singleShot(0, this, &FlowDisplayControllerDataModel::fetchTimelines);
        }

    private slots:
        /** GET /api/v1/timelines：刷新列表 + 保活（含 is_active / note / loop / duration） */
        void fetchTimelines()
        {
            if (m_pendingPoll) {
                return;
            }

            syncParametersFromWidget();
            const QUrl url = QUrl::fromUserInput(apiRoot() + QStringLiteral("/timelines"));
            if (!url.isValid() || url.host().isEmpty()) {
                setConnected(false);
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "无效的 URL"));
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));
            QNetworkReply *reply = m_manager->get(request);
            if (!reply) {
                setConnected(false);
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "查询失败"));
                return;
            }

            reply->setProperty("op", QStringLiteral("timelines"));
            m_pendingPoll = true;
            if (!m_connected) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "正在连接..."));
            }
        }

        /** 双击：按该项播放，不改 Index */
        void onTimelineItemPlay(QListWidgetItem *item)
        {
            if (!item) {
                return;
            }
            playByIndex(item->data(kRoleIndex).toInt(), false);
        }

        /** 轮询节奏：已连接 5s 保活；未连接 1s 快速重试（配合 3s 传输超时） */
        void scheduleNextPoll(bool ok)
        {
            m_pollTimer->setInterval(ok ? kPollIntervalMs : kRetryIntervalMs);
            m_pollTimer->start();
        }

        /**
         * 按排序号播放时间轴
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
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "已排队..."));
                return;
            }

            syncParametersFromWidget();
            if (index < 0) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "无效的时间轴序号"));
                publishDone(false);
                return;
            }

            if (updateIndexControl) {
                applyIndexToControl(index);
            }

            const QString timelineId = timelineIdAtIndex(index);
            if (timelineId.isEmpty()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "序号 %1 无对应时间轴").arg(index));
                publishDone(false);
                return;
            }

            const QUrl url = QUrl::fromUserInput(apiRoot()
                + QStringLiteral("/timelines/") + percentEncode(timelineId)
                + QStringLiteral("/play"));
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "无效的 URL"));
                publishDone(false);
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));

            QNetworkReply *reply = m_manager->post(
                request, QByteArrayLiteral("{}"));
            if (!reply) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "请求失败"));
                publishDone(false);
                return;
            }

            reply->setProperty("op", QStringLiteral("play"));
            reply->setProperty("timelineId", timelineId);
            reply->setProperty("playIndex", index);
            m_pendingPlay = true;
            m_pendingIndex = -1;
            m_pendingUpdateIndex = true;
            setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "切换 [%1] \"%2\"...").arg(index).arg(timelineId));
            AbstractDelegateModel::stateFeedBack("/play", true);
        }

        /** 停止当前播放回待机；不修改 Index */
        void stopPlayback()
        {
            if (m_pendingPlay || m_pendingStop) {
                m_resendStop = true;
                m_resendPlay = false;
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "停止已排队..."));
                return;
            }

            syncParametersFromWidget();
            const QUrl url = QUrl::fromUserInput(apiRoot() + QStringLiteral("/playback/stop"));
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "无效的 URL"));
                publishDone(false);
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));

            QNetworkReply *reply = m_manager->post(request, QByteArrayLiteral("{}"));
            if (!reply) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "请求失败"));
                publishDone(false);
                return;
            }

            reply->setProperty("op", QStringLiteral("stop"));
            m_pendingStop = true;
            setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "正在停止..."));
            AbstractDelegateModel::stateFeedBack("/stop", true);
        }

        /** 循环链入口（用户意图）：GET 文档 → 改 loop → PUT 写回；与 MPV loop 同为独立队列 */
        void applyLoopSetting(bool loop)
        {
            m_loopRetries = 0;

            const QString timelineId = timelineIdAtIndex(m_index);
            if (timelineId.isEmpty()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "序号 %1 无对应时间轴，无法设置循环").arg(m_index));
                return;
            }

            if (m_pendingLoopGet || m_pendingLoopPut) {
                m_pendingLoopValue = loop;
                m_resendLoop = true;
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "循环设置已排队..."));
                return;
            }

            startLoopGet(timelineId, loop);
        }

        void onReplyFinished(QNetworkReply *reply)
        {
            if (!reply) {
                return;
            }
            const QString op = reply->property("op").toString();
            if (op == QLatin1String("timelines")) {
                handleTimelinesFinished(reply);
            } else if (op == QLatin1String("play")) {
                handlePlayFinished(reply);
            } else if (op == QLatin1String("stop")) {
                handleStopFinished(reply);
            } else if (op == QLatin1String("loopGet")) {
                handleLoopGetFinished(reply);
            } else if (op == QLatin1String("loopPut")) {
                handleLoopPutFinished(reply);
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
                fetchTimelines();
            } else if (addr == makeFullOscAddress("/index")) {
                setIndex(ev.payload.toInt());
            } else if (addr == makeFullOscAddress("/play") && ev.payload.toBool()) {
                playByIndex(widget->indexSpinBox->value(), true);
            } else if (addr == makeFullOscAddress("/stop") && ev.payload.toBool()) {
                stopPlayback();
            } else if (addr == makeFullOscAddress("/loop")) {
                setLoop(ev.payload.toBool());
            }
        }

    private:
        void connectUiSignals()
        {
            connect(widget->baseUrlEdit, &QLineEdit::editingFinished, this, [this]() {
                setBaseUrl(widget->baseUrlEdit->text());
                fetchTimelines();
            });
            connect(widget->indexSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int value) {
                        if (m_index == value) {
                            return;
                        }
                        m_index = value;
                        Q_EMIT indexChanged(m_index);
                        syncLoopFromList(m_index);
                    });
            connect(widget->playButton, &QPushButton::clicked, this, [this]() {
                setIndex(widget->indexSpinBox->value());
            });
            connect(widget->stopButton, &QPushButton::clicked, this, [this]() {
                stopPlayback();
            });
            connect(widget->loopCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setLoop(checked);
            });
            connect(widget->timelineListWidget, &QListWidget::itemDoubleClicked,
                    this, &FlowDisplayControllerDataModel::onTimelineItemPlay);
        }

        void registerOscBindings()
        {
            registerBinding("/baseUrl", "baseUrl", widget->baseUrlEdit);
            registerBinding("/index", "index", widget->indexSpinBox);
            registerBinding("/loop", "loop", widget->loopCheckBox);
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
            widget->statusLabel->setText(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "状态: %1").arg(text));
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
            syncLoopFromList(index);
        }

        /**
         * 同步循环勾选；pushToServer 为 true 时启动 GET→PUT 循环链
         */
        void applyLoopToControl(bool loop, bool pushToServer)
        {
            const bool changed = (m_loop != loop);
            if (changed) {
                m_loop = loop;
                Q_EMIT loopChanged(m_loop);
            }
            {
                QSignalBlocker blocker(widget->loopCheckBox);
                if (widget->loopCheckBox->isChecked() != loop) {
                    widget->loopCheckBox->setChecked(loop);
                }
            }
            if (pushToServer && (changed || m_resendLoop)) {
                applyLoopSetting(loop);
            }
        }

        /** 从列表回写当前序号时间轴的 loop 到勾选框（服务器真相；链路在途时跳过） */
        void syncLoopFromList(int index)
        {
            if (m_pendingLoopGet || m_pendingLoopPut) {
                return;
            }
            if (index < 0 || index >= widget->timelineListWidget->count()) {
                return;
            }
            auto *item = widget->timelineListWidget->item(index);
            if (!item) {
                return;
            }
            const QVariant v = item->data(kRoleLoop);
            if (!v.isValid() || !v.canConvert<bool>()) {
                return;
            }
            applyLoopToControl(v.toBool(), false);
        }

        /** 活跃时间轴高亮（is_active 或最近一次播放成功的时间轴） */
        void syncActiveHighlight()
        {
            QSignalBlocker blocker(widget->timelineListWidget);
            QListWidgetItem *matched = nullptr;
            for (int i = 0; i < widget->timelineListWidget->count(); ++i) {
                auto *item = widget->timelineListWidget->item(i);
                const bool current = (item->data(kRoleTimelineId).toString() == m_activeTimelineId);
                item->setBackground(current ? QColor(60, 100, 160, 80) : QBrush());
                if (current) {
                    matched = item;
                }
            }
            widget->timelineListWidget->setCurrentItem(matched);
        }

        QString timelineIdAtIndex(int index) const
        {
            if (index < 0 || index >= widget->timelineListWidget->count()) {
                return {};
            }
            auto *item = widget->timelineListWidget->item(index);
            return item ? item->data(kRoleTimelineId).toString().trimmed() : QString();
        }

        /** 解析响应壳：成功返回 data 值（对象或数组）；失败时返回 error 对象 */
        QJsonValue parseEnvelope(QNetworkReply *reply, bool *okOut) const
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
            return root.value(QStringLiteral("data"));
        }

        void handleTimelinesFinished(QNetworkReply *reply)
        {
            m_pendingPoll = false;

            bool ok = false;
            const QJsonValue dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                setConnected(false);
                const QString msg = dataOrError.toObject().value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "连接失败")
                              : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "连接失败: %1").arg(msg));
                scheduleNextPoll(false);
                return;
            }

            setConnected(true);

            const QJsonArray items = dataOrError.toArray();
            int loadedCount = 0;
            QString activeId;
            {
                QSignalBlocker blocker(widget->timelineListWidget);
                widget->timelineListWidget->clear();

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
                    const bool active = obj.value(QStringLiteral("is_active")).toBool();
                    const QString note = obj.value(QStringLiteral("note")).toString().trimmed();

                    QString text = QStringLiteral("[%1] %2").arg(displayIndex).arg(name);
                    if (!note.isEmpty()) {
                        text += QStringLiteral("（%1）").arg(note);
                    }
                    if (active) {
                        text += QStringLiteral(" ▶");
                        activeId = id;
                    }
                    auto *item = new QListWidgetItem(text, widget->timelineListWidget);
                    item->setData(kRoleTimelineId, id);
                    item->setData(kRoleIndex, displayIndex);
                    item->setData(kRoleLoop, obj.value(QStringLiteral("loop")).toBool());

                    const qint64 durationUs = static_cast<qint64>(obj.value(QStringLiteral("duration_us")).toDouble());
                    QString tip = QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "index=%1\nid=%2\n时长=%3s\nloop=%4")
                                      .arg(displayIndex)
                                      .arg(id)
                                      .arg(durationUs / 1000000)
                                      .arg(obj.value(QStringLiteral("loop")).toBool()
                                               ? QStringLiteral("on")
                                               : QStringLiteral("off"));
                    if (!note.isEmpty()) {
                        tip += QStringLiteral("\nnote=%1").arg(note);
                    }
                    item->setToolTip(tip);
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

            m_activeTimelineId = activeId;
            syncActiveHighlight();
            syncLoopFromList(m_index);
            scheduleNextPoll(true);
            setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "已加载 %1 个时间轴").arg(loadedCount));
        }

        void handlePlayFinished(QNetworkReply *reply)
        {
            m_pendingPlay = false;
            AbstractDelegateModel::stateFeedBack("/play", false);

            bool ok = false;
            const QJsonValue dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                const QString msg = dataOrError.toObject().value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "播放失败")
                              : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "播放失败: %1").arg(msg));
                publishDone(false);
            } else {
                // 优先用返回的 data.id
                const QString idFromResp = dataOrError.toObject().value(QStringLiteral("id")).toString().trimmed();
                m_activeTimelineId = idFromResp.isEmpty()
                    ? reply->property("timelineId").toString()
                    : idFromResp;
                syncActiveHighlight();
                publishDone(true);
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "正在播放 \"%1\"").arg(m_activeTimelineId));
            }

            flushQueuedControl();
        }

        void handleStopFinished(QNetworkReply *reply)
        {
            m_pendingStop = false;
            AbstractDelegateModel::stateFeedBack("/stop", false);

            bool ok = false;
            const QJsonValue dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                const QString msg = dataOrError.toObject().value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "停止失败")
                              : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "停止失败: %1").arg(msg));
                publishDone(false);
            } else {
                m_activeTimelineId.clear();
                syncActiveHighlight();
                publishDone(true);
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "已停止"));
            }

            flushQueuedControl();
        }

        /** 循环链第一步：GET /timelines/{id} 取完整文档（含 rev） */
        void startLoopGet(const QString &timelineId, bool loop)
        {
            // 防御：409 重试定时器与用户新意图并发时，只允许一条链在途
            if (m_pendingLoopGet || m_pendingLoopPut) {
                return;
            }
            const QUrl url = QUrl::fromUserInput(apiRoot()
                + QStringLiteral("/timelines/") + percentEncode(timelineId));
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "无效的 URL"));
                flushLoopResend();
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));

            QNetworkReply *reply = m_manager->get(request);
            if (!reply) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "请求失败"));
                flushLoopResend();
                return;
            }

            reply->setProperty("op", QStringLiteral("loopGet"));
            reply->setProperty("loop", loop);
            reply->setProperty("timelineId", timelineId);
            m_pendingLoopGet = true;
            setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "读取 \"%1\"...").arg(timelineId));
        }

        void handleLoopGetFinished(QNetworkReply *reply)
        {
            m_pendingLoopGet = false;
            const bool wantLoop = reply->property("loop").toBool();
            const QString timelineId = reply->property("timelineId").toString();

            bool ok = false;
            const QJsonValue dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                const QString msg = dataOrError.toObject().value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "读取时间轴失败")
                              : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "读取时间轴失败: %1").arg(msg));
                flushLoopResend();
                return;
            }

            QJsonObject doc = dataOrError.toObject();
            if (doc.value(QStringLiteral("readonly")).toBool()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "\"%1\" 为只读条目（.show/ini 装配），无法设置循环").arg(timelineId));
                flushLoopResend();
                return;
            }

            doc.insert(QStringLiteral("loop"), wantLoop);

            const QUrl url = QUrl::fromUserInput(apiRoot()
                + QStringLiteral("/timelines/") + percentEncode(timelineId));
            if (!url.isValid() || url.host().isEmpty()) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "无效的 URL"));
                flushLoopResend();
                return;
            }

            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader,
                              QStringLiteral("application/json"));

            QNetworkReply *putReply = m_manager->put(
                request, QJsonDocument(doc).toJson(QJsonDocument::Compact));
            if (!putReply) {
                setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "请求失败"));
                flushLoopResend();
                return;
            }

            putReply->setProperty("op", QStringLiteral("loopPut"));
            putReply->setProperty("loop", wantLoop);
            putReply->setProperty("timelineId", timelineId);
            m_pendingLoopPut = true;
            setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "设置循环: %1...")
                          .arg(wantLoop ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "开") : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "关")));
        }

        void handleLoopPutFinished(QNetworkReply *reply)
        {
            m_pendingLoopPut = false;
            const bool wantLoop = reply->property("loop").toBool();
            const QString timelineId = reply->property("timelineId").toString();

            bool ok = false;
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QJsonValue dataOrError = parseEnvelope(reply, &ok);
            if (!ok) {
                // 乐观锁冲突：文档已被其他窗口修改，重读重写（有限次）
                if (statusCode == 409 && m_loopRetries < kMaxLoopRetries) {
                    ++m_loopRetries;
                    setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "文档已被修改，重读重试 (%1)...").arg(m_loopRetries));
                    QTimer::singleShot(0, this, [this, wantLoop, timelineId]() {
                        startLoopGet(timelineId, wantLoop);
                    });
                    return;
                }
                const QString msg = dataOrError.toObject().value(QStringLiteral("message")).toString();
                setStatus(msg.isEmpty()
                              ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "设置循环失败")
                              : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "设置循环失败: %1").arg(msg));
                flushLoopResend();
                return;
            }

            m_loopRetries = 0;
            applyLoopToControl(wantLoop, false);
            setStatus(QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "\"%1\" 循环: %2")
                          .arg(timelineId, wantLoop ? QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "开") : QCoreApplication::translate("Nodes::FlowDisplayControllerDataModel", "关")));
            flushLoopResend();
        }

        /** 循环链排队重发：在途链完成后执行最新意图 */
        void flushLoopResend()
        {
            if (!m_resendLoop) {
                return;
            }
            m_resendLoop = false;
            const bool pending = m_pendingLoopValue;
            QTimer::singleShot(0, this, [this, pending]() {
                applyLoopSetting(pending);
            });
        }

        /** 播放/停止互斥排队：完成当前请求后执行最新意图 */
        void flushQueuedControl()
        {
            if (m_resendStop) {
                m_resendStop = false;
                m_resendPlay = false;
                QTimer::singleShot(0, this, &FlowDisplayControllerDataModel::stopPlayback);
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

        static QString percentEncode(const QString &segment)
        {
            return QString::fromUtf8(QUrl::toPercentEncoding(segment));
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

        FlowDisplayControllerInterface *widget = new FlowDisplayControllerInterface();
        QNetworkAccessManager *m_manager = nullptr;
        QTimer *m_pollTimer = nullptr;

        std::shared_ptr<VariableData> m_doneOutput;
        std::shared_ptr<VariableData> m_connectedOutput;

        QString m_baseUrl;
        int m_index = 0;
        bool m_loop = false;
        bool m_connected = false;
        QString m_activeTimelineId;

        bool m_pendingPoll = false;
        bool m_pendingPlay = false;
        bool m_pendingStop = false;
        bool m_pendingLoopGet = false;
        bool m_pendingLoopPut = false;
        bool m_resendPlay = false;
        bool m_resendStop = false;
        bool m_resendLoop = false;
        bool m_pendingLoopValue = false;
        int m_loopRetries = 0;
        int m_pendingIndex = -1;
        bool m_pendingUpdateIndex = true;
    };
}
