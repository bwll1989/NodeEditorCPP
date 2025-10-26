#include "AudioInDataModel.hpp"

#include <QVBoxLayout>

#include "../../DataTypes/NodeDataList.hpp"
#include <QSpinBox>
#include <QtCore/QJsonValue>
#include <QtGui/QDoubleValidator>
#include <QtWidgets/QLineEdit>
namespace Nodes {
    AudioInDataModel::AudioInDataModel()
        : _inputSelector{new QComboBox()}
    {
        InPortCount =0;
        OutPortCount=2;
        CaptionVisible=true;
        Caption="Audio In";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        registerOSCControl("/index",_inputSelector);
        setRemarks("Undefined");
        ModelDataBridge::instance().registerEntranceDelegate(this);
        connect(_inputSelector,&QComboBox::currentTextChanged,this,&AudioInDataModel::setRemarks);

    }

    AudioInDataModel::~AudioInDataModel() {
        ModelDataBridge::instance().unregisterEntranceDelegate(this);
    }
    QJsonObject AudioInDataModel::save() const
    {
        QJsonObject modelJson = NodeDelegateModel::save();

        modelJson["quoted address"] =_inputSelector->currentText();

        return modelJson;
    }


    void AudioInDataModel::setRemarks(const QString& remarks){
        NodeDelegateModel::setRemarks(remarks);
        ModelDataBridge::instance().updateRemarksForDelegate(this,true,getRemarks());
    };
    void AudioInDataModel::load(QJsonObject const &p)
    {
        QJsonValue v = p["quoted address"];

        if (!v.isUndefined()) {
            QString strNum = v.toString();
            if (_inputSelector)
                _inputSelector->setCurrentText(strNum);
        }

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
        _inputSelector->clear();
        _inputSelector->addItems(ModelDataBridge::instance().getAllExportRemarks());
        return _inputSelector;
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