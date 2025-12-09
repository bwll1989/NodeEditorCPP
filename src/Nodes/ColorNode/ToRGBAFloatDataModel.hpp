#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include "ColorConvertInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class ToRGBAFloatDataModel final : public QtNodes::NodeDelegateModel
    {
        Q_OBJECT

    public:
        ToRGBAFloatDataModel();

        ~ToRGBAFloatDataModel() override;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return widget; }



        void stateFeedBack(const QString& oscAddress,QVariant value) override ;

    private:
        ColorConvertInterface *widget = new ColorConvertInterface();
        QColor m_color = QColor(0,0,0,255);

    private Q_SLOTS:
        void onColorChanged(const QColor& c);
    };
}