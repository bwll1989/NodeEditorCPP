#include "OscOutGroupModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
using namespace Nodes;
using namespace NodeDataTypes;

OscOutGroupModel::OscOutGroupModel(){
    InPortCount = 1;
    OutPortCount=0;
    CaptionVisible=true;
    Caption="OscOutGroup";
    WidgetEmbeddable=false;
    Resizable=true;
    connect(widget->testButton,&QPushButton::clicked,this,&OscOutGroupModel::trigger);
    NodeDelegateModel::registerOSCControl("/trigger",widget->testButton);
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
                widget->testButton->click();
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
void OscOutGroupModel::stateFeedBack(const QString& oscAddress,QVariant value)
{

    OSCMessage message;
    message.host = AppConstants::EXTRA_FEEDBACK_HOST;
    message.port = AppConstants::EXTRA_FEEDBACK_PORT;
    message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
    message.value = value;
    OSCSender::instance()->sendOSCMessageWithQueue(message);
}

void OscOutGroupModel::trigger()
{
    auto messages = widget->m_listWidget->getOSCMessages();
    for(auto message : messages){

        OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
}
