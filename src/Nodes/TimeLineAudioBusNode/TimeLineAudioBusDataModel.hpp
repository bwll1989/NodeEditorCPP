#pragma once

#include <atomic>
#include <memory>

#include <QtCore/QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>

#include "QtNodes/NodeDelegateModel"

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioData.h"
#include "Common/DataTypes/SyncData.h"
#include "Common/DataTypes/TimelineAudioBusRegistry.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include "TimeLineAudioBusWorker.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;

namespace Nodes
{

/**
 * @brief TimeLineAudioBus —— timeline 音频进入节点图的入口节点
 *
 * 无输入端口。输出：
 *  - 0 号口：SyncData（共享 SyncSharedState，持续发布 timeline 播放头快照）；
 *  - 1..N 号口：AudioData（共享 AudioTimestampRingQueue，bus 名 = 端口名）。
 *
 * 队列归属：每个 Audio 端口一个环形队列，出生点在节点内部，创建即注册进
 * TimelineAudioBusRegistry（runtime/timeline 混音侧按名写入——单写者铁律，
 * driveMode=external 时节点自身不写）。
 *
 * 端口编辑：PortEditable=true，右键/Ctrl+P 增删输出端口（框架能力），
 * 端口数随存档持久化。0 号口为 SYNC，勿删（删后首口即视为音频口）。
 *
 * 时间基准：TimestampGenerator 全局帧计数；音频块打戳 = 帧计数 +
 * FIXED_DELAY_FRAMES（与全系统源节点同口径，音画同拍根基）。
 */
class TimeLineAudioBusDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(int audioPorts READ audioPortsProperty WRITE setAudioPortsProperty NOTIFY audioPortsChanged)
    Q_PROPERTY(QString driveMode READ driveModeProperty WRITE setDriveModeProperty NOTIFY driveModeChanged)
    Q_PROPERTY(bool followClock READ followClockProperty WRITE setFollowClockProperty NOTIFY followClockChanged)
    Q_PROPERTY(double positionSec READ positionSecProperty WRITE setPositionSecProperty NOTIFY positionSecChanged)
    Q_PROPERTY(bool playing READ playingProperty WRITE setPlayingProperty NOTIFY playingChanged)

public:
    TimeLineAudioBusDataModel()
    {
        InPortCount = 0;
        OutPortCount = 3;   // 0 = SYNC，1..2 = 音频 bus
        CaptionVisible = true;
        Caption = "TimeLine Audio Bus";
        WidgetEmbeddable = false;
        Resizable = true;
        PortEditable = true;

        // 主机侧：创建同步发布端，交给 worker 每拍写入
        m_syncPublisher = std::make_shared<SyncSharedState>(QStringLiteral("TimeLineAudioBus"));
        worker_->setSyncPublisher(m_syncPublisher);

        // 默认两个音频 bus（BUS 1 / BUS 2）
        worker_->reconcilePortCount(OutPortCount);

        connect(this, &TimeLineAudioBusDataModel::audioPortsChanged, this,
                [this](int) { worker_->reconcilePortCount(OutPortCount); });
        connect(this, &TimeLineAudioBusDataModel::driveModeChanged, this,
                [this](const QString& m) { worker_->setDriveMode(m); });
        connect(this, &TimeLineAudioBusDataModel::followClockChanged, this,
                [this](bool f) { worker_->setFollowClock(f); });
        connect(this, &TimeLineAudioBusDataModel::positionSecChanged, this,
                [this](double s) { worker_->setPositionSec(s); });
        connect(this, &TimeLineAudioBusDataModel::playingChanged, this,
                [this](bool p) { worker_->setPlaying(p); });

        worker_->startProcessing();
    }

    ~TimeLineAudioBusDataModel() override
    {
        // 先停线程再注销 bus，防止 worker 写已注销队列
        worker_->stopProcessing();
        auto* registry = TimelineAudioBusRegistry::instance();
        const QStringList names = registry->buses();
        for (unsigned int i = 1; i <= OutPortCount; ++i) {
            registry->unregisterBus(QStringLiteral("BUS %1").arg(i));
        }
        Q_UNUSED(names);
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return (portIndex == 0) ? SyncData().type() : AudioData().type();
        }
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        if (port == 0) {
            return std::make_shared<SyncData>(m_syncPublisher);
        }
        auto audioData = std::make_shared<AudioData>();
        audioData->setSharedAudioBuffer(worker_->queueForPort(port));
        return audioData;
    }

    void setInData(std::shared_ptr<NodeData>, PortIndex) override
    {
        // 无输入端口
    }

    QString portCaption(PortType portType, PortIndex portIndex) const override
    {
        if (portType == PortType::Out) {
            return (portIndex == 0) ? QStringLiteral("SYNC")
                                    : QStringLiteral("BUS %1").arg(portIndex);
        }
        return QString();
    }

    QWidget* embeddedWidget() override { return nullptr; }

    QJsonObject save() const override
    {
        QJsonObject values;
        values["audioPorts"] = audioPortsProperty();
        values["driveMode"] = driveModeProperty();
        values["followClock"] = followClockProperty();
        values["positionSec"] = positionSecProperty();
        values["playing"] = playingProperty();

        QJsonObject json = NodeDelegateModel::save();
        json["values"] = values;
        return json;
    }

    void load(const QJsonObject& p) override
    {
        NodeDelegateModel::load(p);
        QJsonObject values = p.value(QStringLiteral("values")).toObject();
        if (values.contains(QStringLiteral("audioPorts"))) {
            setAudioPortsProperty(values.value(QStringLiteral("audioPorts")).toInt());
        }
        if (values.contains(QStringLiteral("driveMode"))) {
            setDriveModeProperty(values.value(QStringLiteral("driveMode")).toString());
        }
        if (values.contains(QStringLiteral("followClock"))) {
            setFollowClockProperty(values.value(QStringLiteral("followClock")).toBool());
        }
        if (values.contains(QStringLiteral("positionSec"))) {
            setPositionSecProperty(values.value(QStringLiteral("positionSec")).toDouble());
        }
        if (values.contains(QStringLiteral("playing"))) {
            setPlayingProperty(values.value(QStringLiteral("playing")).toBool());
        }
    }

public slots:
    int audioPortsProperty() const
    {
        return (OutPortCount > 0) ? static_cast<int>(OutPortCount - 1) : 0;
    }

    void setAudioPortsProperty(int count)
    {
        count = qBound(1, count, 16);
        if (count == audioPortsProperty()) {
            return;
        }
        OutPortCount = static_cast<unsigned int>(count + 1); // + SYNC 口
        Q_EMIT audioPortsChanged(count);
    }

    QString driveModeProperty() const { return m_driveMode; }

    void setDriveModeProperty(const QString& mode)
    {
        if (mode == m_driveMode) {
            return;
        }
        m_driveMode = mode;
        worker_->setDriveMode(mode);
        // 驱动权同步到注册表（单写者协调，2026-11-13 T1）：非 external
        // 模式下节点 worker 是唯一写者，runtime host 侧据此跳过这些 bus
        auto* registry = TimelineAudioBusRegistry::instance();
        for (unsigned int i = 1; i <= OutPortCount; ++i) {
            registry->setBusDriveMode(QStringLiteral("BUS %1").arg(i), mode);
        }
        Q_EMIT driveModeChanged(mode);
    }

    bool followClockProperty() const { return m_followClock; }

    void setFollowClockProperty(bool follow)
    {
        if (follow == m_followClock) {
            return;
        }
        m_followClock = follow;
        worker_->setFollowClock(follow);
        Q_EMIT followClockChanged(follow);
    }

    double positionSecProperty() const { return m_positionSec; }

    void setPositionSecProperty(double sec)
    {
        if (qFuzzyCompare(sec + 1.0, m_positionSec + 1.0)) {
            return;
        }
        m_positionSec = sec;
        worker_->setPositionSec(sec);
        Q_EMIT positionSecChanged(sec);
    }

    bool playingProperty() const { return m_playing; }

    void setPlayingProperty(bool playing)
    {
        if (playing == m_playing) {
            return;
        }
        m_playing = playing;
        worker_->setPlaying(playing);
        Q_EMIT playingChanged(playing);
    }

signals:
    void audioPortsChanged(int count);
    void driveModeChanged(const QString& mode);
    void followClockChanged(bool follow);
    void positionSecChanged(double sec);
    void playingChanged(bool playing);

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/position"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress("/driveMode"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private slots:
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address == makeFullOscAddress("/play")) {
            setPlayingProperty(ev.payload.toBool());
        } else if (ev.address == makeFullOscAddress("/position")) {
            setFollowClockProperty(false);
            setPositionSecProperty(ev.payload.toDouble());
        } else if (ev.address == makeFullOscAddress("/driveMode")) {
            setDriveModeProperty(ev.payload.toString());
        }
    }

private:
    QString m_driveMode = QStringLiteral("external");
    bool m_followClock = true;
    double m_positionSec = 0.0;
    bool m_playing = false;

    std::shared_ptr<SyncSharedState> m_syncPublisher;          ///< 0 号口同步发布端
    std::shared_ptr<TimeLineAudioBusWorker> worker_ =
        std::make_shared<TimeLineAudioBusWorker>();
};

} // namespace Nodes
