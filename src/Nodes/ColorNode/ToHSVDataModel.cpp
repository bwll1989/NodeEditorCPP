#include "ToHSVDataModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
using namespace Nodes;
using namespace NodeDataTypes;
ToHSVDataModel::ToHSVDataModel(){
    InPortCount = 1;
    OutPortCount=3;
    CaptionVisible=true;
    Caption="To HSV";
    WidgetEmbeddable=false;
    Resizable=false;
}

ToHSVDataModel::~ToHSVDataModel(){

}

QtNodes::NodeDataType ToHSVDataModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch(portType){
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return VariableData().type();
        default:
            return VariableData().type();
    }
}

QString ToHSVDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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
                return "H";
            case 1:
                return "S";
            case 2:
                return "V";
            default:
                return "";
        }
    }
    return "";

}

std::shared_ptr<QtNodes::NodeData> ToHSVDataModel::outData(QtNodes::PortIndex port) {
        switch (port) {
            case 0:
                return std::make_shared<VariableData>(m_color.hsvHue());
            case 1:
                return std::make_shared<VariableData>(m_color.hsvSaturation());
            case 2:
                return std::make_shared<VariableData>(m_color.value());
            default:
                return nullptr;
        }
}

void ToHSVDataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;
    QVariant val = v->value();

    m_color=QColor(val.value<QColor>());

    onColorChanged(m_color);
}

void ToHSVDataModel::onColorChanged(const QColor& c) {
    m_color = c;
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);

}