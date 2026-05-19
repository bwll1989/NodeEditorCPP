#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QSignalBlocker>
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include <QtCore/qglobal.h>
#include <QToolBox>
#include "BaseTrackListView.h"
#include "TimeLineNodeWidget.hpp"
#include "TimeLineNodeModel.h"
#include "BasePluginLoader.h"
// #include "TimelineInterface.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "AbstractClipDelegateModel.h"
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
        InPortCount =4;
        OutPortCount=0;
        CaptionVisible=true;
        Caption="TimeLineNode";
        Resizable= false;
        WidgetEmbeddable=false;
        PortEditable=false;
        model = new TimeLineNodeModel(this);

        widget=new TimelineNodeWidget(model);
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
        model->setModelAlias("dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()));

        if (widget && widget->toolbar) {
            if (auto* tb = dynamic_cast<TimeLineNodeToolBar*>(widget->toolbar)) {
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
            });
        }

        stateFeedBack("/stop", !m_playing);

        for (TrackData* track : model->getTracks()) {
            if (!track) continue;
            for (AbstractClipModel* clip : track->clips) {
                if (auto* delegate = dynamic_cast<AbstractClipDelegateModel*>(clip)) {
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
                }
            case PortType::Out:
                return "OUT";
            default:
                break;
        }
        return "";
    }

public:

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            case PortType::None:
                break;
            default:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return VariableData().type();

    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VariableData>();
    }
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariableData>(data)) {
            if (textData->value().canConvert<bool>()) {
                bool b=textData->value().toBool();
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
    // Nodes::TimelineInterface  *widget;
    TimelineNodeWidget *widget;
    bool m_playing = false;
    bool m_looping = false;
    qint64 m_currentFrame = 0;
    // unordered_map<unsigned int, QVariant> in_dictionary;
    // unordered_map<unsigned int, QVariant> out_dictionary;
    // QString value;

signals:
    void playingChanged(bool playing);
    void loopingChanged(bool looping);
    void currentFrameChanged(qint64 frame);

};
