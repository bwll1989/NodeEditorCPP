#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>

#include <iostream>
#include <QLineEdit>
#include "../../DataTypes/NodeDataList.hpp"
#include "../Devices/ModelDataBridge/ModelDataBridge.hpp"
#include "AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///
namespace Nodes {
    class VariableOutDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        VariableOutDataModel();

        ~VariableOutDataModel();


    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

        std::shared_ptr<NodeData> outData(PortIndex port) override;

        void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;

        QWidget *embeddedWidget() override;

        void setRemarks(const QString& remarks) override;
    private:
        std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
    };
}