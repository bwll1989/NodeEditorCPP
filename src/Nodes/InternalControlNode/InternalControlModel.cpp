#include "InternalControlModel.hpp"
#include "PluginDefinition.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
using namespace Nodes;
using namespace NodeDataTypes;

InternalControlModel::InternalControlModel(){
    InPortCount = 1;
    OutPortCount=0;
    CaptionVisible=true;
    Caption=PLUGIN_NAME;
    WidgetEmbeddable=false;
    Resizable=false;
    connect(widget->testButton,&QPushButton::clicked,this,&InternalControlModel::trigger);
    NodeDelegateModel::registerOSCControl("/trigger",widget->testButton);
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

    modelJson1["messages"] = widget->m_listWidget->save();
    modelJson["values"]=modelJson1;
    return modelJson;
}

void InternalControlModel::load(const QJsonObject &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined()&&v.isObject()) {
        widget->m_listWidget->load(v["messages"].toObject());
    }
}

void InternalControlModel::trigger()
{

    auto messages = widget->m_listWidget->getOSCMessages();
    for(auto message : messages){

        OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
}
