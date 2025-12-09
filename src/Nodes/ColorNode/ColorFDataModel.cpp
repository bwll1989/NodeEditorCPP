#include "ColorFDataModel.hpp"
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
using namespace Nodes;
using namespace NodeDataTypes;
ColorFDataModel::ColorFDataModel(){
    InPortCount = 4;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Color Float";
    WidgetEmbeddable=false;
    Resizable=false;
    connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ColorFDataModel::onColorChanged);
    connect(widget->colorEditButton, &QPushButton::clicked, this, &ColorFDataModel::toggleEditorMode);
    NodeDelegateModel::registerOSCControl("/red", colorEditorWidget->spinRed);
    NodeDelegateModel::registerOSCControl("/green", colorEditorWidget->spinGreen);
    NodeDelegateModel::registerOSCControl("/blue", colorEditorWidget->spinBlue);
    NodeDelegateModel::registerOSCControl("/alpha", colorEditorWidget->spinAlpha);
    NodeDelegateModel::registerOSCControl("/hue", colorEditorWidget->spinHue);
    NodeDelegateModel::registerOSCControl("/saturation", colorEditorWidget->spinSaturation);
    NodeDelegateModel::registerOSCControl("/value", colorEditorWidget->spinValue);
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
        return "RGB";
    }
    return "";

}

std::shared_ptr<QtNodes::NodeData> ColorFDataModel::outData(QtNodes::PortIndex) {
        return std::make_shared<VariableData>(m_color);
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
    colorEditorWidget->setColor(m_color);
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
        colorEditorWidget->setColor(QColor(v["color"].toString()));
    }
}
void ColorFDataModel::stateFeedBack(const QString& oscAddress,QVariant value){

    OSCMessage message;
    message.host = AppConstants::EXTRA_FEEDBACK_HOST;
    message.port = AppConstants::EXTRA_FEEDBACK_PORT;
    message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
    message.value = value;
    OSCSender::instance()->sendOSCMessageWithQueue(message);
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

void ColorFDataModel::onColorChanged(const QColor& c) {
    m_color = c;
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);
    Q_EMIT dataUpdated(0);
}