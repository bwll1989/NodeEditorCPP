#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include "AbstractDelegateModel.h"
#include <iostream>
#include <QLineEdit>
#include "../../DataTypes/NodeDataList.hpp"
#include "../Devices/ModelDataBridge/ModelDataBridge.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
namespace Nodes {
    class ImageOutDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        ImageOutDataModel();

        ~ImageOutDataModel();


    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

        std::shared_ptr<NodeData> outData(PortIndex port) override;

        void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

        QWidget *embeddedWidget() override;

        void setRemarks(const QString& remarks) override;
    private:
        std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
    };
}