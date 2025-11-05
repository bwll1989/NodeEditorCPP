#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtCore/qglobal.h>
#include <QToolBox>
#include "BaseTrackListView.h"
#include "TimeLineNodeWidget.hpp"
#include "TimeLineNodeModel.h"
#include "BasePluginLoader.h"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"

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
class TimeLineDataModel : public NodeDelegateModel
{
Q_OBJECT

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
        Resizable= true;
        PortEditable=true;
        model = new TimeLineNodeModel();
        widget=new TimelineNodeWidget(model);;
        auto toolbar=dynamic_cast<TimeLineNodeToolBar*>(widget->toolbar);
        if (toolbar!= nullptr){
            auto allActions=toolbar->allActions();
            for (auto& action:allActions){
                // 将 QAction 映射到其对应的 QWidget（例如 QToolButton）
                QWidget* w = toolbar->widgetForAction(action.second);
                if (w) {
                    NodeDelegateModel::registerOSCControl("/"+action.first, w);
                }
            }
        }


    }

    ~TimeLineDataModel()
    {

        delete model;
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
                return "out";
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
                    case 0:
                        if (b)
                            model->onStartPlay();
                        else
                            model->onStopPlay();
                        break;
                    case 1:
                        if (b)
                            model->onStopPlay();
                        break;
                    case 2:
                        if (b)
                            model->onPausePlay();
                        break;
                    case 3:

                        widget->toolbar->allActions()["loop"]->setChecked(b);
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
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            model->load(v.toObject());
        }
    }
    QWidget *embeddedWidget() override{

        return widget;
    }

    void stateFeedBack(const QString& oscAddress,QVariant value) override {

        OSCMessage message;
        message.host = AppConstants::EXTRA_FEEDBACK_HOST;
        message.port = AppConstants::EXTRA_FEEDBACK_PORT;
        message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
        message.value = value;
        OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
private Q_SLOTS:

    void setTimeLineState(bool const &string)
    {
        Q_EMIT dataUpdated(0);
    }

private:
    TimeLineNodeModel* model;
    TimelineNodeWidget  *widget;
    unordered_map<unsigned int, QVariant> in_dictionary;
    unordered_map<unsigned int, QVariant> out_dictionary;
    QString value;
};
