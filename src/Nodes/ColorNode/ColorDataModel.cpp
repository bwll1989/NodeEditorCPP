#include "ColorDataModel.hpp"
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace Nodes;
using namespace NodeDataTypes;

ColorDataModel::ColorDataModel(){
    InPortCount = 4;
    OutPortCount=5;
    CaptionVisible=true;
    Caption="Color";
    WidgetEmbeddable=false;
    Resizable=false;
    connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ColorDataModel::onColorChanged);
    connect(widget->colorEditButton, &QPushButton::clicked, this, &ColorDataModel::toggleEditorMode);
    
    AbstractDelegateModel::registerExternalControl("/red", colorEditorWidget->spinRed);
    AbstractDelegateModel::registerExternalControl("/green", colorEditorWidget->spinGreen);
    AbstractDelegateModel::registerExternalControl("/blue", colorEditorWidget->spinBlue);
    AbstractDelegateModel::registerExternalControl("/alpha", colorEditorWidget->spinAlpha);
    AbstractDelegateModel::registerExternalControl("/hue", colorEditorWidget->spinHue);
    AbstractDelegateModel::registerExternalControl("/saturation", colorEditorWidget->spinSaturation);
    AbstractDelegateModel::registerExternalControl("/value", colorEditorWidget->spinValue);
}

ColorDataModel::~ColorDataModel(){
    if (colorEditorWidget) {
        colorEditorWidget->setParent(nullptr);
        colorEditorWidget->deleteLater();
    }
}

QtNodes::NodeDataType ColorDataModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch(portType){
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return VariableData().type();
        default:
            return VariableData().type();
    }
}

QString ColorDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
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

std::shared_ptr<QtNodes::NodeData> ColorDataModel::outData(QtNodes::PortIndex portIndex) {
        switch (portIndex) {
            case 0:
                return std::make_shared<VariableData>(m_color);
            case 1:
                return std::make_shared<VariableData>(m_color.red());
            case 2:
                return std::make_shared<VariableData>(m_color.green());
            case 3:
                return std::make_shared<VariableData>(m_color.blue());
            case 4:
                return std::make_shared<VariableData>(m_color.alpha());
            default:
                return std::make_shared<VariableData>(m_color);
        }
}

void ColorDataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;
    QVariant val = v->value();
    switch (port) {
        case 0: // WIDTH (RED)
            m_color.setRed(val.toInt());
            break;
        case 1: // GREEN
            m_color.setGreen(val.toInt());
            break;
        case 2: // BLUE
            m_color.setBlue(val.toInt());
            break;
        case 3: // ALPHA
            m_color.setAlpha(val.toInt());
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

QJsonObject ColorDataModel::save() const
{
    QJsonObject modelJson1;
    modelJson1["color"] = colorEditorWidget->getColor().name(QColor::HexArgb);
    QJsonObject modelJson  = NodeDelegateModel::save();
    modelJson["values"]=modelJson1;
    return modelJson;
}
void ColorDataModel::load(const QJsonObject &p)
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

void ColorDataModel::toggleEditorMode() {
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

void ColorDataModel::updateDisplay() {
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);
}

void ColorDataModel::notifyAllPropertiesChanged() {
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

void ColorDataModel::onColorChanged(const QColor& c) {
    m_color = c;
    updateDisplay();
    notifyAllPropertiesChanged();
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);
    Q_EMIT dataUpdated(3);
    Q_EMIT dataUpdated(4);
}

void ColorDataModel::setRed(int r) {
    if (m_color.red() == r) return;
    m_color.setRed(r);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::setGreen(int g) {
    if (m_color.green() == g) return;
    m_color.setGreen(g);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::setBlue(int b) {
    if (m_color.blue() == b) return;
    m_color.setBlue(b);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::setAlpha(int a) {
    if (m_color.alpha() == a) return;
    m_color.setAlpha(a);
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::setHue(int h) {
    if (m_color.hue() == h) return;
    // QColor::setHsv preserves others?
    // Using setHsv to update.
    // Note: setHsv(h, s, v, a)
    m_color.setHsv(h, m_color.saturation(), m_color.value(), m_color.alpha());
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::setSaturation(int s) {
    if (m_color.saturation() == s) return;
    m_color.setHsv(m_color.hue(), s, m_color.value(), m_color.alpha());
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::setValue(int v) {
    if (m_color.value() == v) return;
    m_color.setHsv(m_color.hue(), m_color.saturation(), v, m_color.alpha());
    {
        QSignalBlocker b(colorEditorWidget);
        colorEditorWidget->setColor(m_color);
    }
    onColorChanged(m_color);
}

void ColorDataModel::afterModelReady() {
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/red"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/green"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/blue"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/alpha"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/hue"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/saturation"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
}

void ColorDataModel::onGlobalEvent(const GlobalEvent& ev) {
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
