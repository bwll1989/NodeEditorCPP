/**
 * @file NDVPlayerDataModel.hpp
 * @brief NDV Player：按 ID 经全局 NDVController 发指令；仅输出 STOPPED
 */
#pragma once

#include <QPushButton>
#include <QPointer>
#include <QSignalBlocker>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/NDVController/NDVController.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "NDVPlayerInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class NDVPlayerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int fileIndex READ getFileIndex WRITE setFileIndex NOTIFY fileIndexChanged)
        Q_PROPERTY(int playerId READ getPlayerId WRITE setPlayerId NOTIFY playerIdChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
        Q_PROPERTY(QString host READ host NOTIFY hostChanged)

        enum InputPort : PortIndex { IndexPort = 0, PlayPort = 1, StopPort = 2, LoopPort = 3 };
        enum OutputPort : PortIndex { StoppedPort = 0 };

    public:
        NDVPlayerDataModel()
        {
            InPortCount = 4;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = QStringLiteral("NDV Player");
            WidgetEmbeddable = false;
            Resizable = false;

            m_stoppedOutput = std::make_shared<VariableData>(QVariant(false));
            m_controller = NDVController::getInstance();

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("play");
                b.control = widget->Play;
                AbstractDelegateModel::registerExternalBinding("/play", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("stop");
                b.control = widget->Stop;
                AbstractDelegateModel::registerExternalBinding("/stop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("loop");
                b.control = widget->LoopPlay;
                AbstractDelegateModel::registerExternalBinding("/loop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("fileIndex");
                b.control = widget->FileIndex;
                AbstractDelegateModel::registerExternalBinding("/index", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("playerId");
                b.control = widget->PlayerID;
                AbstractDelegateModel::registerExternalBinding("/playerID", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("connected");
                b.control = widget->connectionButton;
                AbstractDelegateModel::registerExternalBinding("/connected", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("host");
                b.control = widget->hostButton;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }

            currentFileIndex = widget->FileIndex->value();
            currentPlayerId = widget->PlayerID->value();

            connect(widget->Play, &QPushButton::clicked, this, &NDVPlayerDataModel::sendPlayCommand);
            connect(widget->Stop, &QPushButton::clicked, this, &NDVPlayerDataModel::sendStopCommand);
            connect(widget->LoopPlay, &QPushButton::clicked, this, &NDVPlayerDataModel::sendLoopCommand);
            connect(widget->FileIndex, &IntDragValueWidget::valueChanged,
                    this, &NDVPlayerDataModel::setFileIndex);
            connect(widget->PlayerID, &IntDragValueWidget::valueChanged,
                    this, &NDVPlayerDataModel::setPlayerId);

            connect(m_controller, &NDVController::clientStatusChanged,
                    this, &NDVPlayerDataModel::onClientStatusChanged);
            connect(m_controller, &NDVController::clientListChanged,
                    this, &NDVPlayerDataModel::refreshFromController);

            refreshFromController();
        }

        ~NDVPlayerDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);

            if (m_controller) {
                disconnect(m_controller, nullptr, this, nullptr);
                m_controller = nullptr;
            }

            if (widget) {
                widget->setParent(nullptr);
                delete widget;
            }
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                static const char *const kIn[] = {"INDEX", "PLAY", "STOP", "LOOP"};
                return (portIndex >= 0 && portIndex < 4)
                    ? QString::fromLatin1(kIn[portIndex])
                    : QString();
            }
            if (portType == PortType::Out && portIndex == StoppedPort) {
                return QStringLiteral("STOPPED");
            }
            return {};
        }

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port == StoppedPort) {
                return m_stoppedOutput;
            }
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) {
                return;
            }
            auto variableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!variableData) {
                return;
            }

            switch (portIndex) {
            case IndexPort:
                setFileIndex(variableData->asInt());
                break;
            case PlayPort:
                if (variableData->asBool()) {
                    sendPlayCommand();
                } else {
                    sendStopCommand();
                }
                break;
            case StopPort:
                if (variableData->asBool()) {
                    sendStopCommand();
                }
                break;
            case LoopPort:
                if (variableData->asBool()) {
                    sendLoopCommand();
                }
                break;
            default:
                break;
            }
        }

        QJsonObject save() const override
        {
            QJsonObject settings;
            settings[QStringLiteral("FileIndex")] = currentFileIndex;
            settings[QStringLiteral("PlayerID")] = currentPlayerId;
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("PlayerSettings")] = settings;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p.value(QStringLiteral("PlayerSettings"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject obj = v.toObject();
            if (obj.contains(QStringLiteral("FileIndex"))) {
                setFileIndex(obj.value(QStringLiteral("FileIndex")).toInt());
            }
            if (obj.contains(QStringLiteral("PlayerID"))) {
                setPlayerId(obj.value(QStringLiteral("PlayerID")).toInt());
            }
            NodeDelegateModel::load(p);
        }

        QWidget *embeddedWidget() override { return widget; }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            return (portType == PortType::In || portType == PortType::Out)
                ? ConnectionPolicy::Many
                : ConnectionPolicy::One;
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto *bus = GlobalEventBus::instance();
            for (const char *path : {"/index", "/playerID", "/play", "/stop", "/loop"}) {
                bus->subscribe(makeFullOscAddress(QLatin1String(path)),
                               this, SLOT(onGlobalEvent(GlobalEvent)));
            }
            refreshFromController();
        }

        int getFileIndex() const { return currentFileIndex; }
        int getPlayerId() const { return currentPlayerId; }
        bool connected() const { return m_online; }
        QString host() const { return m_deviceIp; }

        void setFileIndex(int value)
        {
            if (currentFileIndex == value) {
                return;
            }
            currentFileIndex = value;
            if (widget && widget->FileIndex->value() != value) {
                QSignalBlocker blocker(widget->FileIndex);
                widget->FileIndex->setValue(value);
            }
            Q_EMIT fileIndexChanged(value);
        }

        void setPlayerId(int value)
        {
            if (currentPlayerId == value) {
                return;
            }
            currentPlayerId = value;
            if (widget && widget->PlayerID->value() != value) {
                QSignalBlocker blocker(widget->PlayerID);
                widget->PlayerID->setValue(value);
            }
            Q_EMIT playerIdChanged(value);
            refreshFromController();
        }

    Q_SIGNALS:
        void fileIndexChanged(int value);
        void playerIdChanged(int value);
        void connectedChanged(bool connected);
        void hostChanged(const QString &host);

    private slots:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("index")) {
                setFileIndex(ev.payload.toInt());
            } else if (localPath == QLatin1String("playerID")) {
                setPlayerId(ev.payload.toInt());
            } else if (localPath == QLatin1String("play")) {
                sendPlayCommand();
            } else if (localPath == QLatin1String("stop")) {
                sendStopCommand();
            } else if (localPath == QLatin1String("loop")) {
                sendLoopCommand();
            }
        }

        void onClientStatusChanged(int deviceId, const NDVClientInfo &info)
        {
            if (deviceId != currentPlayerId) {
                return;
            }
            applyClientInfo(info);
        }

        void refreshFromController()
        {
            if (!m_controller) {
                return;
            }
            applyClientInfo(m_controller->clientInfo(currentPlayerId));
        }

    private:
        void sendPlayCommand()
        {
            if (m_controller) {
                m_controller->sendCommand(QStringLiteral("play"), currentFileIndex, currentPlayerId);
            }

        }

        void sendStopCommand()
        {
            if (m_controller) {
                m_controller->sendCommand(QStringLiteral("stop"), 0, currentPlayerId);
            }
            AbstractDelegateModel::stateFeedBack("/stop", true);
        }

        void sendLoopCommand()
        {
            if (m_controller) {
                m_controller->sendCommand(QStringLiteral("loop"), currentFileIndex, currentPlayerId);
            }
            AbstractDelegateModel::stateFeedBack("/loop", true);
        }

        void applyClientInfo(const NDVClientInfo &info)
        {
            const bool wasOnline = m_online;
            const QString prevIp = m_deviceIp;
            m_online = info.online;
            m_deviceIp = info.ipAddress;
            // 在线且非 Playing 视为已停止；离线不报停止
            const bool stopped = info.online && (info.state != QLatin1String("Playing"));
            if (widget) {
                QString displayIp = m_deviceIp;
                if (displayIp.startsWith(QLatin1String("::ffff:"), Qt::CaseInsensitive)) {
                    displayIp = displayIp.mid(7);
                }
                widget->updateConnectionDisplay(m_online, displayIp);
            }
            if (wasOnline != m_online) {
                Q_EMIT connectedChanged(m_online);

            }
            if (prevIp != m_deviceIp) {
                Q_EMIT hostChanged(m_deviceIp);

            }
            publishStopped(stopped);
        }

        void publishStopped(bool stopped)
        {
            if (m_stopped == stopped && m_stoppedOutput) {
                return;
            }
            m_stopped = stopped;
            m_stoppedOutput = std::make_shared<VariableData>(QVariant(m_stopped));
            Q_EMIT dataUpdated(StoppedPort);
        }

        std::shared_ptr<VariableData> m_stoppedOutput;
        NDVController *m_controller = nullptr;

        int currentFileIndex = 0;
        int currentPlayerId = 1;
        bool m_online = false;
        bool m_stopped = false;
        QString m_deviceIp;

        QPointer<NDVPlayerInterface> widget{new NDVPlayerInterface()};
    };
}
