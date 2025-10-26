#include "ImageInDataModel.hpp"

#include <QVBoxLayout>

#include "../../DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
namespace Nodes
{
ImageInDataModel::ImageInDataModel()
    : _inputSelector{new QComboBox()}
 {
    InPortCount =0;
    OutPortCount=2;
    CaptionVisible=true;
    Caption="Image In";
    WidgetEmbeddable= true;
    Resizable=false;
    PortEditable= true;
    registerOSCControl("/index",_inputSelector);
    setRemarks("Undefined");
    ModelDataBridge::instance().registerEntranceDelegate(this);
    connect(_inputSelector,&QComboBox::currentTextChanged,this,&ImageInDataModel::setRemarks);

}

ImageInDataModel::~ImageInDataModel() {
    ModelDataBridge::instance().unregisterEntranceDelegate(this);
}
QJsonObject ImageInDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();

    modelJson["quoted address"] =_inputSelector->currentText();

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
        if (_inputSelector)
            _inputSelector->setCurrentText(strNum);
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
    _inputSelector->clear();
    _inputSelector->addItems(ModelDataBridge::instance().getAllExportRemarks());
    return _inputSelector;
}

void ImageInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
   _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::ImageData>(data);
    emit dataUpdated(portIndex);
}
}
