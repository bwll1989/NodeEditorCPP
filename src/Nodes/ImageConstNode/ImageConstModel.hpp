#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "ImageConstInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class ImageConstModel final : public QtNodes::NodeDelegateModel
    {
        Q_OBJECT

    public:
        ImageConstModel();

        ~ImageConstModel() override = default;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;
    private:
        ImageConstInterface *widget = new ImageConstInterface();
        std::weak_ptr<ImageData> m_outData;
        int m_width = 100;
        int m_height = 100;
        QColor m_color = QColor(255,255,255,255);
        QImage m_image;
    private Q_SLOTS:
        void updateImage();
        void onInputChanged();
    };
}