#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSignalBlocker>
#include <QtNodes/NodeDelegateModel>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/FTOMVController/FTOMVController.h"
#include "OMVControllerInterface.hpp"
#include "PluginDefinition.hpp"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes
{
    /**
     * @brief OMV 安卓播放器控制节点：调用 FTOMVController 全局单例
     *
     * 输入：ID / play / stop / send
     * 输出：PLAYING
     */
    class OMVControllerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
        Q_PROPERTY(QString command READ command WRITE setCommand NOTIFY commandChanged)
        Q_PROPERTY(bool send READ send WRITE setSend NOTIFY sendChanged)
        Q_PROPERTY(bool play READ play WRITE setPlay NOTIFY playChanged)
        Q_PROPERTY(bool stop READ stop WRITE setStop NOTIFY stopChanged)
        Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
        Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)

    public:
        OMVControllerDataModel()
        {
            InPortCount = 4;
            OutPortCount = 1;
            CaptionVisible = true;
            PortEditable = false;
            Caption = QStringLiteral("FT-OMVController");
            WidgetEmbeddable = false;
            Resizable = false;

            m_playingData = std::make_shared<VariableData>(false);

            m_controller = FTOMVController::acquire();

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "deviceId";
                b.control = widget->deviceIdSpin;
                AbstractDelegateModel::registerExternalBinding("/deviceId", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "command";
                AbstractDelegateModel::registerExternalBinding("/command", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "send";
                b.control = widget->sendButton;
                AbstractDelegateModel::registerExternalBinding("/send", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "play";
                b.control = widget->playButton;
                AbstractDelegateModel::registerExternalBinding("/play", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "stop";
                b.control = widget->stopButton;
                AbstractDelegateModel::registerExternalBinding("/stop", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "playing";
                AbstractDelegateModel::registerExternalBinding("/playing", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "listening";
                AbstractDelegateModel::registerExternalBinding("/listening", this, b);
            }

            connect(widget->deviceIdSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
                setDeviceId(value);
            });
            // 界面输入即时同步到 m_command，保证 save() 能保存用户编辑
            connect(widget->jsonEdit, &QTextEdit::textChanged, this, [this]() {
                setCommand(widget->jsonEdit->toPlainText());
            });
            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                setCommand(widget->jsonEdit->toPlainText());
                setSend(true);
            });
            connect(widget->playButton, &QPushButton::clicked, this, [this]() {
                setPlay(true);
            });
            connect(widget->stopButton, &QPushButton::clicked, this, [this]() {
                setStop(true);
            });

            if (m_controller) {
                connect(m_controller, &FTOMVController::isReady, this, [this](bool ready) {
                    setListening(ready);
                });
                connect(m_controller, &FTOMVController::clientDisconnected, this,
                        [this](int id, const QString & /*host*/) {
                            if (id == m_deviceId) {
                                setPlaying(false);
                            }
                        });
                connect(m_controller, &FTOMVController::frameReceived, this,
                        &OMVControllerDataModel::onFrameReceived,
                        Qt::QueuedConnection);

                setListening(m_controller->isListening());
            }

            widget->deviceIdSpin->setValue(m_deviceId);
            widget->jsonEdit->setPlainText(m_command);
            widget->updatePlayingStatus(m_playing);
            updatePlayingOutput();
        }

        ~OMVControllerDataModel() override
        {
            if (m_controller) {
                FTOMVController::release();
                m_controller = nullptr;
            }
        }

        int deviceId() const { return m_deviceId; }
        QString command() const { return m_command; }
        bool send() const { return m_send; }
        bool play() const { return m_playPulse; }
        bool stop() const { return m_stopPulse; }
        bool playing() const { return m_playing; }
        bool listening() const { return m_listening; }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0:
                    return QStringLiteral("ID");
                case 1:
                    return QStringLiteral("play");
                case 2:
                    return QStringLiteral("stop");
                case 3:
                    return QStringLiteral("send");
                default:
                    break;
                }
                break;
            case PortType::Out:
                if (portIndex == 0) {
                    return QStringLiteral("PLAYING");
                }
                break;
            default:
                break;
            }
            return {};
        }

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            if (portIndex == 0) {
                return m_playingData;
            }
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (!varData) {
                return;
            }

            switch (portIndex) {
            case 0:
                setDeviceId(varData->value().toInt());
                break;
            case 1:
                if (varData->value().toBool()) {
                    setPlay(true);
                }
                break;
            case 2:
                if (varData->value().toBool()) {
                    setStop(true);
                }
                break;
            case 3:
                if (varData->value().toBool()) {
                    setSend(true);
                }
                break;
            default:
                break;
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values.insert(QStringLiteral("deviceId"), widget->deviceIdSpin->value());
            // 以界面当前文本为准，避免未触发同步时丢失用户输入
            values.insert(QStringLiteral("command"), widget->jsonEdit->toPlainText());
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson.insert(QStringLiteral("values"), values);
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject values = v.toObject();
            setDeviceId(values.value(QStringLiteral("deviceId")).toInt(m_deviceId));
            setCommand(values.value(QStringLiteral("command")).toString(m_command));
        }

    public slots:
        void setDeviceId(int id)
        {
            const int next = qMax(0, id);
            if (m_deviceId == next) {
                return;
            }
            m_deviceId = next;
            {
                const QSignalBlocker blocker(widget->deviceIdSpin);
                widget->deviceIdSpin->setValue(m_deviceId);
            }
            setPlaying(false);
            emit deviceIdChanged(m_deviceId);
        }

        void setCommand(const QString &cmd)
        {
            if (m_command == cmd) {
                return;
            }
            m_command = cmd;
            {
                const QSignalBlocker blocker(widget->jsonEdit);
                if (widget->jsonEdit->toPlainText() != m_command) {
                    widget->jsonEdit->setPlainText(m_command);
                }
            }
            emit commandChanged(m_command);
        }

        void setSend(bool value)
        {
            if (!value || m_send) {
                return;
            }

            m_send = true;
            emit sendChanged(true);
            sendCurrentCommand();
            m_send = false;
            emit sendChanged(false);
        }

        void setPlay(bool value)
        {
            if (!value || m_playPulse) {
                return;
            }
            m_playPulse = true;
            emit playChanged(true);
            // 播放状态以播放器返回数据为准，不在此乐观更新
            sendPlayStop(true);
            m_playPulse = false;
            emit playChanged(false);
        }

        void setStop(bool value)
        {
            if (!value || m_stopPulse) {
                return;
            }
            m_stopPulse = true;
            emit stopChanged(true);
            // 停止状态以播放器返回数据为准，不在此乐观更新
            sendPlayStop(false);
            m_stopPulse = false;
            emit stopChanged(false);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            if (ev.address == makeFullOscAddress("/deviceId")) {
                setDeviceId(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/command")) {
                setCommand(ev.payload.toString());
            } else if (ev.address == makeFullOscAddress("/send")) {
                setSend(ev.payload.toBool());
            } else if (ev.address == makeFullOscAddress("/play")) {
                setPlay(ev.payload.toBool());
            } else if (ev.address == makeFullOscAddress("/stop")) {
                setStop(ev.payload.toBool());
            }
        }

    Q_SIGNALS:
        void deviceIdChanged(int deviceId);
        void commandChanged(const QString &command);
        void sendChanged(bool value);
        void playChanged(bool value);
        void stopChanged(bool value);
        void playingChanged(bool playing);
        void listeningChanged(bool listening);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/deviceId"), this,
                                                  SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/command"), this,
                                                  SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/send"), this,
                                                  SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/play"), this,
                                                  SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/stop"), this,
                                                  SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        static QJsonObject makePlayJson()
        {
            QJsonObject obj;
            obj.insert(QStringLiteral("tp"), QStringLiteral("1"));
            obj.insert(QStringLiteral("com"), QStringLiteral("10"));
            obj.insert(QStringLiteral("pm"), QStringLiteral("10"));
            return obj;
        }

        static QJsonObject makeStopJson()
        {
            QJsonObject obj;
            obj.insert(QStringLiteral("tp"), QStringLiteral("1"));
            obj.insert(QStringLiteral("com"), QStringLiteral("10"));
            obj.insert(QStringLiteral("pm"), QStringLiteral("17"));
            return obj;
        }

        bool sendPlayStop(bool playCmd)
        {
            if (!m_controller) {
                return false;
            }
            const QJsonObject obj = playCmd ? makePlayJson() : makeStopJson();
            return m_controller->sendJson(m_deviceId, obj);
        }

        bool sendCurrentCommand()
        {
            if (!m_controller) {
                return false;
            }

            const QString text = widget->jsonEdit->toPlainText().trimmed();
            if (text != m_command) {
                setCommand(text);
            }
            if (m_command.trimmed().isEmpty()) {
                return false;
            }

            QJsonParseError err {};
            const QJsonDocument doc = QJsonDocument::fromJson(m_command.toUtf8(), &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) {
                qWarning() << "OMVController invalid JSON:" << err.errorString() << m_command;
                return false;
            }

            return m_controller->sendJson(m_deviceId, doc.object());
        }

        void setListening(bool ready)
        {
            if (m_listening == ready) {
                return;
            }
            m_listening = ready;
            widget->updateListeningStatus(ready);
            emit listeningChanged(ready);
        }

        void setPlaying(bool playing)
        {
            if (m_playing == playing) {
                return;
            }
            m_playing = playing;
            widget->updatePlayingStatus(playing);
            emit playingChanged(playing);
            updatePlayingOutput();
        }

        void updatePlayingOutput()
        {
            m_playingData = std::make_shared<VariableData>(m_playing);
            m_playingData->insert(QStringLiteral("deviceId"), m_deviceId);
            Q_EMIT dataUpdated(0);
        }

        void onFrameReceived(const QVariantMap &msg)
        {
            bool deviceIdOk = false;
            const int msgDeviceId = msg.value(QStringLiteral("deviceId")).toInt(&deviceIdOk);
            if (deviceIdOk && msgDeviceId >= 0 && msgDeviceId != m_deviceId) {
                return;
            }

            // 与 tset.lua 一致：仅根据播放器返回帧判定
            // hex[23:24]=='32' 且 hex[67:68]=='30'→播放 / '32'→停止
            const QString hex = msg.value(QStringLiteral("hex")).toString().toUpper();
            const QString typeDigit = msg.value(QStringLiteral("typeDigit")).toString();
            const bool isType2 = (typeDigit == QLatin1String("2"))
                || (hex.size() >= 24 && hex.mid(22, 2) == QLatin1String("32"));
            if (!isType2 || hex.size() < 68) {
                return;
            }

            const QString flag = hex.mid(66, 2);
            if (flag == QLatin1String("30")) {
                setPlaying(true);
            } else if (flag == QLatin1String("32")) {
                setPlaying(false);
            }
        }

        OMVControllerInterface *widget = new OMVControllerInterface();
        std::shared_ptr<VariableData> m_playingData;

        FTOMVController *m_controller = nullptr;
        int m_deviceId = 1;
        QString m_command = QStringLiteral("{\"tp\":\"1\",\"com\":\"10\",\"pm\":\"10\"}");
        bool m_send = false;
        bool m_playPulse = false;
        bool m_stopPulse = false;
        bool m_playing = false;
        bool m_listening = false;
    };
}
