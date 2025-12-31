#include "InternalControlModel.hpp"
#include "PluginDefinition.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "StatusContainer/StatusContainer.h"
using namespace Nodes;
using namespace NodeDataTypes;

InternalControlModel::InternalControlModel(){
    InPortCount = 1;
    OutPortCount=0;
    CaptionVisible=true;
    Caption=PLUGIN_NAME;
    WidgetEmbeddable=false;
    Resizable=true;
    connect(widget->testButton,&QPushButton::clicked,this,&InternalControlModel::trigger);
    AbstractDelegateModel::registerOSCControl("/trigger",widget->testButton);
}

QtNodes::NodeDataType InternalControlModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {

    return VariableData().type();

}

QString InternalControlModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

std::shared_ptr<QtNodes::NodeData> InternalControlModel::outData(QtNodes::PortIndex) {
    return std::make_shared<VariableData>();
}

void InternalControlModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
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

QJsonObject InternalControlModel::save() const
{
    QJsonObject modelJson1;
    QJsonObject modelJson  = NodeDelegateModel::save();

    modelJson1= widget->m_listWidget->save();
    modelJson["values"]=modelJson1;
    return modelJson;
}

void InternalControlModel::load(const QJsonObject &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined()&&v.isObject()) {
        widget->m_listWidget->load(v.toObject());
    }
}

ConnectionPolicy InternalControlModel::portConnectionPolicy(PortType portType, PortIndex index) const {
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

void InternalControlModel::trigger()
{
    auto messages = widget->m_listWidget->getOSCMessages();
    for(auto message : messages){
        StatusContainer::instance()->parseOSC(message);
        // OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
}
