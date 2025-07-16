#include "ImageConstModel.hpp"

#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
using namespace Nodes;
using namespace NodeDataTypes;
QFutureWatcher<QImage> *m_watcher = nullptr;
ImageConstModel::ImageConstModel(){
    InPortCount = 6;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Image Constant";
    WidgetEmbeddable=false;
    Resizable=false;
    m_watcher = new QFutureWatcher<QImage>(this);
    // 连接输入框变化信号到槽
    connect(widget->widthEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->heightEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorRedEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorGreenEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorBlueEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorAlphaEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    updateImage();
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
    if (m_image.isNull()) return nullptr;
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
            m_color.setRed(val.toInt());
            widget->colorRedEdit->setText(QString::number(m_color.red()));
            break;
        case 3: // GREEN
            m_color.setGreen(val.toInt());
            widget->colorGreenEdit->setText(QString::number(m_color.green()));
            break;
        case 4: // BLUE
            m_color.setBlue(val.toInt());
            widget->colorBlueEdit->setText(QString::number(m_color.blue()));
            break;
        case 5: // ALPHA
            m_color.setAlpha(val.toInt());
            widget->colorAlphaEdit->setText(QString::number(m_color.alpha()));
            break;
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
    int r = widget->colorRedEdit->text().toInt();
    int g = widget->colorGreenEdit->text().toInt();
    int b = widget->colorBlueEdit->text().toInt();
    int a = widget->colorAlphaEdit->text().isEmpty() ? 255 : widget->colorAlphaEdit->text().toInt();
    m_color = QColor(r, g, b, a);
    updateImage();
    Q_EMIT dataUpdated(0);
}

void ImageConstModel::updateImage() {
    if (m_width <= 0) m_width = 1;
    if (m_height <= 0) m_height = 1;
    // 立即预览当前颜色
    QPixmap pix(widget->display->width(), widget->display->height());
    pix.fill(m_color);
    widget->display->setPixmap(pix);
    // 异步生成图像
    auto width = m_width;
    auto height = m_height;
    auto color = m_color;
    QFuture<QImage> future = QtConcurrent::run([width, height, color]() {
        QImage img(width, height, QImage::Format_ARGB32);
        img.fill(color);
        return img;
    });
    QObject::disconnect(m_watcher, nullptr, nullptr, nullptr);
    QObject::connect(m_watcher, &QFutureWatcher<QImage>::finished, this, [this]() {
        m_image = m_watcher->result();
        Q_EMIT dataUpdated(0);
    });
    m_watcher->setFuture(future);
}

QJsonObject ImageConstModel::save() const
{
    QJsonObject modelJson1;
    modelJson1["width"] = widget->widthEdit->text();
    modelJson1["height"] = widget->heightEdit->text();
    modelJson1["red"] = widget->colorRedEdit->text();
    modelJson1["green"] = widget->colorGreenEdit->text();
    modelJson1["blue"] = widget->colorBlueEdit->text();
    modelJson1["alpha"] = widget->colorAlphaEdit->text();
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
        widget->colorRedEdit->setText(v["red"].toString());
        widget->colorGreenEdit->setText(v["green"].toString());
        widget->colorBlueEdit->setText(v["blue"].toString());
        widget->colorAlphaEdit->setText(v["alpha"].toString());
        updateImage();
    }
}