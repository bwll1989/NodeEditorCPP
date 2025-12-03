#include "ImageInDataModel.hpp"

#include <QVBoxLayout>

#include "../../DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
namespace Nodes {
    ImageInDataModel::ImageInDataModel()
    {

        widget=new DataBridgeSelectorBox();
        InPortCount =0;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Image In";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        NodeDelegateModel::registerOSCControl("/input",widget);
        NodeDelegateModel::setRemarks("Undefined");
        ModelDataBridge::instance().registerEntranceDelegate(this);
        connect(widget,&DataBridgeSelectorBox::selectionChanged,this,&ImageInDataModel::setRemarks);

    }

    ImageInDataModel::~ImageInDataModel() {
        ModelDataBridge::instance().unregisterEntranceDelegate(this);
    }
    QJsonObject ImageInDataModel::save() const
    {
        QJsonObject modelJson = NodeDelegateModel::save();

        modelJson["quoted address"] =widget->text();

        return modelJson;
    }


    void ImageInDataModel::setRemarks(const QString& remarks){
        NodeDelegateModel::setRemarks(remarks);
        ModelDataBridge::instance().updateRemarksForDelegate(this,true,getRemarks());
    };

    void ImageInDataModel::load(QJsonObject const &p)
    {
        QJsonValue v = p["quoted address"];

        if (!v.isUndefined()) {
            QString strNum = v.toString();
            if (widget)
                widget->setCurrentValue(strNum);
        }

    }


    NodeDataType ImageInDataModel::dataType(PortType, PortIndex) const
    {
        return NodeDataTypes::ImageData().type();
    }

    std::shared_ptr<NodeData> ImageInDataModel::outData(PortIndex port)
    {
        return _dataMap[port];
    }

    QWidget *ImageInDataModel::embeddedWidget()
    {
        return widget;
    }

    void ImageInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
    {
        _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::ImageData>(data);
        emit dataUpdated(portIndex);
    }


    void ImageInDataModel::stateFeedBack(const QString& oscAddress,QVariant value){

        OSCMessage message;
        message.host = AppConstants::EXTRA_FEEDBACK_HOST;
        message.port = AppConstants::EXTRA_FEEDBACK_PORT;
        message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
        message.value = value;
        OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
}