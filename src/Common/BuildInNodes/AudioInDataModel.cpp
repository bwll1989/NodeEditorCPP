#include "AudioInDataModel.hpp"

#include <QVBoxLayout>

#include "../../DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>

using namespace QtNodes;
namespace Nodes {
    AudioInDataModel::AudioInDataModel()
    {
		widget=new DataBridgeSelectorBox();
        InPortCount =0;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Audio In";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        NodeDelegateModel::registerOSCControl("/input",widget);
        ModelDataBridge::instance().registerEntranceDelegate(this);
        connect(widget,&DataBridgeSelectorBox::selectionChanged,this,&AudioInDataModel::setRemarks);
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


    void AudioInDataModel::setRemarks(const QString& remarks){
        QString trimmedRemarks = remarks.trimmed();

        if (trimmedRemarks.isEmpty())
            trimmedRemarks="Undefined";
        NodeDelegateModel::setRemarks(trimmedRemarks);
        ModelDataBridge::instance().updateRemarksForDelegate(this,true,getRemarks());
        ModelDataBridge::instance().requestDataManual(this->getRemarks());
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
    void AudioInDataModel::stateFeedBack(const QString& oscAddress,QVariant value){

        OSCMessage message;
        message.host = AppConstants::EXTRA_FEEDBACK_HOST;
        message.port = AppConstants::EXTRA_FEEDBACK_PORT;
        message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
        message.value = value;
        OSCSender::instance()->sendOSCMessageWithQueue(message);
    }
}