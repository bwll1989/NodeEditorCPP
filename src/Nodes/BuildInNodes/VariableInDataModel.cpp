#include "VariableInDataModel.hpp"

#include <QVBoxLayout>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace QtNodes;
namespace Nodes
{
VariableInDataModel::VariableInDataModel()
 {

    widget=new DataBridgeSelectorBox();
    InPortCount =0;
    OutPortCount=1;
    CaptionVisible=false;
    Caption="Variable In";
    WidgetEmbeddable= true;
    Resizable=false;
    PortEditable= true;
    NodeDelegateModel::ExternalBinding binding;
    binding.member = "remarks";
    binding.control=widget;
    AbstractDelegateModel::registerExternalBinding("/input", this, binding);
    // AbstractDelegateModel::registerExternalControl("/input",widget);
    setRemarks("Undefined");
    ModelDataBridge::instance().registerEntranceDelegate(this);
    connect(widget,&DataBridgeSelectorBox::selectionChanged,this,&VariableInDataModel::setRemarks);
    connect(this,&VariableInDataModel::remarksChanged,this,[this](const QString &normalizedRemarks){
        ModelDataBridge::instance().updateRemarksForDelegate(this,true,normalizedRemarks);
        if (widget->text()!=normalizedRemarks)
            widget->setCurrentValue(normalizedRemarks);
    });

}

VariableInDataModel::~VariableInDataModel() {
    ModelDataBridge::instance().unregisterEntranceDelegate(this);
}
QJsonObject VariableInDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();

    modelJson["quoted address"] =widget->text();

    return modelJson;
}


/**
 * 函数级注释：获取当前变量输入节点的备注属性值
 */
QString VariableInDataModel::remarks() const
{
    return getRemarks();
}


/**
 * 函数级注释：设置当前变量输入节点的备注属性，并触发变更通知
 */
void VariableInDataModel::setRemarks(const QString& remarks){
    if (remarks == getRemarks()) {
        return;
    }
    NodeDelegateModel::setRemarks(remarks);
    Q_EMIT remarksChanged(remarks);
};
void VariableInDataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["quoted address"];

    if (!v.isUndefined()) {
        QString strNum = v.toString();
        if (widget)
            widget->setCurrentValue(strNum);
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
    return widget;
}

void VariableInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
   _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    emit dataUpdated(portIndex);
}

/**
 * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
 */
void VariableInDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/input"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}

/**
 * 函数级注释：处理来自全局事件总线的外部命令，更新备注属性
 */
void VariableInDataModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }
    if (ev.address != makeFullOscAddress("/input")) {
        return;
    }
    setRemarks(ev.payload.toString());
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
