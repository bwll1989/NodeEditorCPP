//
// Created by pablo on 3/9/24.
//
//

#include "CutImageModel.h"
#include "DataTypes/NodeDataList.hpp"
#include <QSignalBlocker>
using namespace NodeDataTypes;
using namespace Nodes;
using namespace QtNodes;

// 函数级注释：构造函数，初始化端口、标题，并将界面控件与属性系统绑定
// - 通过 QLineEdit 的 textChanged 信号更新属性，从而触发图像裁剪与状态反馈
// - 使用 registerExternalControl 注册控制接口，便于统一外部控制入口
CutImageModel::CutImageModel() {
    InPortCount =6;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="CutImage";
    WidgetEmbeddable=false;
    Resizable=false;
    NodeDelegateModel::ExternalBinding ui;
    ui.member="topLeftX";
    AbstractDelegateModel::registerExternalBinding("/topLeftX", this, ui);
    // AbstractDelegateModel::registerExternalControl("/topLeftX",widget->pos_x);
    NodeDelegateModel::ExternalBinding ui1;
    ui1.member="topLeftY";
    AbstractDelegateModel::registerExternalBinding("/topLeftY", this, ui1);
    // AbstractDelegateModel::registerExternalControl("/topLeftY",widget->pos_y);
    NodeDelegateModel::ExternalBinding ui3;
    ui3.member="width";
    AbstractDelegateModel::registerExternalBinding("/width", this, ui3);
    // AbstractDelegateModel::registerExternalControl("/width",widget->widthEdit);
    NodeDelegateModel::ExternalBinding ui4;
    ui4.member="height";
    AbstractDelegateModel::registerExternalBinding("/height", this, ui4);
    // AbstractDelegateModel::registerExternalControl("/height",widget->heightEdit);
}

QString CutImageModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    switch(portType)
    {
    case PortType::In:
        switch(portIndex)
        {
            case 0:
                    return "IMAGE";
            case 1:
                    return "RECT";
            case 2:
                    return "POS_X";
            case 3:
                    return "POS_Y";
            case 4:
                    return "WIDTH";
            case 5:
                    return "HEIGHT";
            default:
            return "";
        }
    case PortType::Out:
        return "IMAGE";
    default:
        return "";
    }

}
QtNodes::NodeDataType CutImageModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    switch (portType) {
        case QtNodes::PortType::In:
            switch (portIndex) {
                case 0:
                    return ImageData().type();
                default:
                    return VariableData().type();
            }
        case QtNodes::PortType::Out:
            return ImageData().type();
        default:
            return VariableData().type();
    }
}

// 函数级注释：根据不同输入端口更新输入图像或裁剪参数，并委托给属性接口
void CutImageModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) {
    switch (portIndex) {
        case 0:
            m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
            processImage();
            break;
        case 1: {
            if (nodeData == nullptr) {
                return;
            }
            const QRect rect = std::dynamic_pointer_cast<VariableData>(nodeData)->value().toRect();
            setTopLeftX(rect.left());
            setTopLeftY(rect.top());
            setWidth(rect.width());
            setHeight(rect.height());
            break;
        }
        case 2:
            if (nodeData == nullptr) {
                return;
            }
            setTopLeftX(std::dynamic_pointer_cast<VariableData>(nodeData)->value().toInt());
            break;
        case 3:
            if (nodeData == nullptr) {
                return;
            }
            setTopLeftY(std::dynamic_pointer_cast<VariableData>(nodeData)->value().toInt());
            break;
        case 4:
            if (nodeData == nullptr) {
                return;
            }
            setWidth(std::dynamic_pointer_cast<VariableData>(nodeData)->value().toInt());
            break;
        case 5:
            if (nodeData == nullptr) {
                return;
            }
            setHeight(std::dynamic_pointer_cast<VariableData>(nodeData)->value().toInt());
            break;
    }
}

std::shared_ptr<QtNodes::NodeData> CutImageModel::outData(const QtNodes::PortIndex port) {
    return m_outImageData;
}

// 函数级注释：模型就绪后订阅全局事件总线，实现外部命令控制裁剪参数
void CutImageModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/topLeftX"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/topLeftY"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/width"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/height"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}

void CutImageModel::processImage() {
    if (m_width <= 0 || m_height <= 0) {
        m_outImageData.reset();
        emit dataUpdated(0);
        return;
    }

    if (const auto lock = m_inImageData.lock()) {
        const cv::Mat inputMat = lock->imgMat();

        // 边界检查
        cv::Rect validRect(
            std::max(0, m_topLeftX),
            std::max(0, m_topLeftY),
            std::min(inputMat.cols - m_topLeftX, m_width),
            std::min(inputMat.rows - m_topLeftY, m_height)
        );

        if (validRect.width <= 0 || validRect.height <= 0) {
            m_outImageData.reset();
            return;
        }

        // 使用OpenCV进行ROI剪裁
        cv::Mat croppedMat = inputMat(validRect).clone();
        m_outImageData = std::make_shared<ImageData>(croppedMat);
    } else {
        m_outImageData.reset();
    }
    emit dataUpdated(0);
}

QJsonObject CutImageModel::save() const {


    QJsonObject modelJson  = NodeDelegateModel::save();
    QJsonObject modelJson1;
    modelJson1["pos_x"] = topLeftX();
    modelJson1["pos_y"] = topLeftY();
    modelJson1["width"] = width();
    modelJson1["height"] = height();
    modelJson["values"]=modelJson1;
    return modelJson;
}

void CutImageModel::load(const QJsonObject& data) {
    QJsonValue modelJson = data["values"];
    const int x = modelJson["pos_x"].toInt();
    const int y = modelJson["pos_y"].toInt();
    const int w = modelJson["width"].toInt();
    const int h = modelJson["height"].toInt();

    setTopLeftX(x);
    setTopLeftY(y);
    setWidth(w);
    setHeight(h);
}

// 函数级注释：设置裁剪矩形左上角 X 坐标属性，并更新界面与状态
void CutImageModel::setTopLeftX(int x)
{
    if (m_topLeftX== x) {
        return;
    }
    m_topLeftX = x;

    processImage();
    Q_EMIT topLeftXChanged(x);

}

// 函数级注释：设置裁剪矩形左上角 Y 坐标属性，并更新界面与状态
void CutImageModel::setTopLeftY(int y)
{
    if (m_topLeftY == y) {
        return;
    }
    m_topLeftY = y;

    processImage();
    Q_EMIT topLeftYChanged(y);

}

// 函数级注释：设置裁剪矩形宽度属性，并更新界面与状态
void CutImageModel::setWidth(int w)
{
    if (m_width == w) {
        return;
    }
    m_width = w;
    processImage();
    Q_EMIT widthChanged(w);
    // AbstractDelegateModel::stateFeedBack("/width", w);
}

// 函数级注释：设置裁剪矩形高度属性，并更新界面与状态
void CutImageModel::setHeight(int h)
{
    if (m_height == h) {
        return;
    }
    m_height = h;
    processImage();
    Q_EMIT heightChanged(h);
    // AbstractDelegateModel::stateFeedBack("/height", h);
}
// 函数级注释：处理来自全局事件总线的裁剪命令，映射到对应属性
void CutImageModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString addrX = makeFullOscAddress("/topLeftX");
    const QString addrY = makeFullOscAddress("/topLeftY");
    const QString addrW = makeFullOscAddress("/width");
    const QString addrH = makeFullOscAddress("/height");

    if (ev.address == addrX) {
        setTopLeftX(ev.payload.toInt());
    } else if (ev.address == addrY) {
        setTopLeftY(ev.payload.toInt());
    } else if (ev.address == addrW) {
        setWidth(ev.payload.toInt());
    } else if (ev.address == addrH) {
        setHeight(ev.payload.toInt());
    }
}
