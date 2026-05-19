#include "ToRGBADataModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace Nodes;
using namespace NodeDataTypes;

ToRGBADataModel::ToRGBADataModel(){
    InPortCount = 1;
    OutPortCount=4;
    CaptionVisible=true;
    Caption="To RGBA";
    WidgetEmbeddable=false;
    Resizable=false;

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "color";
        AbstractDelegateModel::registerExternalBinding("/color", this, b);
    }
}

ToRGBADataModel::~ToRGBADataModel(){

}

QtNodes::NodeDataType ToRGBADataModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch(portType){
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return VariableData().type();
        default:
            return VariableData().type();
    }
}

QString ToRGBADataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

std::shared_ptr<QtNodes::NodeData> ToRGBADataModel::outData(QtNodes::PortIndex port) {
        switch (port) {
            case 0:
                return std::make_shared<VariableData>(m_color.red());
            case 1:
                return std::make_shared<VariableData>(m_color.green());
            case 2:
                return std::make_shared<VariableData>(m_color.blue());
            case 3:
                return std::make_shared<VariableData>(m_color.alpha());
            default:
                return nullptr;
        }
}

void ToRGBADataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
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

QColor ToRGBADataModel::color() const
{
    return m_color;
}

void ToRGBADataModel::setColor(const QColor& c)
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

void ToRGBADataModel::onColorChanged(const QColor& c) {
    setColor(c);
}

void ToRGBADataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/color"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}

void ToRGBADataModel::onGlobalEvent(const GlobalEvent& ev)
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