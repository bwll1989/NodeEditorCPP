#include "ToRGBAFloatDataModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
using namespace Nodes;
using namespace NodeDataTypes;
ToRGBAFloatDataModel::ToRGBAFloatDataModel(){
    InPortCount = 1;
    OutPortCount=4;
    CaptionVisible=true;
    Caption="To Float RGBA";
    WidgetEmbeddable=false;
    Resizable=false;
}

ToRGBAFloatDataModel::~ToRGBAFloatDataModel(){

}

QtNodes::NodeDataType ToRGBAFloatDataModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch(portType){
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return VariableData().type();
        default:
            return VariableData().type();
    }
}

QString ToRGBAFloatDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        switch (portIndex) {
            case 0:
                return "COLOR";
            default:
                return "";
        }
    }
    if (portType == QtNodes::PortType::Out) {
        switch (portIndex) {
            case 0:
                return "R";
            case 1:
                return "G";
            case 2:
                return "B";
            case 3:
                return "A";
            default:
                return "";
        }
    }
    return "";

}

std::shared_ptr<QtNodes::NodeData> ToRGBAFloatDataModel::outData(QtNodes::PortIndex port) {
        switch (port) {
            case 0:
                return std::make_shared<VariableData>(m_color.redF());
            case 1:
                return std::make_shared<VariableData>(m_color.greenF());
            case 2:
                return std::make_shared<VariableData>(m_color.blueF());
            case 3:
                return std::make_shared<VariableData>(m_color.alphaF());
            default:
                return nullptr;
        }
}

void ToRGBAFloatDataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;
    QVariant val = v->value();

    m_color=QColor(val.value<QColor>());

    onColorChanged(m_color);
}


void ToRGBAFloatDataModel::stateFeedBack(const QString& oscAddress,QVariant value){

    OSCMessage message;
    message.host = AppConstants::EXTRA_FEEDBACK_HOST;
    message.port = AppConstants::EXTRA_FEEDBACK_PORT;
    message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
    message.value = value;
    OSCSender::instance()->sendOSCMessageWithQueue(message);
}

void ToRGBAFloatDataModel::onColorChanged(const QColor& c) {
    m_color = c;
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);
    Q_EMIT dataUpdated(3);
}