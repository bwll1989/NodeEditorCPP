#include "ColorFDataModel.hpp"
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace Nodes;
using namespace NodeDataTypes;

ColorFDataModel::ColorFDataModel(){
    InPortCount = 4;
    OutPortCount=5;
    CaptionVisible=true;
    Caption="Color Float";
    WidgetEmbeddable=false;
    Resizable=false;
    connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ColorFDataModel::onColorChanged);
    // connect(widget->colorEditButton, &QPushButton::clicked, this, &ColorFDataModel::toggleEditorMode);
    
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "red";
        b.control = colorEditorWidget->spinRed;
        AbstractDelegateModel::registerExternalBinding("/red", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "green";
        b.control = colorEditorWidget->spinGreen;
        AbstractDelegateModel::registerExternalBinding("/green", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "blue";
        b.control = colorEditorWidget->spinBlue;
        AbstractDelegateModel::registerExternalBinding("/blue", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "alpha";
        b.control = colorEditorWidget->spinAlpha;
        AbstractDelegateModel::registerExternalBinding("/alpha", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "hue";
        b.control = colorEditorWidget->spinHue;
        AbstractDelegateModel::registerExternalBinding("/hue", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "saturation";
        b.control = colorEditorWidget->spinSaturation;
        AbstractDelegateModel::registerExternalBinding("/saturation", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "value";
        b.control = colorEditorWidget->spinValue;
        AbstractDelegateModel::registerExternalBinding("/value", this, b);
    }
}

ColorFDataModel::~ColorFDataModel(){
    if (colorEditorWidget) {
        colorEditorWidget->setParent(nullptr);
        colorEditorWidget->deleteLater();
    }
}

QtNodes::NodeDataType ColorFDataModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch(portType){
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return VariableData().type();
        default:
            return VariableData().type();
    }
}

QString ColorFDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
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
    if (portType == QtNodes::PortType::Out) {
        switch (portIndex) {
            case 0:
                return "COLOR";
            case 1:
                return "RED";
            case 2:
                return "GREEN";
            case 3:
                return "BLUE";
            case 4:
                return "ALPHA";
            default:
                return "";
        }
    }
    return "";

}

std::shared_ptr<QtNodes::NodeData> ColorFDataModel::outData(QtNodes::PortIndex portIndex) {
        switch (portIndex) {
            case 0:
                return std::make_shared<VariableData>(m_color);
            case 1:
                return std::make_shared<VariableData>(m_color.redF());
            case 2:
                return std::make_shared<VariableData>(m_color.greenF());
            case 3:
                return std::make_shared<VariableData>(m_color.blueF());
            case 4:
                return std::make_shared<VariableData>(m_color.alphaF());
            default:
                return std::make_shared<VariableData>(m_color);
        }
}

void ColorFDataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;
    QVariant val = v->value();
    switch (port) {
        case 0: // WIDTH
            m_color.setRedF(val.toFloat());
            break;
        case 1: // GREEN
            m_color.setGreenF(val.toFloat());
            break;
        case 2: // BLUE
            m_color.setBlueF(val.toFloat());
            break;
        case 3: // ALPHA
            m_color.setAlphaF(val.toFloat());
            break;
        default:
            break;
    }
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

QJsonObject ColorFDataModel::save() const
{
    QJsonObject modelJson1;
    modelJson1["color"] = colorEditorWidget->getColor().name(QColor::HexArgb);
    QJsonObject modelJson  = NodeDelegateModel::save();
    modelJson["values"]=modelJson1;
    return modelJson;
}
void ColorFDataModel::load(const QJsonObject &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined()&&v.isObject()) {
        QColor c(v["color"].toString());
        {
            QSignalBlocker b(colorEditorWidget);
            colorEditorWidget->setColor(c);
        }
        onColorChanged(c);
    }
}

void ColorFDataModel::toggleEditorMode() {
    // 移除父子关系，使其成为独立窗口
    colorEditorWidget->setParent(nullptr);

    // 设置为独立窗口
    colorEditorWidget->setWindowTitle("颜色编辑器");

    // 设置窗口图标
    colorEditorWidget->setWindowIcon(QIcon(":/icons/icons/curve.png"));

    // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
    colorEditorWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

    // 设置窗口属性：当关闭时自动删除
    colorEditorWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
    colorEditorWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序

    // 设置窗口大小和显示
    colorEditorWidget->resize(800, 400);
    colorEditorWidget->show();
    // 激活窗口并置于前台
    colorEditorWidget->activateWindow();
    colorEditorWidget->raise();
}

void ColorFDataModel::updateDisplay() {
    // QPixmap pix(widget->display->width(), widget->display->height());
    // pix.fill(m_color);
    // widget->display->setPixmap(pix);
}

void ColorFDataModel::notifyAllPropertiesChanged() {
    Q_EMIT redChanged(m_color.red());
    Q_EMIT greenChanged(m_color.green());
    Q_EMIT blueChanged(m_color.blue());
    Q_EMIT alphaChanged(m_color.alpha());
    Q_EMIT hueChanged(m_color.hue());
    Q_EMIT saturationChanged(m_color.saturation());
    Q_EMIT valueChanged(m_color.value());
    
    AbstractDelegateModel::stateFeedBack("/red", m_color.red());
    AbstractDelegateModel::stateFeedBack("/green", m_color.green());
    AbstractDelegateModel::stateFeedBack("/blue", m_color.blue());
    AbstractDelegateModel::stateFeedBack("/alpha", m_color.alpha());
    AbstractDelegateModel::stateFeedBack("/hue", m_color.hue());
    AbstractDelegateModel::stateFeedBack("/saturation", m_color.saturation());
    AbstractDelegateModel::stateFeedBack("/value", m_color.value());
}

void ColorFDataModel::onColorChanged(const QColor& c) {
    m_color = c;
    updateDisplay();
    notifyAllPropertiesChanged();
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);
    Q_EMIT dataUpdated(3);
    Q_EMIT dataUpdated(4);
}

void ColorFDataModel::setRed(int r) {
    if (m_color.red() == r) return;
    m_color.setRed(r);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::setGreen(int g) {
    if (m_color.green() == g) return;
    m_color.setGreen(g);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::setBlue(int b) {
    if (m_color.blue() == b) return;
    m_color.setBlue(b);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::setAlpha(int a) {
    if (m_color.alpha() == a) return;
    m_color.setAlpha(a);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::setHue(int h) {
    if (m_color.hue() == h) return;
    m_color.setHsv(h, m_color.saturation(), m_color.value(), m_color.alpha());
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::setSaturation(int s) {
    if (m_color.saturation() == s) return;
    m_color.setHsv(m_color.hue(), s, m_color.value(), m_color.alpha());
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::setValue(int v) {
    if (m_color.value() == v) return;
    m_color.setHsv(m_color.hue(), m_color.saturation(), v, m_color.alpha());
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorFDataModel::afterModelReady() {
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/red"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/green"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/blue"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/alpha"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/hue"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/saturation"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

void ColorFDataModel::onGlobalEvent(const GlobalEvent& ev) {
    if (ev.kind != GlobalEventKind::Command) return;
    
    if (ev.address == makeFullOscAddress("/red")) {
        setRed(ev.payload.toInt());
    } else if (ev.address == makeFullOscAddress("/green")) {
        setGreen(ev.payload.toInt());
    } else if (ev.address == makeFullOscAddress("/blue")) {
        setBlue(ev.payload.toInt());
    } else if (ev.address == makeFullOscAddress("/alpha")) {
        setAlpha(ev.payload.toInt());
    } else if (ev.address == makeFullOscAddress("/hue")) {
        setHue(ev.payload.toInt());
    } else if (ev.address == makeFullOscAddress("/saturation")) {
        setSaturation(ev.payload.toInt());
    } else if (ev.address == makeFullOscAddress("/value")) {
        setValue(ev.payload.toInt());
    }
}
