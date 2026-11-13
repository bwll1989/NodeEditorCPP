#pragma once


#include <QtCore/QObject>

#include "NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QSignalBlocker>
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include <QtCore/qglobal.h>
#include <QToolBox>
#include "TimelineEngine.h"
#include "TimeLineNodeWidget.hpp"
#include "TimeLineNodeModel.h"
#include "TimelineInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/SyncData.h"
#include "Common/Devices/TimestampGenerator/TimestampGenerator.hpp"
#include <cmath>
using namespace NodeDataTypes;
using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class TimeLineDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool looping READ looping WRITE setLooping NOTIFY loopingChanged)
    Q_PROPERTY(qint64 currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY currentFrameChanged)

public:

    /**
     * 构造函数
     * - 初始化节点模型与界面
     * - 从工具栏提取所有动作，并将其对应的 QWidget 注册到 OSC 控制映射
     * 注意：registerOSCControl 只接受 QWidget*，需通过 widgetForAction 将 QAction 转换为控件
     */
    TimeLineDataModel(){
        // 静态库中的资源必须显式初始化一次（前缀 :/tlnode/icons，与旧 QtTimeLine 的 :/icons 隔离）
        static bool s_tlnodeResourcesInitialized = false;
        if (!s_tlnodeResourcesInitialized) {
            Q_INIT_RESOURCE(tlnode_resources);
            s_tlnodeResourcesInitialized = true;
        }

        InPortCount =5;    ///< 0=PLAY 1=STOP 2=PAUSE 3=LOOP 4=SYNC（跟随源）
        OutPortCount=1;    ///< 0=SYNC 发布（本时间线作为图内时钟源）
        CaptionVisible=true;
        Caption="TimeLineNode";
        Resizable= false;
        WidgetEmbeddable=false;
        PortEditable=false;
        model = new TimeLineNodeModel(this);

        // ── SYNC 双角色 ──
        // 主机侧：时钟源发布状态（播放头位置 + 全局帧锚点，供下游解码节点跟随）
        m_syncPublisher = std::make_shared<SyncSharedState>(QStringLiteral("TimeLineNode"));

        // 从机侧：轮询同步源播放态并驱动播放头（时间线作为从机跟随外部时钟）
        m_syncWatcher = new QTimer(this);
        m_syncWatcher->setInterval(40);   // 25Hz 轮询，位置纠偏粒度 1 帧
        connect(m_syncWatcher, &QTimer::timeout, this, &TimeLineDataModel::onSyncWatchTick);
        m_syncWatcher->start();

        widget = new Nodes::TimelineInterface(model);
        if (model && model->getClock()) {
            m_currentFrame = model->getClock()->getCurrentFrame();
            m_looping = model->getClock()->isLooping();
        }

        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "playing";
            AbstractDelegateModel::registerExternalBinding("/play", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "looping";
            AbstractDelegateModel::registerExternalBinding("/loop", this, b);
        }
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = "currentFrame";
            AbstractDelegateModel::registerExternalBinding("/currentFrame", this, b);
        }
        // connect(widget->startButton, &QPushButton::clicked, this, &TimeLineDataModel::setIsPlaying);
    }

    ~TimeLineDataModel() override
    {
        if (model) {
            model->onStopPlay();
        }
    }

    /**
     * 函数级注释：节点模型就绪回调
     * - GraphModel 已分配 NodeID 与 ParentAlias，可安全拼接 /dataflow/... 完整地址
     * - 同步设置 timeline 的 modelAlias，并让已加载的剪辑立刻完成控件注册与初始状态反馈
     */
    void afterModelReady() override {
        // 与 NodeDelegateModel::makeFullOscAddress 对齐：根层不插入空 parent 段
        // setModelAlias 会补前导 '/'，最终为 /dataflow/<nodeId> 或 /dataflow/<parent>/<nodeId>
        const QString parent = getParentAlias().trimmed();
        const QString alias = parent.isEmpty()
            ? QStringLiteral("dataflow/%1").arg(getNodeID())
            : QStringLiteral("dataflow/%1/%2").arg(parent).arg(getNodeID());
        model->setModelAlias(alias);

        auto* editor = widget ? widget->editorWidget() : nullptr;
        if (editor && editor->toolbar) {
            if (auto* tb = dynamic_cast<TimeLineNodeToolBar*>(editor->toolbar)) {
                tb->bindBus(getParentAlias(), getNodeID());
                connect(tb, &TimeLineNodeToolBar::setCurrentFrame, this, [this](qint64 frame) {
                    if (model && model->getClock()) {
                        model->getClock()->setCurrentFrame(frame);
                    }
                });
            }
        }

        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/pause"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/currentFrame"), this, SLOT(onGlobalEvent(GlobalEvent)));

        if (model && model->getClock()) {
            connect(model->getClock(), &TimeLineNodeClock::timecodePlayingChanged, this, [this](bool isPlaying) {
                if (m_playing == isPlaying) {
                    return;
                }
                m_playing = isPlaying;
                emit playingChanged(m_playing);
                stateFeedBack("/stop", !m_playing);
                publishSyncState();   // 播放态变化即时发布（恢复/定格）
            });

            connect(model->getClock(), &TimeLineNodeClock::loopingChanged, this, [this](bool isLooping) {
                if (m_looping == isLooping) {
                    return;
                }
                m_looping = isLooping;
                emit loopingChanged(m_looping);
                stateFeedBack("/loop", m_looping);
            });

            connect(model->getClock(), &TimeLineNodeClock::currentFrameChanged, this, [this](qint64 frame) {
                if (m_currentFrame == frame) {
                    return;
                }
                m_currentFrame = frame;
                emit currentFrameChanged(m_currentFrame);
                stateFeedBack("/currentFrame", m_currentFrame);
                publishSyncState();   // 主机发布：播放头心跳（tick/拖动/seek 均触发）
            });
        }

        stateFeedBack("/stop", !m_playing);

        for (TrackData* track : model->getTracks()) {
            if (!track) continue;
            for (TimeLineClipModel* clip : track->clips) {
                // 剪辑代理基类是本节点私有的 NodeClipDelegateModel（继承静态副本
                // TimeLineClipModel），不能用 BaseClass.dll 的 AbstractClipModel
                if (auto* delegate = dynamic_cast<NodeClipDelegateModel*>(clip)) {
                    delegate->onModelReady();
                }
            }
        }
    }

    bool playing() const { return m_playing; }
    void setPlaying(bool playing) {
        if (m_playing == playing) {
            return;
        }
        if (!model) {
            return;
        }
        if (playing) {
            model->onStartPlay();
        } else {
            model->onStopPlay();
        }
    }

    bool looping() const { return m_looping; }
    void setLooping(bool looping) {
        if (m_looping == looping) {
            return;
        }
        if (!model) {
            return;
        }
        model->onSetLoop(looping);
    }

    qint64 currentFrame() const { return m_currentFrame; }
    void setCurrentFrame(qint64 frame) {
        if (m_currentFrame == frame) {
            return;
        }
        if (!model || !model->getClock()) {
            return;
        }
        model->getClock()->setCurrentFrame(frame);
    }

public:
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
       ;
        switch (portType) {
            case PortType::In:
                switch(portIndex) {
                    case 0: return "PLAY";
                        case 1: return "STOP";
                        case 2: return "PAUSE";
                        case 3: return "LOOP";
                        case 4: return "SYNC";
                }
            case PortType::Out:
                return "SYNC";
            default:
                break;
        }
        return "";
    }

public:

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
            case PortType::In:
                return (portIndex == 4) ? SyncData().type() : VariableData().type();
            case PortType::Out:
                return SyncData().type();   ///< Out 0 = SYNC 发布（时钟源）
            case PortType::None:
                break;
            default:
                break;
        }

        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        // SYNC 发布端口：输出共享同步状态句柄（时间线作为图内时钟源）
        return std::make_shared<SyncData>(m_syncPublisher);
    }
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
        // SYNC 输入：绑定从机跟随源（断开时清空，回到本地时钟）
        if (portIndex == 4) {
            auto sd = (data != nullptr) ? std::dynamic_pointer_cast<SyncData>(data) : nullptr;
            m_syncSource = sd ? sd->getSharedState() : nullptr;
            return;
        }
        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariableData>(data)) {
            bool b = textData->asBool();
            switch (portIndex) {
            case 0: // PLAY
                setPlaying(b);
                break;
            case 1: // STOP
                if (b) setPlaying(false);
                break;
            case 2: // PAUSE
                if (b && model) model->onPausePlay();
                break;
            case 3: // LOOP
                setLooping(b);
                break;
            }
        }
        Q_EMIT dataUpdated(portIndex);
    }


    QJsonObject save() const override
    {
        QJsonObject modelJson1=model->save();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        
        // Save properties if needed, though model->save() might cover internal state.
        // Assuming model->save() covers timeline state.
        // We should save looping state if not covered by model->save().
        // Based on previous code, it only saved "values" -> model->save().
        
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            model->load(v.toObject());
        }
        // Sync local state from model after load?
        // Loop state might be in model.
    }
    QWidget *embeddedWidget() override{

        return widget;
    }

// signals:
//     void isPlayingChanged(bool playing);
//     void isLoopingChanged(bool looping);

private slots:
    void onGlobalEvent(const GlobalEvent& ev) {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }

        const QString addrPlay = makeFullOscAddress("/play");
        const QString addrStop = makeFullOscAddress("/stop");
        const QString addrPause = makeFullOscAddress("/pause");
        const QString addrLoop = makeFullOscAddress("/loop");
        const QString addrFrame = makeFullOscAddress("/currentFrame");

        if (ev.address == addrPlay) {
            const bool wantPlay = !ev.payload.isValid() || ev.payload.toBool();
            setPlaying(wantPlay);
            return;
        }
        if (ev.address == addrStop) {
            const bool wantStop = !ev.payload.isValid() || ev.payload.toBool();
            if (wantStop) {
                setPlaying(false);
            }
            return;
        }
        if (ev.address == addrPause) {
            const bool wantPause = !ev.payload.isValid() || ev.payload.toBool();
            if (wantPause && model) {
                model->onPausePlay();
            }
            return;
        }
        if (ev.address == addrLoop) {
            setLooping(ev.payload.toBool());
            return;
        }
        if (ev.address == addrFrame) {
            setCurrentFrame(ev.payload.toLongLong());
            return;
        }
    }

    // void setTimeLineState(bool const &string)
    // {
    //     Q_EMIT dataUpdated(0);
    // }

    /**
     * 函数级注释：主机发布——把时间线播放头写入共享同步状态
     * currentFrameChanged（tick/拖动/seek）与播放态变化时调用；
     * 位置=当前帧/帧率，锚点=写入时刻的全局帧计数，跟随端按全局时钟外推。
     */
    void publishSyncState() {
        if (!m_syncPublisher || !model || !model->getClock()) {
            return;
        }
        const double fps = model->getClock()->getFrameRate();
        if (fps <= 0.0) {
            return;
        }
        const double sec = double(m_currentFrame) / fps;
        m_syncPublisher->write(sec,
                               TimestampGenerator::getInstance()->getCurrentFrameCount(),
                               m_playing);
    }

private slots:
    /**
     * 函数级注释：从机跟随——轮询同步源并驱动时间线播放头
     * - 源开始播放且本节点未播 → 自动起播；源停止且本节点在播 → 自动停止
     * - 播放中：源期望位置换算为帧号，与本地播放头差 > 1 帧时 setCurrentFrame 纠偏
     *   （本地 Internal tick 继续推进填充帧间空隙，外部驱动为主、本地 tick 为辅）
     */
    void onSyncWatchTick() {
        if (!m_syncSource || !model || !model->getClock()) {
            return;
        }
        const SyncState snap = m_syncSource->read();
        if (!snap.active) {
            return;
        }
        auto* clock = model->getClock();
        // 播放态联动：源位置已超出时间线时长（非循环）时不起播，定格在末尾待命，
        // 避免"起播→tick 到末尾停止→又起播"的闪烁循环；源位置回拉后自动恢复跟随
        if (snap.playing != m_playing) {
            bool allowStart = true;
            if (snap.playing) {
                const qint64 maxFrames = clock->getMaxFrames();
                const double fps0 = clock->getFrameRate();
                if (!m_looping && maxFrames > 0 && fps0 > 0.0
                        && m_syncSource->expectedNowSec() * fps0 >= double(maxFrames)) {
                    allowStart = false;
                }
            }
            if (allowStart) {
                setPlaying(snap.playing);
            }
        }
        // 位置跟随（仅源在播时）
        if (!snap.playing) {
            return;
        }
        const double fps = clock->getFrameRate();
        if (fps <= 0.0) {
            return;
        }
        qint64 targetFrame = qint64(std::round(m_syncSource->expectedNowSec() * fps));
        const qint64 maxFrames = clock->getMaxFrames();
        if (maxFrames > 0) {
            if (targetFrame > maxFrames) {
                targetFrame = m_looping ? (targetFrame % (maxFrames + 1)) : maxFrames;
            }
        }
        if (targetFrame < 0) {
            targetFrame = 0;
        }
        if (std::abs(targetFrame - clock->getCurrentFrame()) > 1) {
            clock->setCurrentFrame(targetFrame);
        }
    }
    
    // void onClockPlayingChanged(bool playing) {
    //     if (m_isPlaying != playing) {
    //         m_isPlaying = playing;
    //         emit isPlayingChanged(m_isPlaying);

    //         if (widget && widget->timeline && widget->timeline->toolbar) {
    //             if (auto* tb = dynamic_cast<TimeLineNodeToolBar*>(widget->timeline->toolbar)) {
    //                 tb->setPlaybackState(m_isPlaying);
    //             }
    //         }

    //         if (widget && widget->startButton) {
    //             QSignalBlocker blocker(widget->startButton);
    //             widget->startButton->setChecked(m_isPlaying);
    //         }
    //     }
    // }

    /**
     * 函数级注释：时钟当前帧变化回调
     * - 像播放状态一样，通过工具栏向总线发布当前帧
    //  */
    // void onClockCurrentFrameChanged(int frame) {
    //     if (widget && widget->timeline && widget->timeline->toolbar) {
    //         if (auto* tb = dynamic_cast<TimeLineNodeToolBar*>(widget->timeline->toolbar)) {
    //             tb->publishCurrentFrame(frame);
    //         }
    //     }
    // }

private:
    TimeLineNodeModel* model;
    Nodes::TimelineInterface* widget;
    bool m_playing = false;
    bool m_looping = false;
    qint64 m_currentFrame = 0;

    // ---- 节点间同步（SyncData 体系）----
    std::shared_ptr<SyncSharedState> m_syncPublisher;  ///< 主机：发布时间线播放头
    std::shared_ptr<SyncSharedState> m_syncSource;     ///< 从机：外部同步源句柄
    QTimer* m_syncWatcher = nullptr;                   ///< 源轮询（启停联动 + 位置跟随）
    // unordered_map<unsigned int, QVariant> in_dictionary;
    // unordered_map<unsigned int, QVariant> out_dictionary;
    // QString value;

signals:
    void playingChanged(bool playing);
    void loopingChanged(bool looping);
    void currentFrameChanged(qint64 frame);

};
