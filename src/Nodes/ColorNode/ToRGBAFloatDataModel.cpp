#include "ToRGBAFloatDataModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace Nodes;
using namespace NodeDataTypes;

ToRGBAFloatDataModel::ToRGBAFloatDataModel(){
    InPortCount = 1;
    OutPortCount=4;
    CaptionVisible=true;
    Caption="To Float RGBA";
    WidgetEmbeddable=false;
    Resizable=false;

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "color";
        AbstractDelegateModel::registerExternalBinding("/color", this, b);
    }
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
    Q_UNUSED(port)
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;

    const QVariant val = v->value();
    if (val.canConvert<QColor>()) {
        setColor(val.value<QColor>());
    } else {
        const QColor c(val.toString());
        if (c.isValid()) {
            setColor(c);
        }
    }
}

QColor ToRGBAFloatDataModel::color() const
{
    return m_color;
}

void ToRGBAFloatDataModel::setColor(const QColor& c)
{
    if (m_color == c) {
        return;
    }
    m_color = c;
    Q_EMIT colorChanged(m_color);
    AbstractDelegateModel::stateFeedBack("/color", m_color);

    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);
    Q_EMIT dataUpdated(3);
}

void ToRGBAFloatDataModel::onColorChanged(const QColor& c) {
    setColor(c);
}

void ToRGBAFloatDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/color"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}

void ToRGBAFloatDataModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }
    if (ev.address != makeFullOscAddress("/color")) {
        return;
    }

    const QVariant v = ev.payload;
    if (v.canConvert<QColor>()) {
        setColor(v.value<QColor>());
    } else {
        const QColor c(v.toString());
        if (c.isValid()) {
            setColor(c);
        }
    }
}