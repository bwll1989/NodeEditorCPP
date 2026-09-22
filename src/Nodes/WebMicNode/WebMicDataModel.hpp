#pragma once

#include "PluginDefinition.hpp"
#include "WebMicInterface.h"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/Devices/WebSocketServer/WebSocketServer.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtWebSockets/QWebSocket>
#include <QJsonObject>
#include <QSignalBlocker>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    /**
     * @brief 网页对讲麦克风：独立 WebSocket 收 mono/48k float32 PCM，输出 AudioData
     */
    class WebMicDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(double gainDb READ gainDb WRITE setGainDb NOTIFY gainDbChanged)
        Q_PROPERTY(bool talking READ talking NOTIFY talkingChanged)

    public:
        static constexpr int kSampleRate = 48000;
        static constexpr int kDefaultPort = WebMicInterface::kDefaultPort;

        WebMicDataModel()
        {
            InPortCount = 0;
            OutPortCount = 2; // Mic + TALKING
            CaptionVisible = true;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            m_audioBuffer = std::make_shared<AudioTimestampRingQueue>();
            m_audioBuffer->setActive(true);
            m_talkingData = std::make_shared<VariableData>(false);

            server = new WebSocketServer();

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "port";
                b.control = widget->portSpin;
                AbstractDelegateModel::registerExternalBinding("/port", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "gainDb";
                b.control = widget->gainSpin;
                AbstractDelegateModel::registerExternalBinding("/gain", this, b);
            }

            connect(widget->portSpin, &IntDragValueWidget::valueChanged, this, [this](int v) {
                setPort(v);
            });
            connect(widget->gainSpin, &FloatDragValueWidget::valueChanged, this, [this](double v) {
                setGainDb(v);
            });

            connect(server, &WebSocketServer::newConnection, this, [this](QWebSocket *socket) {
                m_activeClient = socket;
                resetStreamState();
                setTalking(true);
                updateStatusLabel();
            }, Qt::QueuedConnection);

            connect(server, &WebSocketServer::connectionClosed, this, [this](QWebSocket *socket) {
                if (socket == m_activeClient) {
                    m_activeClient = nullptr;
                    // 断开前把不足一帧的尾巴补零播出，避免松开时最后一小段被丢掉
                    flushPendingTail();
                    resetStreamState();
                    setTalking(false);
                    updateStatusLabel();
                }
            }, Qt::QueuedConnection);

            connect(server, &WebSocketServer::messageReceived, this,
                    [this](QWebSocket *socket, const QByteArray &message) {
                        if (m_activeClient && socket != m_activeClient) {
                            return;
                        }
                        if (!m_activeClient) {
                            m_activeClient = socket;
                            resetStreamState();
                            setTalking(true);
                            updateStatusLabel();
                        }
                        ingestPcm(message);
                    },
                    Qt::QueuedConnection);

            server->start(static_cast<quint16>(m_port));
            updateStatusLabel();
        }

        ~WebMicDataModel() override
        {
            if (server) {
                server->stop();
                delete server;
                server = nullptr;
            }
        }

        int port() const { return m_port; }

        void setPort(int port)
        {
            if (port < 1 || port > 65535) {
                return;
            }
            if (m_port == port) {
                return;
            }
            m_port = port;
            {
                QSignalBlocker blocker(widget->portSpin);
                widget->portSpin->setValue(m_port);
            }
            if (server) {
                server->start(static_cast<quint16>(m_port));
            }
            m_activeClient = nullptr;
            resetStreamState();
            setTalking(false);
            updateStatusLabel();
            Q_EMIT portChanged(m_port);
        }

        double gainDb() const { return m_gainDb; }

        void setGainDb(double db)
        {
            if (qFuzzyCompare(db + 1.0, m_gainDb + 1.0)) {
                return;
            }
            m_gainDb = db;
            {
                QSignalBlocker blocker(widget->gainSpin);
                widget->gainSpin->setValue(m_gainDb);
            }
            if (m_gainDb <= -60.0) {
                m_gainLinear = 0.0f;
            } else {
                m_gainLinear = static_cast<float>(std::pow(10.0, m_gainDb / 20.0));
            }
            Q_EMIT gainDbChanged(m_gainDb);
        }

        bool talking() const { return m_talking; }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::Out) {
                if (portIndex == 1) {
                    return VariableData().type();
                }
                return AudioData().type();
            }
            return AudioData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::Out) {
                switch (portIndex) {
                case 0: return QStringLiteral("Mic");
                case 1: return QStringLiteral("TALKING");
                default: break;
                }
            }
            return {};
        }

        std::shared_ptr<NodeData> outData(PortIndex port) override
        {
            if (port == 1) {
                if (!m_talkingData) {
                    m_talkingData = std::make_shared<VariableData>(m_talking);
                }
                return m_talkingData;
            }
            if (!m_audioBuffer) {
                m_audioBuffer = std::make_shared<AudioTimestampRingQueue>();
                m_audioBuffer->setActive(true);
            }
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(m_audioBuffer);
            return audioData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(data)
            Q_UNUSED(portIndex)
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override
        {
            QJsonObject values;
            values["port"] = m_port;
            values["gainDb"] = m_gainDb;
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            NodeDelegateModel::load(p);
            QJsonObject values = p.value(QStringLiteral("values")).toObject();
            if (values.isEmpty()) {
                values = p;
            }
            if (values.contains(QStringLiteral("port"))) {
                setPort(values.value(QStringLiteral("port")).toInt(kDefaultPort));
            }
            if (values.contains(QStringLiteral("gainDb"))) {
                setGainDb(values.value(QStringLiteral("gainDb")).toDouble(0.0));
            }
        }

    signals:
        void portChanged(int port);
        void gainDbChanged(double db);
        void talkingChanged(bool talking);

    protected:
        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/gain"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private slots:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address == makeFullOscAddress("/port")) {
                setPort(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/gain")) {
                setGainDb(ev.payload.toDouble());
            }
        }

    private:
        void setTalking(bool talking)
        {
            if (m_talking == talking) {
                return;
            }
            m_talking = talking;
            m_talkingData = std::make_shared<VariableData>(m_talking);
            Q_EMIT dataUpdated(1);
            Q_EMIT talkingChanged(m_talking);
        }

        void updateStatusLabel()
        {
            if (!widget) {
                return;
            }
            if (m_talking) {
                widget->setStatusText(QStringLiteral("Talking" ));
            } else {
                widget->setStatusText(QStringLiteral("Listening"));
            }
        }

        void resetStreamState()
        {
            m_pending.clear();
            m_nextTimestamp = 0;
        }

        /** 连接结束时，把不足一帧的残留样点补静音后写成最后一帧。 */
        void flushPendingTail()
        {
            if (m_pending.empty()) {
                return;
            }
            emitPendingFrames();
            if (m_pending.empty()) {
                return;
            }
            const int frameSamples =
                TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate);
            if (frameSamples <= 0) {
                return;
            }
            m_pending.resize(static_cast<size_t>(frameSamples), 0.0f);
            emitPendingFrames();
        }

        void ingestPcm(const QByteArray &bytes)
        {
            if (bytes.isEmpty() || (bytes.size() % static_cast<int>(sizeof(float))) != 0) {
                return;
            }

            const auto *samples = reinterpret_cast<const float *>(bytes.constData());
            const int count = bytes.size() / static_cast<int>(sizeof(float));
            m_pending.reserve(m_pending.size() + static_cast<size_t>(count));
            for (int i = 0; i < count; ++i) {
                float s = samples[i] * m_gainLinear;
                if (s > 1.0f) {
                    s = 1.0f;
                } else if (s < -1.0f) {
                    s = -1.0f;
                }
                m_pending.push_back(s);
            }

            emitPendingFrames();
        }

        void emitPendingFrames()
        {
            const int frameSamples =
                TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate);
            if (frameSamples <= 0) {
                return;
            }

            if (!m_audioBuffer) {
                m_audioBuffer = std::make_shared<AudioTimestampRingQueue>();
                m_audioBuffer->setActive(true);
            }

            // 网络抖动提前量按「固定时长」换算成帧数（随全局时间戳帧率变化）。
            // AudioDeviceOut 按「全局时间戳」取帧：必须写成连续递增的时间戳，
            // 不能每帧都写 now+5，否则同一时刻多帧撞同一戳会被环形队列互相覆盖。
            constexpr double kLeadMs = 240.0;
            const double fps = TimestampGenerator::getInstance()->getFrameRate();
            qint64 leadFrames = (fps > 0.0)
                ? static_cast<qint64>(std::lround(kLeadMs * fps / 1000.0))
                : 12;
            if (leadFrames < 2) {
                leadFrames = 2;
            }
            while (static_cast<int>(m_pending.size()) >= frameSamples) {
                const qint64 now = TimestampGenerator::getInstance()->getCurrentFrameCount();
                if (m_nextTimestamp <= 0) {
                    // 本段对讲的第一帧：从「当前全局时钟 + 提前量」开始排号
                    m_nextTimestamp = now + leadFrames;
                } else if (m_nextTimestamp < now + 2) {
                    // 网络卡顿后赶上时，下一戳已落后于播放时钟（写进过去会被丢弃）
                    // → 重新锚定到 now + 提前量，宁可跳一截也不要静音
                    m_nextTimestamp = now + leadFrames;
                }

                AudioFrame frame;
                frame.sampleRate = kSampleRate;
                frame.channels = 1;
                frame.bitsPerSample = 32;
                frame.timestamp = m_nextTimestamp++; // 每产出一帧，时间戳 +1
                frame.data.resize(frameSamples * static_cast<int>(sizeof(float)));
                std::memcpy(frame.data.data(), m_pending.data(),
                            static_cast<size_t>(frameSamples) * sizeof(float));
                m_pending.erase(m_pending.begin(),
                                m_pending.begin() + frameSamples);
                m_audioBuffer->pushFrame(frame);
            }
        }

        WebMicInterface *widget = new WebMicInterface();
        WebSocketServer *server = nullptr;
        std::shared_ptr<AudioTimestampRingQueue> m_audioBuffer;
        std::shared_ptr<VariableData> m_talkingData;
        QWebSocket *m_activeClient = nullptr;
        std::vector<float> m_pending;
        qint64 m_nextTimestamp = 0;

        int m_port = kDefaultPort;
        double m_gainDb = 0.0;
        float m_gainLinear = 1.0f;
        bool m_talking = false;
    };
}
