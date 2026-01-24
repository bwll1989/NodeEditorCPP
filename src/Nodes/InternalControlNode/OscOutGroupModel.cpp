#include "OscOutGroupModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include "Common/Devices/OSCSender/OSCSender.h"

using namespace Nodes;
using namespace NodeDataTypes;

OscOutGroupModel::OscOutGroupModel(){
    InPortCount = 1;
    OutPortCount=0;
    CaptionVisible=true;
    Caption="Osc Out Group";
    WidgetEmbeddable=false;
    Resizable=true;
    connect(widget->testButton,&QPushButton::clicked,this,[this](){
        setTrigger(true);
    });
    AbstractDelegateModel::registerExternalControl("/trigger",widget->testButton);
}

QtNodes::NodeDataType OscOutGroupModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {

    return VariableData().type();

}

QString OscOutGroupModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In)
    {
        switch (portIndex)
        {
        case 0:
            return "TRIGGER";
        default:
            return "";
        }
    }else
    {
        return "Image";
    }
}

std::shared_ptr<QtNodes::NodeData> OscOutGroupModel::outData(QtNodes::PortIndex) {
    return std::make_shared<VariableData>();
}

void OscOutGroupModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;
    QVariant val = v->value();
    switch (port) {
        case 0: // Trigger
            if (val.toBool())
                setTrigger(true);
            break;
        default:
            break;
    }

}

QJsonObject OscOutGroupModel::save() const
{
    QJsonObject modelJson1;
    QJsonObject modelJson  = NodeDelegateModel::save();

    modelJson1 = widget->m_listWidget->save();
    modelJson["values"]=modelJson1;
    return modelJson;
}

void OscOutGroupModel::load(const QJsonObject &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined()&&v.isObject()) {
        widget->m_listWidget->load(v.toObject());
    }
}

ConnectionPolicy OscOutGroupModel::portConnectionPolicy(PortType portType, PortIndex index) const {
    auto result = ConnectionPolicy::One;
    switch (portType) {
        case PortType::In:
            result = ConnectionPolicy::Many;
            break;
        case PortType::Out:
            result = ConnectionPolicy::Many;
            break;
        case PortType::None:
            break;
    }

    return result;
}

/**
 * 函数级注释：触发一组 OSC 消息并通过事件总线反馈触发状态
 */
void OscOutGroupModel::setTrigger(bool value)
{
    if (!value) return;
    m_trigger = true;
    auto messages = widget->m_listWidget->getOSCMessages();
    for(auto message : messages){
        OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
    m_trigger = false;
    Q_EMIT triggerChanged(true);
    AbstractDelegateModel::stateFeedBack("/trigger", true);
}

/**
 * 函数级注释：处理来自全局事件总线的触发命令
 */
void OscOutGroupModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) return;
    const QString addrTrigger = makeFullOscAddress("/trigger");
    if (ev.address == addrTrigger) {
        setTrigger(ev.payload.toBool());
    }
}

/**
 * 函数级注释：模型就绪后订阅触发相关的全局命令
 */
void OscOutGroupModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/trigger"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}
