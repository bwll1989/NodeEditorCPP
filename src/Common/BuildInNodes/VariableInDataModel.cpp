#include "VariableInDataModel.hpp"

#include <QVBoxLayout>

#include "../../DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
using namespace QtNodes;
namespace Nodes
{
VariableInDataModel::VariableInDataModel()
    : _inputSelector{new QComboBox()}
 {
    InPortCount =0;
    OutPortCount=2;
    CaptionVisible=true;
    Caption="Variable In";
    WidgetEmbeddable= true;
    Resizable=false;
    PortEditable= true;
    registerOSCControl("/number",_inputSelector);
    setRemarks("Undefined");
    ModelDataBridge::instance().registerEntranceDelegate(this);
    connect(_inputSelector,&QComboBox::currentTextChanged,this,&VariableInDataModel::setRemarks);

}

VariableInDataModel::~VariableInDataModel() {
    ModelDataBridge::instance().unregisterEntranceDelegate(this);
}
QJsonObject VariableInDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();

    modelJson["quoted address"] =_inputSelector->currentText();

    return modelJson;
}


void VariableInDataModel::setRemarks(const QString& remarks){
    NodeDelegateModel::setRemarks(remarks);
    ModelDataBridge::instance().updateRemarksForDelegate(this,true,getRemarks());
};
void VariableInDataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["quoted address"];

    if (!v.isUndefined()) {
        QString strNum = v.toString();
        if (_inputSelector)
            _inputSelector->setCurrentText(strNum);
        }

}


NodeDataType VariableInDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> VariableInDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

QWidget *VariableInDataModel::embeddedWidget()
{
    _inputSelector->clear();
    _inputSelector->addItems(ModelDataBridge::instance().getAllExportRemarks());
    return _inputSelector;
}

void VariableInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
   _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    emit dataUpdated(portIndex);
}

ConnectionPolicy VariableInDataModel::portConnectionPolicy(PortType portType, PortIndex index) const {
    auto result = ConnectionPolicy::One;
    switch (portType) {
        case PortType::In:
            result = ConnectionPolicy::Many;
            break;
        case PortType::Out:
            result = ConnectionPolicy::Many;
            break;
        case PortType::None:
            break;
    }

    return result;
}
}