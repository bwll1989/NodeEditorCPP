#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ImageConstInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class ImageConstModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        ImageConstModel();

        ~ImageConstModel() override;

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
        std::weak_ptr<ImageData> m_outData;
        int m_width = 100;
        int m_height = 100;
        ColorEditorWidget *colorEditorWidget=new ColorEditorWidget();
        QColor m_color = QColor(0,0,0,255);
        cv::Mat m_image=cv::Mat(100, 100, CV_8UC4, cv::Scalar(0, 0, 0, 255));;
        QFutureWatcher<void> *m_watcher = nullptr;
    private Q_SLOTS:
        void updateImage();
        void onInputChanged();
    };
}