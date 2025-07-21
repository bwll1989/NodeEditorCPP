#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QAbstractScrollArea>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "ImageThresholdInterface.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class ImageThresholdDataModel : public NodeDelegateModel
    {
        Q_OBJECT

        public:
        ImageThresholdDataModel()
        {
            InPortCount =4;
            OutPortCount=1;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_outImage=std::make_shared<ImageData>();
            connect(widget->threshEdit,&QSpinBox::valueChanged,this,&ImageThresholdDataModel::updateImage);
            connect(widget->maxvalEdit,&QSpinBox::valueChanged,this,&ImageThresholdDataModel::updateImage);
            connect(widget->methodEdit,&QComboBox::currentIndexChanged,this,&ImageThresholdDataModel::updateImage);

            NodeDelegateModel::registerOSCControl("/method",widget->methodEdit);
            NodeDelegateModel::registerOSCControl("/thresh",widget->threshEdit);
            NodeDelegateModel::registerOSCControl("/maxval",widget->maxvalEdit);
        }

        virtual ~ImageThresholdDataModel() override{}

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "IMAGE";
                case 1:
                    return "THRESHOLD";
                case 2:
                    return "MAXVAL";
                case 3:
                    return "METHOD";
                default:
                    return "INPUT "+QString::number(portIndex);
                }
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType)
            {
            case PortType::In:
                switch (portIndex)
                {
            case 0:
                    return ImageData().type();
            case 1:
                    return VariableData().type();

            default:
                    return VariableData().type();
            };

            case PortType::Out:
                return ImageData().type();
            default:
                return ImageData().type();
            }
        }



        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return m_outImage;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            switch (portIndex)
            {
            case 0:
                m_inImage=std::dynamic_pointer_cast<ImageData>(data);
                updateImage();
                break;
            case 1:
                widget->threshEdit->setValue(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 2:
                widget->maxvalEdit->setValue(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            case 3:
                widget->methodEdit->setCurrentIndex(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                break;
            }
        }
        void updateImage()
        {
            if (m_inImage) {
                cv::Mat inputMat = m_inImage->imgMat();
                cv::Mat thresholdedMat;

                // 转换为灰度图像（如果输入是彩色）
                if (inputMat.channels() > 1) {
                    cv::cvtColor(inputMat, inputMat, cv::COLOR_BGR2GRAY);
                }

                // 获取参数值
                double thresh = widget->threshEdit->value();
                double maxval = widget->maxvalEdit->value();
                int method = widget->methodEdit->currentIndex();
                // 执行阈值操作
                cv::threshold(inputMat, thresholdedMat, thresh, maxval, method);

                m_outImage = std::make_shared<ImageData>(thresholdedMat);
            }
            Q_EMIT dataUpdated(0);
        }
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson  = NodeDelegateModel::save();

            modelJson1["thresh"] = widget->threshEdit->value();
            modelJson1["maxval"] = widget->maxvalEdit->value();
            modelJson1["method"] = widget->methodEdit->currentIndex();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->threshEdit->setValue(v.toObject()["thresh"].toInt());
                widget->maxvalEdit->setValue(v.toObject()["maxval"].toInt());
                widget->methodEdit->setCurrentIndex(v.toObject()["method"].toInt());
            }
        }

    private:

        ImageThresholdInterface *widget=new ImageThresholdInterface();
        std::shared_ptr<NodeData> m_outImage;
        std::shared_ptr<ImageData> m_inImage;
        std::shared_ptr<VariableData> m_inVariable;
    };
}