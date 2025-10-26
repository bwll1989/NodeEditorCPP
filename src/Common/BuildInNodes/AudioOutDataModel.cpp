#include "AudioOutDataModel.hpp"

#include <QtWidgets/QLabel>
namespace Nodes {
    AudioOutDataModel::AudioOutDataModel()
    {
        InPortCount =2;
        OutPortCount=0;
        CaptionVisible=true;
        Caption="Audio Out";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        setRemarks(QString("AudioExport"));
        ModelDataBridge::instance().registerExportDelegate(this);

    }

    AudioOutDataModel::~AudioOutDataModel()
    {
        ModelDataBridge::instance().unregisterExportDelegate(this);
    }


    NodeDataType AudioOutDataModel::dataType(PortType, PortIndex) const
    {
        return NodeDataTypes::AudioData().type();
    }

    std::shared_ptr<NodeData> AudioOutDataModel::outData(PortIndex port)
    {
        return  _dataMap[port];

    }

    void AudioOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
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

    QWidget *AudioOutDataModel::embeddedWidget()
    {
        return nullptr;
    }
    void AudioOutDataModel::setRemarks(const QString& remarks){
        NodeDelegateModel::setRemarks(remarks);
        ModelDataBridge::instance().updateRemarksForDelegate(this,false,getRemarks());
    };
}