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
#include "TimelineInterface.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
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
    Q_PROPERTY(bool isPlaying READ getIsPlaying WRITE setIsPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(bool isLooping READ getIsLooping WRITE setIsLooping NOTIFY isLoopingChanged)

public:

    /**
     * 构造函数
     * - 初始化节点模型与界面
     * - 从工具栏提取所有动作，并将其对应的 QWidget 注册到 OSC 控制映射
     * 注意：registerOSCControl 只接受 QWidget*，需通过 widgetForAction 将 QAction 转换为控件
     */
    TimeLineDataModel(){
        InPortCount =4;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="TimeLineNode";
        WidgetEmbeddable= false;
        Resizable= false;
        PortEditable=true;
        model = new TimeLineNodeModel();
        widget=new Nodes::TimelineInterface(model);
        auto toolbar=dynamic_cast<TimeLineNodeToolBar*>(widget->timeline->toolbar);
        if (toolbar!= nullptr){
            auto allActions=toolbar->allActions();

            // 将 QAction 映射到其对应的 QWidget（例如 QToolButton）
            QWidget* w = toolbar->widgetForAction(allActions["play"]);
            if (w) {
               AbstractDelegateModel::registerExternalControl("/play", w);
            }
            w = toolbar->widgetForAction(allActions["loop"]);
            if (w) {
               AbstractDelegateModel::registerExternalControl("/loop", w);
            }
            w = toolbar->widgetForAction(allActions["stop"]);
            if (w) {
               AbstractDelegateModel::registerExternalControl("/stop", w);
            }
        }
        AbstractDelegateModel::registerExternalControl("/play", widget->startButton);
        connect(widget->startButton, &QPushButton::clicked, this, &TimeLineDataModel::setIsPlaying);
    }

    ~TimeLineDataModel() override
    {
        if (model) {
            model->onStopPlay();
        }
    }

    void afterModelReady() override {
        // Subscribe to GlobalEventBus
        GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
        // Connect clock signals to sync internal state and provide feedback
        if (model && model->getClock()) {
            connect(model->getClock(), &TimeLineNodeClock::timecodePlayingChanged, this, &TimeLineDataModel::onClockPlayingChanged);
        }
    }

    // Getters and Setters
    bool getIsPlaying() const { return m_isPlaying; }
    void setIsPlaying(bool playing) {
        if (m_isPlaying == playing) return;
        
        if (playing) {
            model->onStartPlay();
            if (widget && widget->timeline && widget->timeline->toolbar) {
                auto actions = widget->timeline->toolbar->allActions();
                {
                    QSignalBlocker blocker(actions["play"]); // Action might not be a widget, QSignalBlocker works on QObject
                    actions["play"]->setChecked(playing);
                }
                {
                    QSignalBlocker blocker(widget->startButton); // Action might not be a widget, QSignalBlocker works on QObject
                    widget->startButton->setChecked(playing);

                }
            }
        } else {
            model->onStopPlay();

        }
        // m_isPlaying will be updated by onClockPlayingChanged
    }

    bool getIsLooping() const { return m_isLooping; }
    void setIsLooping(bool looping) {
        if (m_isLooping == looping) return;
        m_isLooping = looping;
        
        model->onSetLoop(m_isLooping);
        
        // Sync UI
        if (widget && widget->timeline && widget->timeline->toolbar) {
            auto actions = widget->timeline->toolbar->allActions();
            {
                 QSignalBlocker blocker(actions["loop"]); // Action might not be a widget, QSignalBlocker works on QObject
                 actions["loop"]->setChecked(m_isLooping);
            }
        }

        emit isLoopingChanged(m_isLooping);
        AbstractDelegateModel::stateFeedBack("/loop", m_isLooping);
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
                        setIsPlaying(b);
                        break;
                    case 1: // STOP
                        if (b) setIsPlaying(false);
                        break;
                    case 2: // PAUSE
                        if (b) model->onPausePlay(); // No property for pause yet, treat as action
                        break;
                    case 3: // LOOP
                        setIsLooping(b);
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

signals:
    void isPlayingChanged(bool playing);
    void isLoopingChanged(bool looping);

public slots:
    void onGlobalEvent(const GlobalEvent& ev) {
        if (ev.kind == GlobalEventKind::Command) {
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "play" || localPath == "start") {
                if (ev.payload.isValid()) setIsPlaying(ev.payload.toBool());
                else setIsPlaying(true);
            }
            else if (localPath == "stop") setIsPlaying(false);
            else if (localPath == "pause") model->onPausePlay();
            else if (localPath == "loop") setIsLooping(ev.payload.toBool());
        }
    }

private slots:

    void setTimeLineState(bool const &string)
    {
        Q_EMIT dataUpdated(0);
    }
    
    void onClockPlayingChanged(bool playing) {
        if (m_isPlaying != playing) {
            m_isPlaying = playing;
            emit isPlayingChanged(m_isPlaying);
            AbstractDelegateModel::stateFeedBack("/play", m_isPlaying);
        }
    }

private:
    TimeLineNodeModel* model;
    Nodes::TimelineInterface  *widget;
    unordered_map<unsigned int, QVariant> in_dictionary;
    unordered_map<unsigned int, QVariant> out_dictionary;
    QString value;
    
    bool m_isPlaying = false;
    bool m_isLooping = false;
};
