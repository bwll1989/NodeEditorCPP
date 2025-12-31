#include "showStoreGBxDataModel.hpp"
#include "../../Common/Devices/JSEngineDefines/JSEngineDefines.hpp"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJSEngine>
#include <QDateTime>
#include <QCoreApplication>
#include "ConstantDefines.h"

namespace Nodes {

showStoreGBxDataModel::showStoreGBxDataModel()
    : _interface(new showStoreGBxInterface())
    , _tcpClient(new TcpClient("127.0.0.1",23))
    , logData(std::shared_ptr<NodeDataTypes::VariableData>())
{
    InPortCount =4;
    OutPortCount=1;
    PortEditable=false;
    CaptionVisible=true;
    Caption="showStoreGBx";
    WidgetEmbeddable= false;
    Resizable=false;

    for (int i = 0; i < 4; ++i)
    {
        NodeDelegateModel::registerOSCControl("/channel"+QString::number(i+1),_interface->_playButtons[i]);
    }
    NodeDelegateModel::registerOSCControl("/stop",_interface->Stop);
    NodeDelegateModel::registerOSCControl("/status",_interface->status);

    // 连接信号和槽
    connect(_tcpClient, &TcpClient::recMsg, this, [this](const QVariantMap &dataMap) {
        if (dataMap.contains("default")) {
            recMsg(dataMap.value("ascii").toString());
        }
    });
    
    // 连接TCP客户端连接状态信号
    connect(_tcpClient, &TcpClient::isReady, this, [this](const bool &isReady) {
        _interface->setConnectionStatus(isReady);
    });
    
    // 连接输出状态改变信号
    connect(_interface, &showStoreGBxInterface::outputChanged, this, [this](int index, bool state) {
        if (index >= 0 && index < 8) {
            setOutput(index, state);
        }
    });
    
    // 连接主机和端口变更信号
    connect(_interface, &showStoreGBxInterface::hostChanged, this, [this](const QString &host) {
            _tcpClient->disconnectFromServer();
            _tcpClient->connectToServer(host,_interface->getPort());

    });
    connect(_interface->Stop,&QPushButton::clicked, this, [this](){
         _tcpClient->sendMessage("SP\r",1);
     });
}

showStoreGBxDataModel::~showStoreGBxDataModel()
{
    delete _tcpClient;
}


NodeDataType showStoreGBxDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In) {

        return NodeDataTypes::VariableData().type();

    } else if (portType == PortType::Out) {
        return NodeDataTypes::VariableData().type();
    }

    return NodeDataType();
}

std::shared_ptr<NodeData> showStoreGBxDataModel::outData(PortIndex port)
{
    return logData;
}

void showStoreGBxDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);

    if (!varData) {
        return;
    }
    if (varData->value().toBool())
        _interface->_playButtons[port]->clicked();
}

QString showStoreGBxDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    switch (portType) {
    case PortType::In:
        return "SHOW "+QString::number(portIndex);
    case PortType::Out:

        return "LOG " +QString::number(portIndex);
    default:
        break;
    }
    return "";
}
QJsonObject showStoreGBxDataModel::save() const
{
    QJsonObject modelJson  = NodeDelegateModel::save();
    QJsonObject modelJson1;
    modelJson1["host"] = _interface->getHost();
    modelJson["values"] = modelJson1;
    return modelJson;
}

void showStoreGBxDataModel::load(QJsonObject const &p)
{
    QJsonValue v = p["values"];
    if (!v.isUndefined()&&v.isObject())
    {
        // 加载主机和端口信息
        if (p.contains("host")) {
            QString host = p["host"].toString();

            _interface->setHost(host);
        }
        // 连接到服务器
        if (!_interface->getHost().isEmpty()) {
            _tcpClient->connectToServer(_interface->getHost(), _interface->getPort());
        }
    }
}

void showStoreGBxDataModel::recMsg(QString msg)
{
    // 处理接收到的Modbus响应
    processResponse(msg);
}
void showStoreGBxDataModel::processResponse(const QString &msg)
{
    
    // 检查logData是否为空,为空则创建新的数据
    if(!logData) {
        logData = std::make_shared<NodeDataTypes::VariableData>();
    }
    logData->insert("default", msg);
    emit dataUpdated(0);
}
void showStoreGBxDataModel::setOutput(int index, bool state)
{
    _tcpClient->sendMessage(QString("ST%1\r").arg(index + 1, 2, 10, QChar('0')), 1);
}


} // namespace Nodes