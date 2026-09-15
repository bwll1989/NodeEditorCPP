#pragma once

#include <algorithm>

#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QSignalBlocker>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkProxy>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "DAWControllerInterface.hpp"
#include <iostream>
#include <qcryptographichash.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include "Common/Devices/FTDAWController/FTDAWController.h"
#include "QMutex"
#include "PluginDefinition.hpp"
#include  "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class DAWControllerNode : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

        static constexpr int kPollIntervalMs = 2000;
        static constexpr int kRolePlayerId = Qt::UserRole;
        static constexpr int kRoleIndex = Qt::UserRole + 1;
        static constexpr int kRoleStatus = Qt::UserRole + 2;
        static constexpr int kDefaultPort = 2004;

    public:
        /**
         * @brief 播放器信息结构体
         */
        struct PlayerInfo
        {
            int id = -1;
            QString name;
            QString type;
            QString status;
            bool enable = false;
            int sort = 0;
        };

        /**
         * @brief 构造函数，初始化DAW控制器节点
         *
         * - 每 2 秒 GET /players 获取播放列表（轮询+保活）
         * - 端口数量由播放列表驱动（不可手动编辑）：In=players.size, Out=players.size
         * - 输入/输出端口 i 对应列表排序号 i；true=PLAY false=STOP；每次写入都发令
         */
        DAWControllerNode(){
            InPortCount = 0;
            OutPortCount = 0;
            CaptionVisible = true;
            PortEditable = false;
            Caption = "FT-DAWController";
            WidgetEmbeddable = false;
            Resizable = true;

            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();

            m_controller = FTDAWController::acquire();

            m_networkManager = new QNetworkAccessManager(this);
            m_networkManager->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
            connect(m_networkManager, &QNetworkAccessManager::finished,
                    this, &DAWControllerNode::onNetworkReplyFinished);

            m_pollTimer = new QTimer(this);
            m_pollTimer->setInterval(kPollIntervalMs);
            connect(m_pollTimer, &QTimer::timeout, this, &DAWControllerNode::fetchPlayers);

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "host";
                b.control = widget->hostEdit;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "connected";
                b.control=widget->connectionStatusLabel;
                AbstractDelegateModel::registerExternalBinding("/connected", this, b);
            }

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [=](){
                setHost(widget->hostEdit->text().trimmed());
                fetchPlayers();
            });

            connect(widget->refreshButton, &QPushButton::clicked, this, [=](){
                fetchPlayers();
            });


            if (m_controller) {
                connect(m_controller, &FTDAWController::isReady, this, [=](bool ready){
                    setConnected(ready);
                });

                connect(this, &DAWControllerNode::hostChanged, this, [=](const QString &host){
                    if (widget && widget->hostEdit) {
                        const QSignalBlocker blocker(widget->hostEdit);
                        widget->hostEdit->setText(host);
                    }
                });
            }

            widget->updateConnectionStatus(false);
        }

        /**
         * @brief 析构函数，清理资源
         *
         * 清理顺序（避免悬垂回调、双释放与 destroyed wildcard 告警）：
         *   1. 停止轮询定时器（this 子对象，父析构会自动 delete，这里只停，避免 deleteLater 双释放）
         *   2. QNetworkAccessManager → finished 信号 → this 的具体槽逐条断开，不 deleteLater（父子链会管）
         *   3. 断开 FTDAWController 单例 → this 的具体信号连接（不用 wildcard，避免 touched destroyed 告警）
         *   4. 取消 GlobalEventBus 中本对象所有订阅
         *   5. 释放 controller 单例引用；其他由 QObject 父子链 + 智能指针自动清理
         */
        ~DAWControllerNode(){
            if (m_pollTimer) {
                m_pollTimer->stop();
            }

            if (m_networkManager) {
                QObject::disconnect(m_networkManager, &QNetworkAccessManager::finished,
                                    this, &DAWControllerNode::onNetworkReplyFinished);
                m_networkManager = nullptr;
            }

            if (m_controller) {
                QObject::disconnect(m_controller, &FTDAWController::isReady,
                                    this, nullptr);
            }

            if (auto *bus = GlobalEventBus::instance()) {
                bus->unsubscribe(this);
            }

            // 注：不再做 wildcard 形式的 QObject::disconnect(sender, 0, this, 0)
            // 或 disconnect(this, 0, nullptr, nullptr)，避免 Qt 扫到 destroyed 内连时
            // 输出 "wildcard call disconnects from destroyed signal" 警告。
            // widget/hostEdit/refresh/playlist 上的 lambda 都是以 this 为 receiver，
            // widget 销毁或本对象销毁时 Qt 内部会自动清掉对应连接项。

            if (m_controller) {
                FTDAWController::release();
                m_controller = nullptr;
            }
        }

    public:
        QString host() const { return m_host; }
        bool connected() const { return m_connected; }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            const int i = static_cast<int>(portIndex);
            if (i < 0) {
                return {};
            }
            if (i < m_players.size()) {
                const auto &pl = m_players[i];
                return QStringLiteral("[%1] %2").arg(i).arg(pl.name);
            }
            return QStringLiteral("[%1] —").arg(i);
        }

        /**
         * @brief 获取端口连接策略
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 连接策略
         */
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            return (portType == PortType::In || portType == PortType::Out)
                ? ConnectionPolicy::Many
                : ConnectionPolicy::One;
        }

       /**
         * @brief 获取输出数据
         * @param portIndex 端口索引
         * @return 输出数据
         *
         * 每个输出端口 i 输出对应序号播放器的播放布尔状态。
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            const int i = static_cast<int>(portIndex);
            if (i < 0 || i >= static_cast<int>(OutPortCount)) {
                return nullptr;
            }
            ensureChannelCaches();
            if (i >= m_statusOutputs.size()) {
                m_statusOutputs.resize(static_cast<int>(OutPortCount));
            }
            if (!m_statusOutputs[i]) {
                const bool playing = (i < m_players.size())
                    && isPlayingStatus(m_players[i].status);
                m_statusOutputs[i] = std::make_shared<VariableData>(QVariant(playing));
            }
            return m_statusOutputs[i];
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         *
         * 端口每次写入都发令：true=PLAY，false=STOP（不因本地已勾选而吞掉），
         * 便于 Trigger 再次 true 重播同一个播放器。
         */
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
            const bool playing = toBool(*var);
            setDesiredPlaying(i, playing, true, false);
        }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
         * @brief 保存节点配置
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("host")] = m_host;

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject values = v.toObject();

            const QString loadedHost = values.value(QStringLiteral("host")).toString();
            // 兼容旧字段
            const QString legacyIp = values.value(QStringLiteral("IP")).toString();
            const QString legacyCmd = values.value(QStringLiteral("Command")).toString();
            const QString legacyPort = values.value(QStringLiteral("port")).toString();
            Q_UNUSED(legacyCmd)
            Q_UNUSED(legacyPort)

            if (!legacyIp.isEmpty() && loadedHost.isEmpty()) {
                setHost(legacyIp);
            } else {
                setHost(loadedHost);
            }
        }

    public slots:
        void setHost(const QString& host)
        {
            if (m_host == host) {
                return;
            }
            m_host = host;
            {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(host);
            }
            if (m_controller) {
                m_controller->connectToServer(host);
            }
            // 换主机时丢掉旧请求，避免连错地址的失败回调
            ++m_networkEpoch;
            m_pendingPoll = false;
            m_pollFailStreak = 0;
            Q_EMIT hostChanged(host);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrHost = makeFullOscAddress("/host");
            if (ev.address == addrHost) {
                setHost(ev.payload.toString());
                fetchPlayers();
            }
        }

    Q_SIGNALS:
        void hostChanged(const QString& host);
        void connectedChanged(bool connected);

     protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线并启动轮询
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/host"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            m_pollTimer->start();
            QTimer::singleShot(0, this, &DAWControllerNode::fetchPlayers);
        }

        void setConnected(bool ready)
        {
            if (m_connected == ready) {
                return;
            }
            m_connected = ready;
            if (widget) {
                widget->updateConnectionStatus(ready);
            }
            Q_EMIT connectedChanged(ready);
        }

    private slots:
        /**
         * @brief 发送HTTP GET请求获取/players播放列表
         */
        void fetchPlayers()
        {
            if (m_pendingPoll) {
                return;
            }
            const QUrl url = buildPlayersUrl();
            if (!url.isValid() || url.host().isEmpty()) {
                setPollStatus(QStringLiteral("主机地址无效"));
                return;
            }

            QNetworkRequest request(url);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            request.setTransferTimeout(5000);
#endif
            m_pendingPoll = true;
            if (!m_httpConnected) {
                setPollStatus(QStringLiteral("正在请求 %1 ...").arg(url.toString()));
            }

            QNetworkReply *reply = m_networkManager->get(request);
            reply->setProperty(kEpochProperty, m_networkEpoch);
            reply->setProperty(kContextProperty, QStringLiteral("poll"));
        }

        /**
         * @brief 处理HTTP响应
         */
        void onNetworkReplyFinished(QNetworkReply *reply)
        {
            const QString context = reply->property(kContextProperty).toString();
            const int replyEpoch = reply->property(kEpochProperty).toInt();
            const auto error = reply->error();
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QString errorString = reply->errorString();

            // 过期 / 主动 abort：静默丢弃（含换主机后迟到的 timeout）
            if (replyEpoch != m_networkEpoch
                || error == QNetworkReply::OperationCanceledError) {
                reply->deleteLater();
                return;
            }

            if (context == QLatin1String("poll")) {
                m_pendingPoll = false;
                if (error != QNetworkReply::NoError) {
                    ++m_pollFailStreak;
                    if (!m_httpConnected || m_pollFailStreak >= 2) {
                        m_httpConnected = false;
                        setPollStatus(QStringLiteral("请求失败: %1 (HTTP %2)").arg(errorString).arg(httpStatus));
                    }
                    reply->deleteLater();
                    return;
                }
                m_pollFailStreak = 0;
                const bool wasConnected = m_httpConnected;
                const int prevCount = m_players.size();
                const QByteArray body = reply->isOpen() ? reply->readAll() : QByteArray();
                reply->deleteLater();

                QJsonParseError parseError;
                const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    setPollStatus(QStringLiteral("JSON 解析失败: %1").arg(parseError.errorString()));
                    return;
                }

                const QJsonObject obj = doc.object();
                if (!wasConnected) {
                    m_httpConnected = true;
                    setConnected(true);
                }
                updatePlayersFromJson(obj);
                if (!wasConnected || prevCount != m_players.size()) {
                    setPollStatus(QStringLiteral("已加载 %1 个播放器").arg(m_players.size()));
                }
                return;
            }

            reply->deleteLater();
        }

    private:
        /**
         * @brief 构造 /players GET 请求的 URL（端口固定为 2004）
         */
        QUrl buildPlayersUrl() const
        {
            QString hostStr = m_host.trimmed();
            if (hostStr.isEmpty()) {
                hostStr = QStringLiteral("127.0.0.1");
            }
            if (hostStr.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)) {
                hostStr = hostStr.mid(7);
            } else if (hostStr.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
                hostStr = hostStr.mid(8);
            }
            const int slash = hostStr.indexOf(QLatin1Char('/'));
            if (slash >= 0) {
                hostStr = hostStr.left(slash);
            }
            // 端口固定 2004；hostStr 若带 IP:Port 则仅取 IP 部分
            const int colon = hostStr.lastIndexOf(QLatin1Char(':'));
            if (colon > 0) {
                bool ok = false;
                const int parsed = hostStr.mid(colon + 1).toInt(&ok);
                if (ok && parsed > 0 && parsed <= 65535) {
                    hostStr = hostStr.left(colon);
                }
            }
            if (hostStr.startsWith(QLatin1Char('[')) && hostStr.endsWith(QLatin1Char(']'))) {
                hostStr = hostStr.mid(1, hostStr.size() - 2);
            }
            QUrl url;
            url.setScheme(QStringLiteral("http"));
            url.setHost(hostStr);
            url.setPort(kDefaultPort);
            url.setPath(QStringLiteral("/players"));
            return url;
        }

        /**
         * @brief 解析JSON更新播放列表，并驱动端口数量变化
         *
         * - 播放列表（players）数量 = InPortCount
         * - OutPortCount = 1（STATUS聚合） + players.size
         * - 增减端口时严格按照 GraphModel 约定发送 portsAboutToBe* / ports* 信号
         */
        void updatePlayersFromJson(const QJsonObject &json)
        {
            QVector<PlayerInfo> items;
            const QJsonArray arr = json.value(QStringLiteral("players")).toArray();
            items.reserve(arr.size());
            int count = 0;
            for (const QJsonValue &val : arr) {
                if (!val.isObject()) {
                    continue;
                }
                const QJsonObject obj = val.toObject();
                PlayerInfo info;
                info.id = obj.value(QStringLiteral("id")).toInt(-1);
                info.name = obj.value(QStringLiteral("name")).toString();
                info.type = obj.value(QStringLiteral("type")).toString();
                info.status = obj.value(QStringLiteral("status")).toString();
                info.enable = obj.value(QStringLiteral("enable")).toBool(false);
                info.sort = obj.value(QStringLiteral("sort")).toInt(count);
                if (info.name.isEmpty()) {
                    info.name = QStringLiteral("Player %1").arg(info.id);
                }
                if (info.id >= 0) {
                    items.push_back(info);
                    ++count;
                }
            }

            std::sort(items.begin(), items.end(), [](const PlayerInfo &a, const PlayerInfo &b) {
                if (a.sort != b.sort) {
                    return a.sort < b.sort;
                }
                return a.id < b.id;
            });

            const int oldCount = m_players.size();
            const int newCount = items.size();
            const int oldIn = static_cast<int>(InPortCount);
            const int oldOut = static_cast<int>(OutPortCount);
            const int newIn = newCount;
            const int newOut = newCount;

            if (oldIn != newIn || oldOut != newOut) {
                // ---------- 输入端口变化 ----------
                if (newIn > oldIn) {
                    Q_EMIT portsAboutToBeInserted(PortType::In,
                        static_cast<PortIndex>(oldIn),
                        static_cast<PortIndex>(newIn - 1));
                    InPortCount = static_cast<unsigned int>(newIn);
                    Q_EMIT portsInserted();
                } else if (newIn < oldIn) {
                    Q_EMIT portsAboutToBeDeleted(PortType::In,
                        static_cast<PortIndex>(newIn),
                        static_cast<PortIndex>(oldIn - 1));
                    InPortCount = static_cast<unsigned int>(newIn);
                    Q_EMIT portsDeleted();
                }

                // ---------- 输出端口变化（与输入一一对应，没有单独 STATUS 端口） ----------
                if (newOut > oldOut) {
                    Q_EMIT portsAboutToBeInserted(PortType::Out,
                        static_cast<PortIndex>(oldOut),
                        static_cast<PortIndex>(newOut - 1));
                    OutPortCount = static_cast<unsigned int>(newOut);
                    Q_EMIT portsInserted();
                } else if (newOut < oldOut) {
                    Q_EMIT portsAboutToBeDeleted(PortType::Out,
                        static_cast<PortIndex>(newOut),
                        static_cast<PortIndex>(oldOut - 1));
                    OutPortCount = static_cast<unsigned int>(newOut);
                    Q_EMIT portsDeleted();
                }

                ensureChannelCaches();
            }

            m_players = items;
            rebuildPlaylistWidget();
            publishAllStatusOutputs();
        }

        /**
         * @brief 重建播放列表UI控件，每行带勾选框对应期望播/停
         */
        void rebuildPlaylistWidget()
        {
            QSignalBlocker blocker(widget->playlistWidget);
            widget->playlistWidget->clear();

            for (int i = 0; i < m_players.size(); ++i) {
                const auto &pl = m_players[i];
                auto *item = new QListWidgetItem(widget->playlistWidget);
                item->setData(kRolePlayerId, pl.id);
                item->setData(kRoleIndex, i);
                item->setData(kRoleStatus, pl.status);

                auto *row = new QWidget(widget->playlistWidget);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(4, 2, 4, 2);
                rowLayout->setSpacing(6);

                ensureDesiredSize(i + 1);
                const bool playing = isPlayingStatus(pl.status);
                const bool paused = (pl.status == QLatin1String("pause") || pl.status == QLatin1String("paused"));
                m_desiredPlaying[i] = playing;

                auto *toggleBtn = new QPushButton(row);
                toggleBtn->setProperty("playerIndex", i);
                toggleBtn->setCheckable(true);
                toggleBtn->setFixedSize(24, 24);
                toggleBtn->setIconSize(QSize(16, 16));
                toggleBtn->setFlat(true);
                toggleBtn->setFocusPolicy(Qt::NoFocus);
                toggleBtn->setStyleSheet(QStringLiteral(
                    "QPushButton {"
                    "  background: transparent;"
                    "  border: none;"
                    "  padding: 0px;"
                    "}"
                    "QPushButton:hover {"
                    "  background: rgba(255,255,255,20);"
                    "  border-radius: 3px;"
                    "}"
                    "QPushButton:checked {"
                    "  background: transparent;"
                    "  border: none;"
                    "}"));
                {
                    QSignalBlocker b(toggleBtn);
                    toggleBtn->setChecked(playing);
                    const QIcon playIcon(QStringLiteral(":/icons/icons/play.png"));
                    const QIcon stopIcon(QStringLiteral(":/icons/icons/stop.png"));
                    toggleBtn->setIcon(playing ? playIcon : stopIcon);
                    toggleBtn->setToolTip(playing
                        ? QStringLiteral("播放中，点击停止")
                        : (paused ? QStringLiteral("暂停，点击播放") : QStringLiteral("停止，点击播放")));
                }
                connect(toggleBtn, &QPushButton::toggled, this,
                    [this, i](bool checked) {
                        QPushButton *btn = qobject_cast<QPushButton *>(sender());
                        if (btn) {
                            const QIcon playIcon(QStringLiteral(":/icons/icons/play.png"));
                            const QIcon stopIcon(QStringLiteral(":/icons/icons/stop.png"));
                            btn->setIcon(checked ? playIcon : stopIcon);
                            btn->setToolTip(checked
                                ? QStringLiteral("播放中，点击停止")
                                : QStringLiteral("停止，点击播放"));
                        }
                        setDesiredPlaying(i, checked, true, true);
                    });

                auto *idLabel = new QLabel(QStringLiteral("[%1]").arg(pl.id), row);
                idLabel->setStyleSheet(QStringLiteral("color: #888; min-width: 40px;"));

                auto *nameLabel = new QLabel(pl.name, row);
                nameLabel->setToolTip(
                    QStringLiteral("index=%1\nid=%2\nname=%3\ntype=%4\nstatus=%5\nenable=%6\nsort=%7")
                        .arg(i)
                        .arg(pl.id)
                        .arg(pl.name)
                        .arg(pl.type)
                        .arg(pl.status)
                        .arg(pl.enable ? "true" : "false")
                        .arg(pl.sort));

                rowLayout->addWidget(toggleBtn);
                rowLayout->addWidget(idLabel);
                rowLayout->addWidget(nameLabel, 1);
                row->setLayout(rowLayout);

                item->setSizeHint(row->sizeHint());
                widget->playlistWidget->addItem(item);
                widget->playlistWidget->setItemWidget(item, row);

                if (playing) {
                    item->setBackground(QColor(60, 160, 100, 50));
                } else if (pl.enable) {
                    item->setBackground(QColor(60, 100, 160, 30));
                }
            }
        }

        /**
         * @brief 从期望状态同步列表所有勾选框
         */
        void syncListCheckboxesFromDesired()
        {
            for (int i = 0; i < widget->playlistWidget->count(); ++i) {
                syncListCheckbox(i, i < m_desiredPlaying.size() ? m_desiredPlaying[i] : false);
            }
        }

        /**
         * @brief 同步单条列表切换按钮状态
         */
        void syncListCheckbox(int index, bool checked)
        {
            if (index < 0 || index >= widget->playlistWidget->count()) {
                return;
            }
            QListWidgetItem *item = widget->playlistWidget->item(index);
            QWidget *row = widget->playlistWidget->itemWidget(item);
            if (!row) {
                return;
            }
            QPushButton *btn = row->findChild<QPushButton *>();
            if (!btn || !btn->isCheckable()) {
                return;
            }
            QSignalBlocker b(btn);
            btn->setChecked(checked);
            const QIcon playIcon(QStringLiteral(":/icons/icons/play.png"));
            const QIcon stopIcon(QStringLiteral(":/icons/icons/stop.png"));
            btn->setIcon(checked ? playIcon : stopIcon);
            btn->setToolTip(checked
                ? QStringLiteral("播放中，点击停止")
                : QStringLiteral("停止，点击播放"));
        }

        /**
         * @brief 在列表中选中指定ID的播放器
         */
        void selectPlayerInList(int playerId)
        {
            for (int i = 0; i < widget->playlistWidget->count(); ++i) {
                QListWidgetItem *item = widget->playlistWidget->item(i);
                if (item && item->data(kRolePlayerId).toInt() == playerId) {
                    widget->playlistWidget->setCurrentItem(item);
                    break;
                }
            }
        }

        /**
         * @brief 获取当前选中的播放器索引，没有则返回-1
         */
        int getSelectedPlayerIndex() const
        {
            QListWidgetItem *cur = widget->playlistWidget->currentItem();
            if (!cur) {
                return -1;
            }
            return cur->data(kRoleIndex).toInt();
        }

        /**
         * @brief 播放选中的播放器（UI按钮）
         */
        void playSelectedPlayer()
        {
            const int idx = getSelectedPlayerIndex();
            if (idx < 0) {
                setPollStatus(QStringLiteral("请先选择一个播放器"));
                return;
            }
            ensureDesiredSize(idx + 1);
            setDesiredPlaying(idx, true, true, false);
        }

        /**
         * @brief 停止选中的播放器（UI按钮）
         */
        void stopSelectedPlayer()
        {
            const int idx = getSelectedPlayerIndex();
            if (idx < 0) {
                setPollStatus(QStringLiteral("请先选择一个播放器"));
                return;
            }
            ensureDesiredSize(idx + 1);
            setDesiredPlaying(idx, false, true, false);
        }

        /**
         * @brief 设置某通道期望播放状态；可选发送命令
         * @param index 通道索引（与端口序号一致，从0起）
         * @param playing true=期望播放 false=期望停止
         * @param sendCommand 是否真正下发指令
         * @param requireChange 勾选框：仅变化时发令；输入端口/OSC：每次写入都发令
         */
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
            if (requireChange && !changed) {
                return;
            }

            if (index >= m_players.size() || m_players[index].id < 0) {
                setPollStatus(QStringLiteral("序号 %1 无对应播放器").arg(index));
                return;
            }

            const int playerId = m_players[index].id;
            sendPlayerCommand(playerId, playing ? QStringLiteral("play") : QStringLiteral("stop"));
        }

        /**
         * @brief 确保期望状态缓存至少 size 长度
         */
        void ensureDesiredSize(int size)
        {
            if (m_desiredPlaying.size() < size) {
                m_desiredPlaying.resize(size);
            }
        }

        /**
         * @brief 通过FTDAWController发送播放控制指令
         * 指令格式：{"type": "player","id": "1","operate": "play"}/0
         */
        void sendPlayerCommand(int playerId, const QString &operate)
        {
            if (!m_controller) {
                setPollStatus(QStringLiteral("FTDAWController 未就绪"));
                return;
            }
            QJsonObject body;
            body.insert(QStringLiteral("type"), QStringLiteral("player"));
            body.insert(QStringLiteral("id"), QString::number(playerId));
            body.insert(QStringLiteral("operate"), operate);
            const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
            const QString cmd = QStringLiteral("%1/0").arg(QString::fromUtf8(payload));

            m_controller->sendMessage(cmd);

            setPollStatus(QStringLiteral("已发送: %1 player #%2").arg(operate).arg(playerId));
            QTimer::singleShot(300, this, &DAWControllerNode::fetchPlayers);
        }

        /**
         * @brief 仅对齐内部缓存到当前 In/OutPortCount，绝不修改端口数量
         */
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

        /**
         * @brief 发布所有输出端口状态
         *
         * 每个输出端口 i 对应 player i 的播放布尔状态；仅当值发生变化时才发送 dataUpdated。
         */
        void publishAllStatusOutputs()
        {
            const int n = static_cast<int>(OutPortCount);
            if (m_statusOutputs.size() < n) {
                m_statusOutputs.resize(n);
            }
            if (m_lastOutputValues.size() < n) {
                m_lastOutputValues.resize(n, false);
                // 扩容的端口默认视为“未发送过”，首次值变化才输出，若初始化就是 true 也视为变化
                for (int i = m_lastOutputValues.size(); i < n; ++i) {
                    m_lastOutputValues[i] = !isPlayingStatus((i < m_players.size()) ? m_players[i].status : QString());
                }
            }
            for (int i = 0; i < n; ++i) {
                const bool playing = (i < m_players.size())
                    && isPlayingStatus(m_players[i].status);
                if (m_lastOutputValues.size() <= i || m_lastOutputValues[i] != playing) {
                    if (m_lastOutputValues.size() <= i) {
                        m_lastOutputValues.resize(i + 1, false);
                    }
                    m_lastOutputValues[i] = playing;
                    m_statusOutputs[i] = std::make_shared<VariableData>(QVariant(playing));
                    Q_EMIT dataUpdated(static_cast<PortIndex>(i));
                }
            }
        }

        /**
         * @brief 判断播放状态字符串是否为播放中
         */
        static bool isPlayingStatus(const QString &status)
        {
            return status == QLatin1String("play") || status == QLatin1String("playing");
        }

        /**
         * @brief VariableData 转 bool（支持 bool/int/"true"/"1"）
         */
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

        /**
         * @brief 设置状态文本
         */
        void setPollStatus(const QString &text)
        {
            if (widget && widget->statusLabel) {
                widget->statusLabel->setText(QStringLiteral("状态: %1").arg(text));
            }
        }

        static constexpr char kEpochProperty[] = "dawEpoch";
        static constexpr char kContextProperty[] = "dawContext";

        DAWControllerInterface *widget = new DAWControllerInterface();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
        FTDAWController* m_controller = nullptr;
        QNetworkAccessManager *m_networkManager = nullptr;
        QTimer *m_pollTimer = nullptr;

        QString m_host;
        bool m_connected = false;
        bool m_httpConnected = false;
        bool m_pendingPoll = false;
        int m_pollFailStreak = 0;
        int m_networkEpoch = 0;

        QVector<PlayerInfo> m_players;
        QVector<bool> m_desiredPlaying;
        QVector<std::shared_ptr<VariableData>> m_statusOutputs;
        QVector<bool> m_lastOutputValues;
    };

}
