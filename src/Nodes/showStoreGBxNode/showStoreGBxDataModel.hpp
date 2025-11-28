#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeData>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>
#include <QtQml/QJSValueIterator> 
#include "../../Common/Devices/TcpClient/TcpClient.h"
#include "DataTypes/NodeDataList.hpp"
#include "showStoreGBxInterface.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

namespace Nodes {

class showStoreGBxDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    showStoreGBxDataModel();

    ~showStoreGBxDataModel() override;

public:
    QJsonObject save() const override;

    void load(QJsonObject const &p) override;

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
public:
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, PortIndex port) override;

    QWidget *embeddedWidget() override { return _interface; }

public slots:
    void stateFeedBack(const QString& oscAddress,QVariant value) override;

private slots:
    void recMsg(QString msg);
    void setOutput(int index, bool state);

private:
    showStoreGBxInterface *_interface;
    TcpClient *_tcpClient;
    // 处理响应
    void processResponse(const QString &response);
    std::shared_ptr<NodeDataTypes::VariableData> logData;

};

} // namespace Nodes