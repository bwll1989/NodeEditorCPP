#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QComboBox>
#include <iostream>
#include "ModelDataBridge/ModelDataBridge.hpp"
#include "Elements/DataBridgeSelectorBox/DataBridgeSelectorBox.hpp"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
namespace Nodes {
    class AudioInDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        AudioInDataModel();

        virtual ~AudioInDataModel();

    public:
        QJsonObject save() const override;

        void load(QJsonObject const &p) override;

    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

        std::shared_ptr<NodeData> outData(PortIndex port) override;

        void setInData(std::shared_ptr<NodeData>, PortIndex) override;

        QWidget *embeddedWidget() override;

        void setRemarks(const QString& remarks) override;

        void stateFeedBack(const QString& oscAddress,QVariant value) override;
    private:
        std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
        DataBridgeSelectorBox *widget;

    };
}