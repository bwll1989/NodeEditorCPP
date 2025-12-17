#include "ColorDataModel.hpp"
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
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
    NodeDelegateModel::registerOSCControl("/red", colorEditorWidget->spinRed);
    NodeDelegateModel::registerOSCControl("/green", colorEditorWidget->spinGreen);
    NodeDelegateModel::registerOSCControl("/blue", colorEditorWidget->spinBlue);
    NodeDelegateModel::registerOSCControl("/alpha", colorEditorWidget->spinAlpha);
    NodeDelegateModel::registerOSCControl("/hue", colorEditorWidget->spinHue);
    NodeDelegateModel::registerOSCControl("/saturation", colorEditorWidget->spinSaturation);
    NodeDelegateModel::registerOSCControl("/value", colorEditorWidget->spinValue);
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
        case 0: // WIDTH
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
    colorEditorWidget->setColor(m_color);
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
        colorEditorWidget->setColor(QColor(v["color"].toString()));
    }
}
void ColorDataModel::stateFeedBack(const QString& oscAddress,QVariant value){

    OSCMessage message;
    message.host = AppConstants::EXTRA_FEEDBACK_HOST;
    message.port = AppConstants::EXTRA_FEEDBACK_PORT;
    message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
    message.value = value;
    OSCSender::instance()->sendOSCMessageWithQueue(message);
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

void ColorDataModel::onColorChanged(const QColor& c) {
    m_color = c;
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);
    Q_EMIT dataUpdated(3);
    Q_EMIT dataUpdated(4);
}