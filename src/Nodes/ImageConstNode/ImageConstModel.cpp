#include "ImageConstModel.hpp"
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
using namespace Nodes;
using namespace NodeDataTypes;
ImageConstModel::ImageConstModel(){
    InPortCount = 3;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Image Constant";
    WidgetEmbeddable=false;
    Resizable=false;
    m_watcher = new QFutureWatcher<void>(this);
    // 连接输入框变化信号到槽
    connect(widget->widthEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->heightEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorEditButton, &QPushButton::clicked, this, &ImageConstModel::toggleEditorMode);
    updateImage();
}

ImageConstModel::~ImageConstModel(){
    if (colorEditorWidget) {
        colorEditorWidget->setParent(nullptr);
        colorEditorWidget->deleteLater();
    }
}

QtNodes::NodeDataType ImageConstModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch(portType){
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return ImageData().type();
        default:
            return VariableData().type();
    }
}

QString ImageConstModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In)
    {
        switch (portIndex)
        {
        case 0:
            return "WIDTH";
        case 1:
            return "HEIGHT";
        case 2:
            return "RED";
        case 3:
            return "GREEN";
        case 4:
            return "BLUE";
        case 5:
            return "ALPHA";
        default:
            return "";
        }
    }else
    {
        return "Image";
    }
}

std::shared_ptr<QtNodes::NodeData> ImageConstModel::outData(QtNodes::PortIndex) {
    // 输出当前生成的图像
    if (m_image.empty()) return nullptr;
    return std::make_shared<ImageData>(m_image);
}

void ImageConstModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) {
    if (!nodeData) return;
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) return;
    QVariant val = v->value();
    switch (port) {
        case 0: // WIDTH
            m_width = val.toInt();
            widget->widthEdit->setText(QString::number(m_width));
            break;
        case 1: // HEIGHT
            m_height = val.toInt();
            widget->heightEdit->setText(QString::number(m_height));
            break;
        case 2: // RED
            colorEditorWidget->setColor(val.toString());

        default:
            break;
    }
    updateImage();
    Q_EMIT dataUpdated(0);
}

void ImageConstModel::onInputChanged() {
    // 解析输入框内容
    m_width = widget->widthEdit->text().toInt();
    m_height = widget->heightEdit->text().toInt();
    m_color = QColor(colorEditorWidget->getColor());
    updateImage();
    Q_EMIT dataUpdated(0);
}

void ImageConstModel::updateImage() {
    if (m_width <= 0) m_width = 1;
    if (m_height <= 0) m_height = 1;

    // // 立即预览当前颜色
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);

    // 异步生成图像
    auto width = m_width;
    auto height = m_height;
    auto color = m_color;
    QFuture<void> future = QtConcurrent::run([this, width, height, color]() {
        // 仅当尺寸变化时重新分配内存
        if (m_image.rows != height || m_image.cols != width) {
            m_image.create(height, width, CV_8UC4);
        }

        // 直接操作现有矩阵内存
        m_image.forEach<cv::Vec4b>([color](cv::Vec4b &pixel, const int*) {
            pixel = cv::Vec4b(
                color.blue(),
                color.green(),
                color.red(),
                color.alpha()
            );
        });
    });

    QObject::disconnect(m_watcher, nullptr, nullptr, nullptr);
    QObject::connect(m_watcher, &QFutureWatcher<void>::finished, this, [this]() {
        Q_EMIT dataUpdated(0);
    });
    m_watcher->setFuture(future);
}

QJsonObject ImageConstModel::save() const
{
    QJsonObject modelJson1;
    modelJson1["width"] = widget->widthEdit->text();
    modelJson1["height"] = widget->heightEdit->text();
    modelJson1["color"] = colorEditorWidget->getColor().name(QColor::HexArgb);
    // modelJson1["green"] = widget->colorGreenEdit->value();
    // modelJson1["blue"] = widget->colorBlueEdit->value();
    // modelJson1["alpha"] = widget->colorAlphaEdit->value();
    QJsonObject modelJson  = NodeDelegateModel::save();
    modelJson["values"]=modelJson1;
    return modelJson;
}
void ImageConstModel::load(const QJsonObject &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined()&&v.isObject()) {
        //            button->setChecked(v["val"].toBool(false));
        widget->widthEdit->setText(v["width"].toString());
        widget->heightEdit->setText(v["height"].toString());
        colorEditorWidget->setColor(QColor(v["color"].toString()));
        // widget->colorRedEdit->setValue(v["red"].toInt());
        // widget->colorGreenEdit->setValue(v["green"].toInt());
        // widget->colorBlueEdit->setValue(v["blue"].toInt());
        // widget->colorAlphaEdit->setValue(v["alpha"].toInt());
        updateImage();
    }
}

void ImageConstModel::toggleEditorMode() {
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