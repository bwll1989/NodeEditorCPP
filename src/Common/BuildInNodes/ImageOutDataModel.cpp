#include "ImageOutDataModel.hpp"

#include <QtWidgets/QLabel>
namespace Nodes {
    ImageOutDataModel::ImageOutDataModel()
    {
        InPortCount =2;
        OutPortCount=0;
        CaptionVisible=true;
        Caption="Image Out";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        setRemarks(QString("ImageExport"));
        ModelDataBridge::instance().registerExportDelegate(this);

    }

    ImageOutDataModel::~ImageOutDataModel()
    {
        ModelDataBridge::instance().unregisterExportDelegate(this);
    }


    NodeDataType ImageOutDataModel::dataType(PortType, PortIndex) const
    {
        return NodeDataTypes::ImageData().type();
    }

    std::shared_ptr<NodeData> ImageOutDataModel::outData(PortIndex port)
    {
        return  _dataMap[port];

    }

    void ImageOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
    {
        _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::ImageData>(data);
        emit dataUpdated(portIndex);
    }

    QWidget *ImageOutDataModel::embeddedWidget()
    {
        return nullptr;
    }
    void ImageOutDataModel::setRemarks(const QString& remarks){
        NodeDelegateModel::setRemarks(remarks);
        ModelDataBridge::instance().updateRemarksForDelegate(this,false,getRemarks());
    };
}