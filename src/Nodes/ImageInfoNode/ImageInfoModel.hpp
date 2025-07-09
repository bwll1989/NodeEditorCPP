//
// Created by pablo on 2/29/24.
//
#pragma once
#ifndef IMAGEINFOMODEL_H
#define IMAGEINFOMODEL_H
#include <qtreeview.h>
#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
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
                m_proprtyData=std::make_shared<VariableData>(imageLock->image());
                m_outImageData = std::make_shared<ImageData>(imageLock->image());
                m_proprtyData->insert("isNull",imageLock->image().isNull());
                m_proprtyData->insert("isGrayScale",imageLock->image().isGrayscale());
                m_proprtyData->insert("hasAlpha",imageLock->image().hasAlphaChannel());
                m_proprtyData->insert("format",formatToString(imageLock->image().format()));
                m_proprtyData->insert("size",imageLock->image().size());
                m_proprtyData->insert("rect",imageLock->image().rect());
            }
            widget-> buildPropertiesFromMap(m_proprtyData->getMap());

            emit dataUpdated(0);
            emit dataUpdated(1);
        }

        static QString formatToString(QImage::Format format){
            switch (format) {
            case QImage::Format_Invalid: return "Invalid";
            case QImage::Format_Mono: return "Mono";
            case QImage::Format_MonoLSB: return "MonoLSB";
            case QImage::Format_Indexed8: return "Indexed8";
            case QImage::Format_RGB32: return "RGB32";
            case QImage::Format_ARGB32: return "ARGB32";
            case QImage::Format_ARGB32_Premultiplied: return "ARGB32 Premultiplied";
            case QImage::Format_RGB16: return "RGB16";
            case QImage::Format_ARGB8565_Premultiplied: return "ARGB8565 Premultiplied";
            case QImage::Format_RGB666: return "RGB666";
            case QImage::Format_ARGB6666_Premultiplied: return "ARGB6666 Premultiplied";
            case QImage::Format_RGB555: return "RGB555";
            case QImage::Format_ARGB8555_Premultiplied: return "ARGB8555 Premultiplied";
            case QImage::Format_RGB888: return "RGB888";
            case QImage::Format_RGB444: return "RGB444";
            case QImage::Format_ARGB4444_Premultiplied: return "ARGB4444 Premultiplied";
            case QImage::Format_RGBX8888: return "RGBX8888";
            case QImage::Format_RGBA8888: return "RGBA8888";
            case QImage::Format_RGBA8888_Premultiplied: return "RGBA8888 Premultiplied";
            case QImage::Format_BGR30: return "BGR30";
            case QImage::Format_A2BGR30_Premultiplied: return "A2BGR30 Premultiplied";
            case QImage::Format_RGB30: return "RGB30";
            case QImage::Format_A2RGB30_Premultiplied: return "A2RGB30 Premultiplied";
            case QImage::Format_Alpha8: return "Alpha8";
            case QImage::Format_Grayscale8: return "Grayscale8";
                // Include all other formats you need to handle
            default: return "Unknown Format";
            }
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

#endif //IMAGEINFOMODEL_H
