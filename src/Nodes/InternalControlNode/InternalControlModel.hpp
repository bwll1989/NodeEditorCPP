#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "InternalControlInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class InternalControlModel final : public QtNodes::NodeDelegateModel
    {
        Q_OBJECT

    public:
        InternalControlModel();

        ~InternalControlModel() override = default;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;
    private Q_SLOTS:
        void trigger();
    private:
        InternalControlInterface *widget = new InternalControlInterface();
    };
}