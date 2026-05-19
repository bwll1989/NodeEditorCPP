#include "ImageInDataModel.hpp"

#include <QVBoxLayout>

#include "../../DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
#include "StatusContainer/GlobalEventBus.hpp"
namespace Nodes {
    ImageInDataModel::ImageInDataModel()
    {

        widget=new DataBridgeSelectorBox();
        InPortCount =0;
        OutPortCount=1;
        CaptionVisible=false;
        Caption="Image In";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        // AbstractDelegateModel::registerExternalControl("/input",widget);
        NodeDelegateModel::ExternalBinding binding;
        binding.member = "remarks";
        binding.control=widget;
        AbstractDelegateModel::registerExternalBinding("/input", this, binding);
        AbstractDelegateModel::setRemarks("Undefined");
        ModelDataBridge::instance().registerEntranceDelegate(this);
        connect(widget,&DataBridgeSelectorBox::selectionChanged,this,&ImageInDataModel::setRemarks);
        connect(this,&ImageInDataModel::remarksChanged,this,[this](const QString &normalizedRemarks){
            ModelDataBridge::instance().updateRemarksForDelegate(this,true,normalizedRemarks);
            if (widget->text()!=normalizedRemarks)
                widget->setCurrentValue(normalizedRemarks);
        });

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


    /**
     * 函数级注释：获取当前图像输入节点的备注属性值
     */
    QString ImageInDataModel::remarks() const
    {
        return getRemarks();
    }


    /**
     * 函数级注释：设置当前图像输入节点的备注属性，并触发变更通知
     */
    void ImageInDataModel::setRemarks(const QString& remarks){
        if (remarks == getRemarks()) {
            return;
        }
        NodeDelegateModel::setRemarks(remarks);
        Q_EMIT remarksChanged(remarks);
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


    /**
     * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
     */
    void ImageInDataModel::afterModelReady()
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
    void ImageInDataModel::onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress("/input")) {
            return;
        }
        setRemarks(ev.payload.toString());
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
}
