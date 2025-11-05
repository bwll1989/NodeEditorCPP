#include "ImageConstModel.hpp"

#include <QtNodes/NodeDelegateModelRegistry>
#include <QtWidgets/QFileDialog>
using namespace Nodes;
using namespace NodeDataTypes;
ImageConstModel::ImageConstModel(){
    InPortCount = 6;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="Image Constant";
    WidgetEmbeddable=false;
    Resizable=false;
    m_watcher = new QFutureWatcher<void>(this);
    // 连接输入框变化信号到槽
    connect(widget->widthEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->heightEdit, &QLineEdit::textChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorRedEdit, &QSpinBox::valueChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorGreenEdit, &QSpinBox::valueChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorBlueEdit, &QSpinBox::valueChanged, this, &ImageConstModel::onInputChanged);
    connect(widget->colorAlphaEdit, &QSpinBox::valueChanged, this, &ImageConstModel::onInputChanged);
    NodeDelegateModel::registerOSCControl("/r",widget->colorRedEdit);
    NodeDelegateModel::registerOSCControl("/g",widget->colorGreenEdit);
    NodeDelegateModel::registerOSCControl("/b",widget->colorBlueEdit);
    NodeDelegateModel::registerOSCControl("/a",widget->colorAlphaEdit);
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
            widget->colorRedEdit->setValue(val.toInt()>=255?255:val.toInt());
            break;
        case 3: // GREEN
            widget->colorGreenEdit->setValue(val.toInt()>=255?255:val.toInt());
            break;
        case 4: // BLUE
            widget->colorBlueEdit->setValue(val.toInt()>=255?255:val.toInt());
            break;
        case 5: // ALPHA
            widget->colorAlphaEdit->setValue(val.toInt()>=255?255:val.toInt());
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
    int r = widget->colorRedEdit->value();
    int g = widget->colorGreenEdit->value();
    int b = widget->colorBlueEdit->value();
    int a = widget->colorAlphaEdit->value();
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
    modelJson1["red"] = widget->colorRedEdit->value();
    modelJson1["green"] = widget->colorGreenEdit->value();
    modelJson1["blue"] = widget->colorBlueEdit->value();
    modelJson1["alpha"] = widget->colorAlphaEdit->value();
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
        widget->colorRedEdit->setValue(v["red"].toInt());
        widget->colorGreenEdit->setValue(v["green"].toInt());
        widget->colorBlueEdit->setValue(v["blue"].toInt());
        widget->colorAlphaEdit->setValue(v["alpha"].toInt());
        updateImage();
    }
}
void ImageConstModel::stateFeedBack(const QString& oscAddress,QVariant value){

    OSCMessage message;
    message.host = AppConstants::EXTRA_FEEDBACK_HOST;
    message.port = AppConstants::EXTRA_FEEDBACK_PORT;
    message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
    message.value = value;
    OSCSender::instance()->sendOSCMessageWithQueue(message);
}