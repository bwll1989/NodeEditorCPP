#include "ToHSVDataModel.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace Nodes;
using namespace NodeDataTypes;

ToHSVDataModel::ToHSVDataModel(){
    InPortCount = 1;
    OutPortCount=3;
    CaptionVisible=true;
    Caption="To HSV";
    WidgetEmbeddable=false;
    Resizable=false;

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "color";
        AbstractDelegateModel::registerExternalBinding("/color", this, b);
    }
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

QColor ToHSVDataModel::color() const
{
    return m_color;
}

void ToHSVDataModel::setColor(const QColor& c)
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
}

void ToHSVDataModel::onColorChanged(const QColor& c) {
    setColor(c);
}

void ToHSVDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/color"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}

void ToHSVDataModel::onGlobalEvent(const GlobalEvent& ev)
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