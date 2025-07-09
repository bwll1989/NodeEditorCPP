//
// Created by pablo on 3/9/24.
//

#include "CutImageModel.h"
#include "DataTypes/NodeDataList.hpp"
using namespace NodeDataTypes;
using namespace Nodes;
using namespace QtNodes;
CutImageModel::CutImageModel() {
    InPortCount =6;
    OutPortCount=1;
    CaptionVisible=true;
    Caption="CutImage";
    WidgetEmbeddable=false;
    Resizable=false;
    connect(widget->pos_x, &QLineEdit::textChanged, this, [this]() {
        m_outRect.setLeft(widget->pos_x->text().toInt());
        processImage();
    });
    connect(widget->pos_y, &QLineEdit::textChanged, this, [this]() {
        m_outRect.setTop(widget->pos_y->text().toInt());
        processImage();
    });
    connect(widget->widthEdit, &QLineEdit::textChanged, this, [this]() {
        m_outRect.setWidth(widget->widthEdit->text().toInt());
        processImage();
    });
    connect(widget->heightEdit, &QLineEdit::textChanged, this, [this]() {
        m_outRect.setHeight(widget->heightEdit->text().toInt());
        processImage();
    });
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

void CutImageModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) {
    switch (portIndex) {
        case 0:
            m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
            break;
        case 1:
            if (nodeData== nullptr){
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(nodeData)->value();
            m_outRect = m_inData.toRect();
            widget->pos_x->setText(QString::number(m_outRect.left()));
            widget->pos_y->setText(QString::number(m_outRect.top()));
            widget->widthEdit->setText(QString::number(m_outRect.width()));
            widget->heightEdit->setText(QString::number(m_outRect.height()));
            break;
        case 2:
            if (nodeData== nullptr){
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(nodeData)->value();
            m_outRect.setLeft(m_inData.toInt());
            widget->pos_x->setText(QString::number(m_inData.toInt()));
            break;
        case 3:
            if (nodeData== nullptr){
                return;
            }
            m_inData=std::dynamic_pointer_cast<VariableData>(nodeData)->value();
            m_outRect.setTop(m_inData.toInt());
            widget->pos_y->setText(QString::number(m_inData.toInt()));
            break;
        case 4:
            if (nodeData== nullptr){
                return;
            }
            m_inData=std::dynamic_pointer_cast<VariableData>(nodeData)->value();
            m_outRect.setWidth(m_inData.toInt());
            widget->widthEdit->setText(QString::number(m_inData.toInt()));
            break;
        case 5:
            if (nodeData== nullptr){
                return;
            }
            m_inData=std::dynamic_pointer_cast<VariableData>(nodeData)->value();
            m_outRect.setHeight(m_inData.toInt());
            widget->heightEdit->setText(QString::number(m_inData.toInt()));
    }
    processImage();
}

std::shared_ptr<QtNodes::NodeData> CutImageModel::outData(const QtNodes::PortIndex port) {
    return m_outImageData;
}

QWidget* CutImageModel::embeddedWidget() {
    return widget;
}

void CutImageModel::processImage() {
    if (m_outRect.size().width()<=0||m_outRect.size().height()<=0)
    {
        m_outImageData.reset();
        emit dataUpdated(0);
        return;
    }
    if (const auto lock = m_inImageData.lock()) {
        m_outImageData = std::make_shared<ImageData>(lock->image().copy(m_outRect));
    } else {
        m_outImageData.reset();
    }
    emit dataUpdated(0);
}

QJsonObject CutImageModel::save() const {


    QJsonObject modelJson  = NodeDelegateModel::save();
    QJsonObject modelJson1;
    modelJson1["values"]=modelJson1;
    modelJson1["pos_x"] = widget->pos_x->text().toInt();
    modelJson1["pos_y"] = widget->pos_y->text().toInt();
    modelJson1["width"] = widget->widthEdit->text().toInt();
    modelJson1["height"] = widget->heightEdit->text().toInt();
    modelJson["values"]=modelJson1;
    return modelJson;
}

void CutImageModel::load(const QJsonObject& data) {
    QJsonValue modelJson = data["values"];
    widget->pos_x->setText(QString::number(modelJson["pos_x"].toInt()));
    widget->pos_y->setText(QString::number(modelJson["pos_y"].toInt()));
    widget->widthEdit->setText(QString::number(modelJson["width"].toInt()));
    widget->heightEdit->setText(QString::number(modelJson["height"].toInt()));
    m_outRect.setLeft(modelJson["pos_x"].toInt());
    m_outRect.setTop(modelJson["pos_y"].toInt());
    m_outRect.setWidth(modelJson["width"].toInt());
    m_outRect.setHeight(modelJson["height"].toInt());
}