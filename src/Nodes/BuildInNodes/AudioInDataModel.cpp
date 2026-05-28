#include "AudioInDataModel.hpp"

#include <QVBoxLayout>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
#include "StatusContainer/GlobalEventBus.hpp"

using namespace QtNodes;
namespace Nodes {
    AudioInDataModel::AudioInDataModel()
    {
		widget=new DataBridgeSelectorBox();
        InPortCount =0;
        OutPortCount=1;
        CaptionVisible=false;
        Caption="Audio In";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        NodeDelegateModel::ExternalBinding binding;
        binding.member = "remarks";
        binding.control=widget;
        AbstractDelegateModel::registerExternalBinding("/input", this, binding);
        ModelDataBridge::instance().registerEntranceDelegate(this);
        connect(widget,&DataBridgeSelectorBox::selectionChanged,this,&AudioInDataModel::setRemarks);
        connect(this,&AudioInDataModel::remarksChanged,this,[this](const QString &normalizedRemarks){
            ModelDataBridge::instance().updateRemarksForDelegate(this,true,normalizedRemarks);
            ModelDataBridge::instance().requestDataManual(normalizedRemarks);
            if (widget->text()!=normalizedRemarks)
                widget->setCurrentValue(normalizedRemarks);
        });
    }

    AudioInDataModel::~AudioInDataModel() {
        ModelDataBridge::instance().unregisterEntranceDelegate(this);
    }
    QJsonObject AudioInDataModel::save() const
    {
        QJsonObject modelJson = NodeDelegateModel::save();

    	modelJson["quoted address"] =widget->text();

        return modelJson;
    }


    /**
     * 函数级注释：获取当前音频输入节点的备注属性值
     */
    QString AudioInDataModel::remarks() const
    {
        return getRemarks();
    }

    /**
     * 函数级注释：设置当前音频输入节点的备注属性，并触发变更通知
     */
    void AudioInDataModel::setRemarks(const QString& remarks){
        QString trimmedRemarks = remarks.trimmed();

        if (trimmedRemarks.isEmpty())
            trimmedRemarks="Undefined";

        if (trimmedRemarks == getRemarks()) {
            return;
        }

        NodeDelegateModel::setRemarks(trimmedRemarks);
        Q_EMIT remarksChanged(trimmedRemarks);
    };

    void AudioInDataModel::load(QJsonObject const &p)
    {
        QJsonValue v = p["quoted address"];

        if (!v.isUndefined()) {
            QString strNum = v.toString();
            if (widget)
                widget->setCurrentValue(strNum);
        }
        ModelDataBridge::instance().requestDataManual(this->getRemarks());

    }


    /**
     * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
     */
    void AudioInDataModel::afterModelReady()
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
    void AudioInDataModel::onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command) {
            return;
        }
        if (ev.address != makeFullOscAddress("/input")) {
            return;
        }
        setRemarks(ev.payload.toString());
    }


    NodeDataType AudioInDataModel::dataType(PortType, PortIndex) const
    {
        return NodeDataTypes::AudioData().type();
    }

    std::shared_ptr<NodeData> AudioInDataModel::outData(PortIndex port)
    {

        return _dataMap[port];
    }

    QWidget *AudioInDataModel::embeddedWidget()
    {
        return widget;
    }

    void AudioInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
    {
        auto audioData = std::dynamic_pointer_cast<NodeDataTypes::AudioData>(data);
        // 检查是否为有效的音频数据连接
        if (audioData && audioData->isConnectedToSharedBuffer()) {
            _dataMap[portIndex]=audioData;
        }else {
            _dataMap[portIndex]=nullptr;
        }
        emit dataUpdated(portIndex);
    }

}
