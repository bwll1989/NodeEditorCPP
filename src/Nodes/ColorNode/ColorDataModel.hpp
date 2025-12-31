#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ColorInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class ColorDataModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        ColorDataModel();

        ~ColorDataModel() override;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;

    public Q_SLOTS:
        void toggleEditorMode();
    private:
        ImageConstInterface *widget = new ImageConstInterface();
        ColorEditorWidget *colorEditorWidget=new ColorEditorWidget();
        QColor m_color = QColor(0,0,0,255);

    private Q_SLOTS:
        void onColorChanged(const QColor& c);
    };
}