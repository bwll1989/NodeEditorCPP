//
// Created by pablo on 2/29/24.
//
#pragma once
#include <qtreeview.h>
#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "QPropertyBrowser.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class ImageInfoModel : public NodeDelegateModel {
        Q_OBJECT
    public:
        ImageInfoModel() {
            InPortCount =1;
            OutPortCount=2;
            CaptionVisible=true;
            Caption="Image Info";
            WidgetEmbeddable= true;
            Resizable=true;
            PortEditable= false;

        };

        ~ImageInfoModel() override{};


        NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
            switch (portType) {
            case PortType::In:
                return ImageData().type();
            case PortType::Out:
                switch (portIndex) {
                    case 0:
                            return ImageData().type();
                    case 1:
                            return VariableData().type();
                    default:
                        break;
                }
            default:
                break;
            }
            return ImageData().type();
        };

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
            switch (portIndex) {
            case 0: {
                    m_inImageData = std::dynamic_pointer_cast<ImageData>(nodeData);
                    updateData();
                    break;
            }
            default:
                break;
            }
        }

        std::shared_ptr<NodeData> outData(const QtNodes::PortIndex port) override {
            switch (port) {
            case 0:
                return m_outImageData;
            case 1:
                return m_proprtyData;
            default:
                return nullptr;
            }
        }

        QWidget* embeddedWidget() override {

            return widget;
        }

    private:
        void invalidateOutData(){
            m_outImageData.reset();
            m_proprtyData.reset();
        }

        void updateData(){
            const auto imageLock = m_inImageData.lock();

            if (!imageLock) {
                invalidateOutData();
                m_proprtyData=std::make_shared<VariableData>();
                m_proprtyData->insert("isNull",true);
            } else {
                m_outImageData = std::make_shared<ImageData>(imageLock->imgMat());
                m_proprtyData = std::make_shared<VariableData>(imageLock->getMap());
                widget-> buildPropertiesFromMap(imageLock->getMap());
            }


            emit dataUpdated(0);
            emit dataUpdated(1);
        }



    private:
        QPropertyBrowser *widget=new QPropertyBrowser();
        // in
        std::weak_ptr<ImageData> m_inImageData;
        // out
        std::shared_ptr<VariableData> m_proprtyData;
        // 0
        std::shared_ptr<ImageData> m_outImageData;
        // // 1

    };
}

